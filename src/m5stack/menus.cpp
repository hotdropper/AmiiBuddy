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
#include "menus.h"
#include "init.h"
#include "firmware.h"
#include "FSTools.h"
#include "utils.h"
#include "classes/AmiiboDatabase.h"
#include "classes/AmiiBuddy.h"
#include "classes/NFCMonitor.h"
#include "classes/MagicNTag215.h"

bool goHome = false;
char adjustedPath[200] = "";
char lastPath[200] = LIBRARY_PATH;

void selectFile(const char* filename) {
    if (! PN532_PRESENT) {
        PRINTLN("PN532 not present.");
        return;
    }
    M5ez::yield();

    ezMenu myMenu("Select save?");
    myMenu.buttons("up#Back#select##down#Home");

    myMenu.addItem("Original|Original Amiibo");
    PRINTLN("Searching");
    printHeapUsage();
    int saveCount = AmiiboDBAO::findSavesByAmiiboFileName(filename, [&myMenu](SaveRecord& record) {
        M5ez::yield();
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
        } else {
            String k = myMenu.pickName();
            int id = k.substring(0, k.indexOf("_")).toInt();
            fileToLoad = k.substring(k.indexOf("_") + 1);
            AmiiboDBAO::updateSaveTimestamp(id, now());
        }
    } else {
        fileToLoad = filename;
    }

    auto pb = new ezProgressBar("Writing...", "Writing amiibo to tag...");

    PRINTV("File to load:", fileToLoad);

    MagicNTag215 tag;
    FSTools::readData(fileToLoad.c_str(), tag.data, NTAG215_SIZE);

    atool.loadKey(KEY_FILE);
    int burn = nfcMonitor.monitorAmiiboWrite(pb, &tag);

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

    File dir;
    FSTools::open(path, &dir);

    if (! dir) {
        char msg[80] = "Could not find: ";
        strcat(msg, path);
        M5ez::msgBox("Error", msg);
        return;
    }

    int counter = -15;

    FSTools::traverseEntries(&dir, false, [&counter, &pb, &filename, &myMenu, &path](File* entry) {
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

    dir.close();
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
    NTag215 tag(&pn532);

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
    MagicNTag215 tag(&pn532);
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

    NTag215 tag;
    int readResult = nfcMonitor.monitorAmiiboRead(pb, &tag);

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
            if (btn == "Ok") {
                return doRead();
            }
            return;
        default:
            M5ez::msgBox("Error", "Unknown error occurred.");
            return;
    }

    PRINTLN("Tag Data");
    PRINTHEX(tag.data, NTAG215_SIZE);

    amiiBuddy.loadKey();
    char amiiboHash[AMIIBO_HASH_LEN];
    char saveHash[AMIIBO_HASH_LEN];
    AmiiboDBAO::calculateSaveHash(tag.data, saveHash);
    AmiiboDBAO::calculateAmiiboInfoHash(tag.data, amiiboHash);

    printAmiibo(atool.amiiboInfo);
    PRINTV("Amiibo Hash: ", amiiboHash);
    PRINTV("Save Hash: ", saveHash);
    AmiiboRecord amiibo;
    bool foundAmiibo = AmiiboDBAO::findAmiiboByHash(amiiboHash, amiibo);
    PRINTV("Found amiibo?", foundAmiibo);

    ezMenu myMenu("Save Amiibo");
    myMenu.txtSmall();
    myMenu.buttons("up#Cancel#Select##down#New");
    bool sawSaveHash = false;
    int customSaveCount = 0;
    int saveLookupResult = AmiiboDBAO::findSavesByAmiiboHash(amiiboHash, [&myMenu, &sawSaveHash, &saveHash, &customSaveCount](SaveRecord& save) {
        if (save.hash == saveHash) {
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
        return;
    } else if (saveLookupResult < 1) {
        M5ez::msgBox("Error", "We must have misread the tag. Please try again.");
        return;
    }

    if (sawSaveHash) {
        String msg("We would recognize ");
        msg = msg + amiibo.name + " anywhere! We found no changes.";
        M5ez::msgBox(TEXT_SUCCESS, msg, TEXT_OK);
        return;
    }

    if (sawSaveHash == false) {
        // new save
        String msg = String("It looks like ");
        msg = msg + amiibo.name + " has new data. Save it?";

        String button = M5ez::msgBox("Success", msg, "Cancel##Save");

        if (button == "Cancel") {
            return;
        }
    } else if (customSaveCount > 0) {
        myMenu.runOnce();

        PRINTV("Selected item name:", myMenu.pickName());
        PRINTV("Selected item caption:", myMenu.pickCaption());
        PRINTV("Selected item button:", myMenu.pickButton());

        if (myMenu.pickButton() == "Cancel") {
            return;
        }
        if (myMenu.pickButton() == "Select") {
            String value = myMenu.pickName();
            int saveId = value.substring(0, value.indexOf("_")).toInt();
            String saveFile = value.substring(value.lastIndexOf("_") + 1);
            FSTools::remove(saveFile.c_str());
            FSTools::writeData(saveFile.c_str(), tag.data, NTAG215_SIZE);
            AmiiboDBAO::updateSaveTimestamp(saveId, now());
            String msg("We saved ");
            msg = msg + amiibo.name + " to " + myMenu.pickButton() + ".";
            M5ez::msgBox(TEXT_SUCCESS, msg, TEXT_OK);
            return;
        }
    }

    // save new
    String saveName = M5ez::textInput("Save name?");
    String saveFilePath = String(SAVES_PATH "/");
    saveFilePath = saveFilePath + amiibo.hash + "/" + saveHash + "_" + saveName + ".bin";
    FSTools::writeData(saveFilePath.c_str(), tag.data, NTAG215_SIZE);
    SaveRecord newSave = SaveRecord();
    newSave.amiibo_id = amiibo.id;
    strcpy(newSave.hash, saveHash);
    strcpy(newSave.name, saveName.c_str());
    strcpy(newSave.file, saveFilePath.c_str());
    newSave.last_update = now();
    newSave.is_custom = true;
    AmiiboDBAO::insertSave(newSave);
    String msg("We saved ");
    msg = msg + amiibo.name + " to " + myMenu.pickButton() + ".";
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
        msg = msg + TEXT_TEST_AMIIBO_SET;
        M5ez::msgBox(TEXT_COMPLETE, msg, TEXT_OK);
        return;
    }

    if (myMenu.pickName() == "Test" && ! usingTestAmiibos) {
        FSTools::rename(LIBRARY_PATH, "/sd/amiibos/full/library");
        FSTools::rename(POWER_SAVES_PATH, "/sd/amiibos/full/powersaves");
        FSTools::rename("/sd/amiibos/test/library", LIBRARY_PATH);
        FSTools::rename("/sd/amiibos/test/powersaves", POWER_SAVES_PATH);
        String msg(TMPL_SWITCHED_AMIIBO_SET);
        msg = msg + TEXT_FULL_AMIIBO_SET;
        M5ez::msgBox(TEXT_COMPLETE, msg, TEXT_OK);
        return;
    }
    printHeapUsage();

}

bool initialized = false;

void showMainMenu() {
    if (initialized == false) {
        ezSettings::menuObj.addItem("Select Data Set", selectDataSet);
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
