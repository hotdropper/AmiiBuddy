//
// Created by Jacob Mather on 8/17/20.
//

#include "read.h"
#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <M5ez.h>
#include <Esp.h>
#include <map>
#include <string>
#include <AmiiboDBAO.h>
#include <Preferences.h>
#include <FSTools.h>
#include "../menus.h"
#include "../init.h"
#include "../firmware.h"
#include "../utils.h"
#include "../classes/AmiiboDatabaseManager.h"
#include "../classes/NFCMonitor.h"
#include "../classes/NTag215Magic.h"
#include "../classes/NTag215Generic.h"


void doRead() {
    auto pb = new ezProgressBar("Reading...", "Reading amiibo to tag...");

    NTag215* tag;
    switch (TARGET_TAG_TYPE) {
        case TARGET_MAGIC_NTAG_215:
            tag =  new NTag215Magic();
            break;
        case TARGET_NTAG_215:
            tag = new NTag215Generic();
            break;
        default:
            M5ez::msgBox(TEXT_WARNING, "Puck.JS target is not currently supported.", TEXT_OK);
            return;
    }

    int readResult = nfcMonitor.monitorAmiiboRead(pb, tag);

    String btn;

    switch (readResult) {
        case 0:
            break;
        case -1:
            M5ez::msgBox("Error", "No tag was found by the reader.");
            return;
        case -2:
            M5ez::msgBox("Error", "The tag does not seem to be an Amiibo.");
            return;
        case -3:
            btn = M5ez::msgBox("Error", "We had trouble reading the tag. Try again?", "Cancel##Ok");
            free(tag);
            if (btn == "Ok") {
                return doRead();
            }
            return;
        default:
            M5ez::msgBox("Error", "Unknown error occurred.");
            free(tag);
            return;
    }

    PRINTLN("Read Tag Data");
    PRINTHEX(tag->data, NTAG215_SIZE);

    HashInfo hashes;
    AmiiboDBAO::calculateHashes(tag->data, hashes);
    M5ez::yield();
    printAmiibo(atool.amiiboInfo);
    PRINTV("Amiibo Hash: ", hashes.amiiboHash);
    PRINTV("Save Hash: ", hashes.saveHash);
    AmiiboRecord amiibo;
    bool foundAmiibo = AmiiboDBAO::getAmiiboByHash(hashes.amiiboHash, amiibo);
    PRINTV("Found amiibo?", foundAmiibo);

    ezMenu myMenu("Save Amiibo");
    myMenu.txtSmall();
    myMenu.buttons("up#Cancel#Select##down#New");
    bool sawSaveHash = false;
    int customSaveCount = 0;
    int saveLookupResult = AmiiboDBAO::findSavesByAmiiboHash(hashes.amiiboHash, [&myMenu, &sawSaveHash, &hashes, &customSaveCount](SaveRecord& save) {
        if (strcmp(save.hash, hashes.saveHash) == 0) {
            sawSaveHash = true;
        }

        if (save.is_custom) {
            customSaveCount++;
            auto item = new String(save.id);
            *item = *item + "_" + save.file + "|" + save.name;
            myMenu.addItem(*item);
        }
    });

    if (! foundAmiibo) {
        M5ez::msgBox("Error", "This does not seem to be an Amiibo we recognize.");
        free(tag);
        return;
    } else if (saveLookupResult < 1) {
        M5ez::msgBox("Error", "We must have misread the tag. Please try again.");
        free(tag);
        return;
    }

    if (sawSaveHash) {
        String msg("We would recognize ");
        msg = msg + amiibo.name + " anywhere!\nWe found no changes.";
        M5ez::msgBox(TEXT_SUCCESS, msg, TEXT_OK);
        free(tag);
        return;
    }

    // new save
    String msg = String("It looks like ");
    msg = msg + amiibo.name + " has new data. Save it?";

    String button = M5ez::msgBox("Success", msg, "Cancel##Save");

    if (button == "Cancel") {
        free(tag);
        return;
    }

    if (customSaveCount > 0) {
        myMenu.runOnce();

        PRINTV("Selected item name:", myMenu.pickName());
        PRINTV("Selected item caption:", myMenu.pickCaption());
        PRINTV("Selected item button:", myMenu.pickButton());

        if (myMenu.pickButton() == "Cancel") {
            free(tag);
            return;
        }
        if (myMenu.pickButton() == "Select") {
            String value = myMenu.pickName();
            int saveId = value.substring(0, value.indexOf("_")).toInt();
            String saveFile = value.substring(value.indexOf("_") + 1);
            if (! FSTools::remove(saveFile.c_str())) {
                PRINTLN("Could not remove old file.");
                M5ez::msgBox(TEXT_ERROR, "We were unable to save the file.", TEXT_OK);
                free(tag);
                return;
            }

            if (FSTools::writeData(saveFile.c_str(), tag->data, NTAG215_SIZE) != NTAG215_SIZE) {
                M5ez::msgBox(TEXT_ERROR, "We were unable to save the file.", TEXT_OK);
                free(tag);
                return;
            }
            SaveRecord save;
            AmiiboDBAO::getSaveById(saveId, save);
            strcpy(save.hash, hashes.saveHash);
            AmiiboDBAO::updateSave(save);
            AmiiboDBAO::updateSaveTimestamp(saveId);
            msg = String("We saved ");
            msg = msg + amiibo.name + " as " + myMenu.pickCaption() + ".";
            M5ez::msgBox(TEXT_SUCCESS, msg, TEXT_OK);
            free(tag);
            return;
        }
    }

    // save new
    String saveName = M5ez::textInput("Save name?");
    String saveFilePath = String(SAVES_PATH "/");
    saveFilePath = saveFilePath + amiibo.hash;
    if (! FSTools::exists(saveFilePath.c_str())) {
        PRINTV("Making directory: ", saveFilePath);
        if (! FSTools::mkdirDeep(saveFilePath.c_str())) {
            PRINTLN("Making directory failed.");
            M5ez::msgBox(TEXT_ERROR, "We were unable to save the file.", TEXT_OK);
            free(tag);
            return;
        }
    }

    saveFilePath = saveFilePath + "/" + hashes.saveHash + "_" + saveName + ".bin";
    PRINTV("Saving file to:", saveFilePath);
    PRINTLN("Data to save:");
    PRINTHEX(tag->data, NTAG215_SIZE);
    int writeResult = FSTools::writeData(saveFilePath.c_str(), tag->data, NTAG215_SIZE);
    if (writeResult < 0) {
        PRINTV("Write failed:", writeResult);
        M5ez::msgBox(TEXT_ERROR, "We were unable to save the file.", TEXT_OK);
        free(tag);
        return;
    }
    SaveRecord newSave = SaveRecord();
    newSave.amiibo_id = amiibo.id;
    strcpy(newSave.hash, hashes.saveHash);
    strcpy(newSave.name, saveName.c_str());
    strcpy(newSave.file, saveFilePath.c_str());
    newSave.is_custom = true;
    AmiiboDBAO::insertSave(newSave);
    AmiiboDBAO::updateSaveTimestamp(newSave.id);
    msg = String("We saved ");
    msg = msg + amiibo.name + " as " + newSave.name + ".";
    M5ez::msgBox(TEXT_SUCCESS, msg, TEXT_OK);
}
