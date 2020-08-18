//
// Created by Jacob Mather on 8/17/20.
//

#include "write.h"
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
#include "../classes/NTag215Reader.h"

AmiiboRecord menuAmiiboBuff;

void writeFileToTag(String& fileToLoad) {
    NTag215* tag = NTag215Reader::getTag();
    if (tag == nullptr || tag->inList() != 0) {
        M5ez::msgBox(TEXT_WARNING, "Could not find a tag to write to.", TEXT_OK);
        return;
    }

    int readResp = FSTools::readData(fileToLoad.c_str(), tag->data, NTAG215_SIZE);
    if (readResp < 0) {
        PRINTV("Got read result:", readResp);
        String msg = String("We were unable to read the file: ") + fileToLoad;
        M5ez::msgBox(TEXT_ERROR, msg, TEXT_OK);
        return;
    }

    auto pb = new ezProgressBar("Writing...", "Writing amiibo to tag...");

    atool.loadKey(KEY_FILE);
    atool.loadFileFromData(tag->data, NTAG215_SIZE, false);
    atool.encryptLoadedFile(tag->uid);
    memcpy(tag->data, atool.original, NTAG215_SIZE);

    int burn = nfcMonitor.monitorAmiiboWrite(pb, tag);

    free(tag);

    PRINT("Got burn result: ");
    PRINTLN(burn);
}

int selectVariant(const char* filename, String& fileToLoad) {
    uint8_t tagData[540];

    FSTools::readData(filename, tagData, NTAG215_SIZE);
    HashInfo hashes;
    AmiiboDBAO::calculateHashes(tagData, hashes);
    AmiiboDBAO::getAmiiboByHash(hashes.amiiboHash, menuAmiiboBuff);

    ezMenu myMenu("Select save?");
    myMenu.buttons("up#Cancel#select##down#");
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

    if (saveCount > 0) {
        myMenu.runOnce();

        if (myMenu.pickButton() == "Cancel") {
            PRINTLN("Cancelled.")
            return -1;
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

    return 0;
}

void selectFile(const char* filename) {
    if (! PN532_PRESENT) {
        PRINTLN("PN532 not present.");
        return;
    }
    M5ez::yield();

    String fileToLoad = "";

    if (selectVariant(filename, fileToLoad) != 0) {
        return;
    }

    writeFileToTag(fileToLoad);
}
