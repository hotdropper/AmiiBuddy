//
// Created by hotdropper on 7/18/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <M5ez.h>
#include <Esp.h>
#include <map>
#include <string>
#include <AmiiboDBAO.h>
#include <Preferences.h>
#include "menus.h"
#include "init.h"
#include "firmware.h"
#include "FSTools.h"
#include "utils.h"
#include "classes/AmiiboDatabaseManager.h"
#include "classes/NFCMonitor.h"
#include "classes/NTag215Magic.h"
#include "classes/NTag215Generic.h"

bool goHome = false;
char adjustedPath[200] = "";
char lastPath[200] = LIBRARY_PATH;
AmiiboRecord menuAmiiboBuff;

void selectTargetTagType() {
    ezMenu myMenu("NFC Target");
    myMenu.txtSmall();
    myMenu.buttons("up#Back#select##down#");

    switch(TARGET_TAG_TYPE) {
        case TARGET_MAGIC_NTAG_215:
            myMenu.addItem("MagicNTag215|Magic NTag 21x (current)");
            myMenu.addItem("NTag215|NTag 215");
            myMenu.addItem("PuckJS|Puck.JS");
            break;
        case TARGET_NTAG_215:
            myMenu.addItem("MagicNTag215|Magic NTag 21x");
            myMenu.addItem("NTag215|NTag 215 (current)");
            myMenu.addItem("PuckJS|Puck.JS");
            break;
        case TARGET_PUCK_JS:
            myMenu.addItem("MagicNTag215|Magic NTag 21x");
            myMenu.addItem("NTag215|NTag 215");
            myMenu.addItem("PuckJS|Puck.JS (current)");
            break;
        default:
            myMenu.addItem("MagicNTag215|Magic NTag 21x (current)");
            myMenu.addItem("NTag215|NTag 215");
            myMenu.addItem("PuckJS|Puck.JS");
    }

    myMenu.runOnce();

    if (myMenu.pickButton() == "Back") {
        return;
    }

    if (myMenu.pickName() == "MagicNTag215") {
        TARGET_TAG_TYPE = TARGET_MAGIC_NTAG_215;
    } else if (myMenu.pickName() == "NTag215") {
        TARGET_TAG_TYPE = TARGET_NTAG_215;
    } else if (myMenu.pickName() == "PuckJS") {
        TARGET_TAG_TYPE = TARGET_PUCK_JS;
    }

    Preferences prefs;
    prefs.begin("amiiBuddy", false);	// read-only
    prefs.putInt("target_tag_type", TARGET_TAG_TYPE);
    prefs.end();
}

