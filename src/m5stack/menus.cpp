//
// Created by hotdropper on 7/18/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <M5ez.h>
#include <map>
#include "menus.h"
#include "init.h"
#include "firmware.h"
#include "fs_tools.h"
#include "utils.h"
#include "classes/AmiiboDatabase.h"
#include "classes/AmiiBuddy.h"
#include "classes/NFCMonitor.h"
#include "classes/MagicNTag215.h"

int selectFile(const char* filename) {
    if (! PN532_PRESENT) {
        PRINTLN("PN532 not present.");
        return -30;
    }

    auto pb = new ezProgressBar("Writing...", "Writing amiibo to tag...");

    MagicNTag215 tag;
    readData(filename, tag.data, NTAG215_SIZE);

    int burn = nfcMonitor.monitorAmiiboWrite(pb, &tag);

    PRINT("Got burn result: ");
    PRINTLN(burn);

    return burn;
}

void showBrowse(const char *path) {
    PRINTLN(path);

    ezMenu myMenu("Browse");
    myMenu.txtSmall();
    myMenu.buttons("up#Back#select##down#");
    char filename[256];

    File dir = AMIIBUDDY_FS_DEFAULT.open(path);
    if (! dir) {
        char msg[80] = "Could not find: ";
        strcat(msg, path);
        M5ez::msgBox("Error", msg);
        return;
    }

    File entry;
    while ((entry = dir.openNextFile())) {
        if (entry.isDirectory()) {
            strncpy(filename, "./", sizeof(filename));
            strncat(filename, &entry.name()[strlen(path) + 1], (sizeof(filename) - strlen(filename)));
            myMenu.addItem(filename);
        } else {
            if (strncmp(&entry.name()[strlen(path) + 1], ".DS_Store", strlen(".DS_Store")) == 0) {
                continue;
            }
            strncpy(filename, &entry.name()[strlen(path) + 1], sizeof(filename));
            myMenu.addItem(filename);
        }
        entry.close();
    }
    dir.close();
    myMenu.runOnce();

//    PRINTV("Selected item:", myMenu.pick());
//    PRINTV("Selected item name:", myMenu.pickName());
//    PRINTV("Selected item caption:", myMenu.pickCaption());
//    PRINTV("Selected item button:", myMenu.pickButton());

    if (myMenu.pickButton() == "Back") {
        String strPath(path);
        const size_t last_slash_idx = strPath.lastIndexOf("/");
        String lastPath = strPath.substring(0, last_slash_idx);
        if (lastPath == "/" || lastPath == "") {
            return;
        }

        return showBrowse(lastPath.c_str());
    }

    if (strncmp(myMenu.pickName().c_str(), "./", 2) == 0) {
        strncpy(filename, path, sizeof(filename));
        strncat(filename, &myMenu.pickName().c_str()[1], (sizeof(filename) - strlen(filename)));
        showBrowse(filename);
    } else {
        strncpy(filename, path, sizeof(filename));
        strncat(filename, "/", (sizeof(filename) - strlen(filename)));
        strncat(filename, myMenu.pickName().c_str(), (sizeof(filename) - strlen(filename)));
        PRINT("File: ");
        PRINTLN(filename);

        selectFile(filename);

        showBrowse(path);
    }
}

void showBrowse() {
    PRINTLN(LIBRARY_PATH);
    showBrowse(LIBRARY_PATH);
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
    int searchResult = amiiboDatabase.search(searchText.c_str(), [&myMenu](String& k, String& v) {
        PRINTV("Result pair key: ", k);
        PRINTV("Result pair val: ", v);
        myMenu.addItem(k + "|" + v);
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

    String* filePath;
    int lookupResult = amiiboDatabase.lookupAmiibo(myMenu.pickName().c_str(), [&filePath](String& k, String& v) {
        if (k == "file_path") {
            filePath = new String(v);
        }
    });

    printHeapUsage();

    PRINTV("Lookup result: ", lookupResult);

    PRINTV("File path: ", *filePath);

    selectFile(filePath->c_str());
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

    uint8_t saveChecksum[AMIIBO_SAVE_CHECKSUM_LEN];
    AmiiboDatabaseManager::getSaveChecksum(tag.data, saveChecksum);

    std::map<String, String> amiiboInfo;
    int lookupResult = amiiboDatabase.findByData(tag.data, [&amiiboInfo](String& k, String& v) {
        amiiboInfo[*(new String(k.c_str()))] = *(new String(v.c_str()));
    });

    if (lookupResult == -1) {
        M5ez::msgBox("Error", "This does not seem to be an Amiibo we recognize.");
        return;
    } else if (lookupResult == -2) {
        M5ez::msgBox("Error", "Filesystem error. Please try again.");
        return;
    }

    String filePath = amiiboInfo.at("file_path");
    String hash = amiiboInfo.at("hash");
    String name = filePath.substring(filePath.lastIndexOf("/") + 1);
    M5ez::msgBox("Success", "We would recognize " + name + " anywhere!");
}

void showMainMenu() {
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