void selectFile(const char* filename) {
    if (! PN532_PRESENT) {
        PRINTLN("PN532 not present.");
        return;
    }
    M5ez::yield();

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

    FSTools::readData(filename, tag->data, NTAG215_SIZE);
    HashInfo hashes;
    AmiiboDBAO::calculateHashes(tag->data, hashes);
    AmiiboDBAO::findAmiiboByHash(hashes.amiiboHash, menuAmiiboBuff);

    ezMenu myMenu("Select save?");
    myMenu.buttons("up#Back#select##down#Home");
    myMenu.txtSmall();

    myMenu.addItem("Original|Original Amiibo");
    PRINTLN("Searching");
    printHeapUsage();
    int saveCount = AmiiboDBAO::findSavesByAmiiboHash(hashes.amiiboHash, [&myMenu, &hashes](SaveRecord& record) {
        M5ez::yield();
        if (strcmp(hashes.saveHash, record.hash) == 0) {
            return;
        }
        auto key = new String(record.id);
        *key = *key + "_" + record.file + "|" + record.name;
        myMenu.addItem(*key);
    });
    PRINTV("saveCount:", saveCount);
    PRINTLN("Searched.");
    printHeapUsage();

    M5ez::yield();

    String fileToLoad;

    if (saveCount > 0) {
        myMenu.runOnce();

        if (myMenu.pickButton() == "Home") {
            goHome = true;
            return;
        }

        if (myMenu.pickButton() == "Back") {
            return;
        }

        if (myMenu.pickName() == "Original") {
            fileToLoad = filename;
            AmiiboDBAO::updateAmiiboTimestamp(menuAmiiboBuff.id);
        } else {
            String k = myMenu.pickName();
            int id = k.substring(0, k.indexOf("_")).toInt();
            fileToLoad = k.substring(k.indexOf("_") + 1);
            AmiiboDBAO::updateSaveTimestamp(id);
        }
    } else {
        fileToLoad = filename;
    }

    auto pb = new ezProgressBar("Writing...", "Writing amiibo to tag...");

    PRINTV("File to load:", fileToLoad);

    int readResp = FSTools::readData(fileToLoad.c_str(), tag->data, NTAG215_SIZE);
    if (readResp < 0) {
        PRINTV("Got read result:", readResp);
        M5ez::msgBox(TEXT_ERROR, "We were unable to read the file.", TEXT_OK);
        return;
    }

    if (TARGET_TAG_TYPE == TARGET_NTAG_215) {
        if (tag->inList() != 0) {
            M5ez::msgBox(TEXT_ERROR, "Could not find a tag to write to!", TEXT_OK);
            return;
        }
    }

    atool.loadKey(KEY_FILE);
    atool.loadFileFromData(tag->data, NTAG215_SIZE, false);
    atool.encryptLoadedFile(tag->uid);
    memcpy(tag->data, atool.original, NTAG215_SIZE);

    int burn = nfcMonitor.monitorAmiiboWrite(pb, tag);

    free(tag);

    PRINT("Got burn result: ");
    PRINTLN(burn);
}

void showBrowse(const char *path) {
    M5ez::yield();
    PRINTLN(path);

    if (goHome) {
        return;
    }

    strcpy(lastPath, path);
    FSTools::getFSByPath(path, adjustedPath);

    ezMenu myMenu("Browse");
    myMenu.txtSmall();
    myMenu.buttons("up#Back#select##down#Home");
    char filename[256];

    String partialPath = String(path);
    int lastSlash = partialPath.lastIndexOf("/");
    partialPath = partialPath.substring(lastSlash + 1);
    ezProgressBar pb("Listing...", String("Finding files in ") + partialPath);
    M5ez::yield();

    if (! FSTools::exists(path)) {
        char msg[80] = "Could not find: ";
        strcat(msg, path);
        M5ez::msgBox("Error", msg);
        return;
    }

    int counter = -15;

    FSTools::traverseEntries(path, false, [&counter, &pb, &filename, &myMenu, &path](File* entry) {
        counter++;
        if (counter > 25) {
            counter = 0;
        }

        if (counter >= 0) {
            float val = (float)counter / (float)25 * 100;
            pb.value(val);
            M5ez::redraw();
        }

        if (counter % 5 == 0) {
            M5ez::yield();
        }

        PRINTV("Path: ", path);
        PRINTV("Path Len: ", strlen(adjustedPath));
        PRINTV("File path: ", entry->name());
        PRINTV("File name: ", &entry->name()[strlen(adjustedPath) + 1]);

        if (entry->isDirectory()) {
            strncpy(filename, "./", sizeof(filename));
            strncat(filename, &entry->name()[strlen(adjustedPath) + 1], (sizeof(filename) - strlen(filename) - 1));
            myMenu.addItem(filename);
        } else {
            if (strncmp(&entry->name()[strlen(adjustedPath) + 1], ".DS_Store", strlen(".DS_Store")) == 0) {
                return;
            }
            strncpy(filename, &entry->name()[strlen(adjustedPath) + 1], sizeof(filename));
            myMenu.addItem(filename);
        }
    });

    M5ez::yield();
    myMenu.runOnce();
    M5ez::yield();

//    PRINTV("Selected item:", myMenu.pick());
//    PRINTV("Selected item name:", myMenu.pickName());
//    PRINTV("Selected item caption:", myMenu.pickCaption());
//    PRINTV("Selected item button:", myMenu.pickButton());

    if (myMenu.pickButton() == "Back") {
        String strPath(path);
        const size_t last_slash_idx = strPath.lastIndexOf("/");
        String priorPath = strPath.substring(0, last_slash_idx);
        if (priorPath == "/" || priorPath == "" || priorPath == "/sd") {
            return;
        }

        M5ez::yield();

        return showBrowse(priorPath.c_str());
    }

    if (myMenu.pickButton() == "Home") {
        goHome = true;
        return;
    }

    if (strncmp(myMenu.pickName().c_str(), "./", 2) == 0) {
        strncpy(filename, path, sizeof(filename));
        strncat(filename, &myMenu.pickName().c_str()[1], (sizeof(filename) - strlen(filename) - 1));
        M5ez::yield();
        return showBrowse(filename);
    } else {
        strncpy(filename, path, sizeof(filename));
        M5ez::yield();
        strncat(filename, "/", (sizeof(filename) - strlen(filename) - 1));
        M5ez::yield();
        strncat(filename, myMenu.pickName().c_str(), (sizeof(filename) - strlen(filename) - 1));
        M5ez::yield();
        PRINTV("File: ", filename);

        selectFile(filename);
    }
}

void showBrowse() {
    goHome = false;
    PRINTV("Last path:", lastPath);
    return showBrowse(lastPath);
}

void showSearch() {
    String searchText = M5ez::textInput("Search for...");
    searchText.trim();
    if (searchText == "") {
        return;
    }

    ezMenu myMenu("Search");
    myMenu.buttons("up#Back#select##down#");
    myMenu.txtSmall();

    printHeapUsage();
    bool searchResult = AmiiboDBAO::findAmiibosByFileNameMatch(searchText.c_str(), [&myMenu](AmiiboRecord& amiibo) {
        auto item = new String(amiibo.id);
        *item = *item + "|" + amiibo.name;
        myMenu.addItem(*item);
    });

    int16_t menuResult = myMenu.runOnce();
    PRINTV("Menu result: ", menuResult);
    PRINTV("Selected item:", myMenu.pick());
    PRINTV("Selected item name:", myMenu.pickName());
    PRINTV("Selected item caption:", myMenu.pickCaption());
    PRINTV("Selected item button:", myMenu.pickButton());

    if (myMenu.pickButton() == "Back") {
        return;
    }

    printHeapUsage();

    AmiiboRecord amiibo;
    int id = myMenu.pickName().toInt();
    bool lookupResult = AmiiboDBAO::findAmiiboById(id, amiibo);

    printHeapUsage();

    PRINTV("Lookup result: ", lookupResult);

    PRINTV("File path: ", amiibo.file);

    selectFile(amiibo.file);
}

void showPN532Diagnostic() {
    ezMenu myMenu("PN532");
    myMenu.txtSmall();

    uint32_t versionData = pn532.getFirmwareVersion();
    if (! versionData) {
        myMenu.addItem("Didn't find PN53x board");
    } else {
        char chipset[30] = "Found chip PN5";
        char firmware[30] = "Firmware ver. ";
        sprintf(&chipset[strlen(chipset)], "%02x", (versionData >> 24) & 0xFF);
        sprintf(&firmware[strlen(firmware)], "%d", (versionData >> 16) & 0xFF);
        sprintf(&firmware[strlen(firmware)], ".%d", (versionData >> 8) & 0xFF);
        myMenu.addItem(chipset);
        myMenu.addItem(firmware);
    }

    myMenu.runOnce();
}

void showNfcDiagnostic() {
    NTag215Generic tag(&pn532);

    int inListRes = tag.inList();

    ezMenu myMenu("NFC Tag");

    if (inListRes == 0) {
        printHexDump(tag.uid, 7, 20);
        myMenu.addItem("Found tag.");
        myMenu.addItem(tag.uidStr);
    } else {
        myMenu.addItem("No NFC Tag Found.");
    }

    myMenu.runOnce();
}

void doWipe() {
    NTag215Magic tag(&pn532);
    tag.reset();
    M5ez::msgBox("AmiiBuddy - Wipe", "Wiped");
}

void showDiagnostic() {
    char menuTitle[20] = "Diagnostics";
    strcat(menuTitle, AMIIBUDDY_VERSION);
    ezMenu myMenu(menuTitle);
    myMenu.buttons("up#Back#select##down#");
    myMenu.addItem("PN532 Chipset", showPN532Diagnostic);
    myMenu.addItem("NFC Tag", showNfcDiagnostic);
    myMenu.addItem("Wipe Tag", doWipe);
    myMenu.runOnce();
}

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
    bool foundAmiibo = AmiiboDBAO::findAmiiboByHash(hashes.amiiboHash, amiibo);
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
            AmiiboDBAO::findSaveById(saveId, save);
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

void selectDataSet() {
    ezMenu myMenu("Select dataset");
    myMenu.buttons("up#Back#select##down#");

    int count = FSTools::getFileCount("/sd/amiibos/test");
    bool usingTestAmiibos = (count == 0);
    char fullAmiiboSet[40] = "";
    char testAmiiboSet[40] = "";
    sprintf(fullAmiiboSet, TMPL_QUAD_STR, "Full", "|", usingTestAmiibos ? "  " : "* ", TEXT_FULL_AMIIBO_SET);
    sprintf(testAmiiboSet, TMPL_QUAD_STR, "Test", "|", usingTestAmiibos ? "* " : "  ", TEXT_TEST_AMIIBO_SET);

    myMenu.addItem(fullAmiiboSet);
    myMenu.addItem(testAmiiboSet);

    myMenu.runOnce();

    if (myMenu.pickName() == "Back") {
        return;
    }

    printHeapUsage();
    if (myMenu.pickName() == "Full" && usingTestAmiibos) {
        FSTools::rename(LIBRARY_PATH, "/sd/amiibos/test/library");
        FSTools::rename(POWER_SAVES_PATH, "/sd/amiibos/test/powersaves");
        FSTools::rename("/sd/amiibos/full/library", LIBRARY_PATH);
        FSTools::rename("/sd/amiibos/full/powersaves", POWER_SAVES_PATH);
        String msg(TMPL_SWITCHED_AMIIBO_SET);
        msg = msg + TEXT_FULL_AMIIBO_SET;
        M5ez::msgBox(TEXT_COMPLETE, msg, TEXT_OK);
        return;
    }

    if (myMenu.pickName() == "Test" && ! usingTestAmiibos) {
        FSTools::rename(LIBRARY_PATH, "/sd/amiibos/full/library");
        FSTools::rename(POWER_SAVES_PATH, "/sd/amiibos/full/powersaves");
        FSTools::rename("/sd/amiibos/test/library", LIBRARY_PATH);
        FSTools::rename("/sd/amiibos/test/powersaves", POWER_SAVES_PATH);
        String msg(TMPL_SWITCHED_AMIIBO_SET);
        msg = msg + TEXT_TEST_AMIIBO_SET;
        M5ez::msgBox(TEXT_COMPLETE, msg, TEXT_OK);
        return;
    }
    printHeapUsage();

}

bool initialized = false;

void showMainMenu() {
    if (initialized == false) {
        ezSettings::menuObj.addItem("Select target tag type", selectTargetTagType);
        ezSettings::menuObj.addItem("Select data set", selectDataSet);
        initialized = true;
    }
    ezMenu myMenu("AmiiBuddy");
    myMenu.addItem("Browse", showBrowse);
    myMenu.addItem("Search", showSearch);
    myMenu.addItem("Read", doRead);

    if (PN532_PRESENT) {
        myMenu.addItem("Diagnostic", showDiagnostic);

    }
    myMenu.addItem("Settings", ezSettings::menu);
    myMenu.run();
}
