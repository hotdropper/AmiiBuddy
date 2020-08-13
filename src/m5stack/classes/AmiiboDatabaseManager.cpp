//
// Created by hotdropper on 7/27/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <Arduino.h>
#include "AmiiboDatabaseManager.h"
#include <string>
#include <list>
#include <M5ez.h>
#include <MD5Builder.h>
#include "AmiiBuddy.h"
#include <FSTools.h>
#include "../utils.h"
#include <sqlite3.h>
#include <AmiiboDBAO.h>
#include "amiibuddy_constants.h"

#ifndef SQLITE_DB_PATH
#define SQLITE_DB_PATH "/sd/db/amiibos.sqlite"
#endif
#ifndef FS_DB_PATH
#define FS_DB_PATH "/db/amiibos.sqlite"
#endif

ezProgressBar* pb;

tag_data_t amiiboData;
DirectoryDetails adbDirDets = {0, 0};
int adbFileCount = 0;
int adbItemCounter = 0;
md5_hash_t adbHashCache = "";
File dirBuff;
int lenBuff;
AmiiboRecord mgrAmiiboBuff;
SaveRecord mgrSaveBuff;
const char* pathBuff;
String fileNameStrBuff;
String nameStrBuff;
String msgBuff;

void fileToAmiiboName(const char* filePath, char* name, size_t nameLen, bool powerSave) {
    fileNameStrBuff = String(filePath);
    nameStrBuff = fileNameStrBuff.substring(fileNameStrBuff.lastIndexOf("/") + 1);
    nameStrBuff = nameStrBuff.substring(0, nameStrBuff.lastIndexOf("."));
    int dashPos = nameStrBuff.indexOf("-");
    if (dashPos > -1) {
        nameStrBuff = nameStrBuff.substring(dashPos + 2);
    }

    if (powerSave) {
        nameStrBuff = nameStrBuff.substring(nameStrBuff.lastIndexOf("[") + 1);
        nameStrBuff = nameStrBuff.substring(nameStrBuff.lastIndexOf("[") + 1);
        nameStrBuff = nameStrBuff.substring(0, nameStrBuff.lastIndexOf("]"));
    } else {
        nameStrBuff = nameStrBuff.substring(0, nameStrBuff.indexOf("["));
        nameStrBuff.trim();
    }

    strlcpy(name, nameStrBuff.c_str(), nameLen);
}

bool loadAmiiboFile(const char* path) {
    if (FSTools::readData(path, amiiboData, NTAG215_SIZE) != 0) {
        msgBuff = String(ERROR_LOAD_FILE);
        msgBuff = msgBuff + path;
        PRINTLN(msgBuff);
        M5ez::msgBox(TEXT_WARNING, msgBuff, TEXT_DISMISS);
        return false;
    }

    // PRINTHEX(amiiboData, NTAG215_SIZE);

    atool.loadKey(KEY_FILE);

    if (! atool.loadFileFromData(amiiboData, NTAG215_SIZE, false)) {
        msgBuff = String(ERROR_INVALID_AMIIBO);
        msgBuff = msgBuff + path;
        PRINTLN(msgBuff);
        M5ez::msgBox(TEXT_WARNING, msgBuff, TEXT_DISMISS);
        return false;
    }

    return true;
}

bool loadAndProcessAmiiboFile(const char* path, AmiiboRecord& amiibo) {
    if (! loadAmiiboFile(path)) {
        PRINTV("Failed to load file: ", path);
        return false;
    }
    M5ez::yield();

    fileToAmiiboName(path, amiibo.name, sizeof(AmiiboRecord::name), false);
    memcpy(amiibo.amiiboName, atool.amiiboInfo.amiiboName, AMIIBO_NAME_LEN*6+1);
    memcpy(amiibo.ownerName, atool.amiiboInfo.amiiboOwnerMiiName, AMIIBO_NAME_LEN*6+1);
    amiibo.characterNumber = atool.amiiboInfo.amiiboCharacterNumber;
    amiibo.variation = atool.amiiboInfo.amiiboVariation;
    amiibo.form = atool.amiiboInfo.amiiboForm;
    amiibo.number = atool.amiiboInfo.amiiboNumber;
    amiibo.set = atool.amiiboInfo.amiiboSet;
    memcpy(amiibo.head, atool.amiiboInfo.amiiboHead, 4);
    memcpy(amiibo.tail, atool.amiiboInfo.amiiboTail, 4);
    AmiiboDBAO::calculateAmiiboInfoHash(atool.amiiboInfo, amiibo.hash);
    strcpy(amiibo.file, path);
    amiibo.last_written = 0;

    return true;
}

void readDirectoryDetails(const char* header, const char* msg, const char* path, DirectoryDetails* dirDetails) {
    adbItemCounter = 0;

    if (pb != nullptr) {
        free(pb);
    }

    pb = new ezProgressBar(header, msg);

    FSTools::readDirectoryDetails(path, adbDirDets, [](File* file) {
        adbItemCounter++;

        if (adbItemCounter % 5 != 0) {
            return;
        }

        if (adbItemCounter > 100) {
            adbItemCounter = 0;
        }

        pb->value(((float) adbItemCounter / (float)100) * 100);
        M5ez::redraw();
        M5ez::yield();
    });
}

void rebuildFileData(const char* library_path) {
    readDirectoryDetails("Scanning", "Scanning your Amiibo library...", library_path, &adbDirDets);
    adbFileCount = adbDirDets.file_count;

    if (pb != nullptr) {
        free(pb);
    }

    pb = new ezProgressBar("Processing...", "Processing your library...");

    PRINTV("Total file count: ", adbFileCount);

    M5ez::yield();

    adbItemCounter = 0;
    M5ez::yield();

    auto incrementProgressValue = []() {
        adbItemCounter++;

        if (adbItemCounter % 2 != 0) {
            return;
        }

        PRINTV("Item counter: ", adbItemCounter);
        PRINTV("File count: ", adbFileCount);
        pb->value(((float)adbItemCounter / (float)adbFileCount) * 100);
        M5ez::redraw();
        M5ez::yield();
    };

    M5ez::yield();
    PRINTLN("____ START ____");
    printHeapUsage();

    FSTools::traverseFiles(library_path, [&incrementProgressValue](File *entry) -> void {
        PRINTV("Processing file: ", entry->name());
        pathBuff = entry->name();
        lenBuff = strlen(pathBuff);

        if (strcasecmp(pathBuff + (lenBuff - 4), ".bin") != 0 || strcasecmp(pathBuff + (lenBuff - 10), "/.ds_store") == 0) {
            PRINTV("skipping file: ", pathBuff);

            M5ez::yield();
            return;
        }

        printHeapUsage();
        loadAndProcessAmiiboFile(pathBuff, mgrAmiiboBuff);
        AmiiboDBAO::insertAmiibo(mgrAmiiboBuff);

        mgrSaveBuff.amiibo_id = mgrAmiiboBuff.id;
//        PRINTV("Save hash:", save.hash);
        AmiiboDBAO::calculateSaveHash(atool.amiiboInfo, mgrSaveBuff.hash);
//        PRINTV("Save hash:", save.hash);
        strcpy(mgrSaveBuff.file, pathBuff);
        fileToAmiiboName(pathBuff, mgrSaveBuff.name, sizeof(SaveRecord::name), false);
        mgrSaveBuff.last_update = 0;
        mgrSaveBuff.is_custom = false;

        AmiiboDBAO::insertSave(mgrSaveBuff);

        incrementProgressValue();

        printHeapUsage();
    });

    PRINTLN("____ END ____");
    printHeapUsage();
}


void rebuildPowerSaveData(const char* power_save_path) {
    readDirectoryDetails("Scanning", "Scanning your Power Save library...", power_save_path, &adbDirDets);
    adbFileCount = adbDirDets.file_count;

    if (pb != nullptr) {
        free(pb);
    }

    pb = new ezProgressBar("Processing...", "Processing power saves...");

    PRINTV("Total file count: ", adbFileCount);

    adbItemCounter = 0;
    M5ez::yield();

    auto incrementProgressValue = []() {
        adbItemCounter++;

        if (adbItemCounter % 2 != 0) {
            return;
        }

        PRINTV("Item counter: ", adbItemCounter);
        PRINTV("File count: ", adbFileCount);
        pb->value(((float)adbItemCounter / (float)adbFileCount) * 100);
        M5ez::redraw();
        M5ez::yield();
    };

    M5ez::yield();

    PRINTLN("____ START ____");
    printHeapUsage();

    FSTools::traverseFiles(power_save_path, [&incrementProgressValue](File *entry) -> void {
        if (! loadAmiiboFile(entry->name())) {
            return;
        }

        M5ez::yield();

        AmiiboDBAO::calculateAmiiboInfoHash(atool.amiiboInfo, adbHashCache);

        mgrSaveBuff.amiibo_id = AmiiboDBAO::findAmiiboIdByHash(adbHashCache);
        AmiiboDBAO::calculateSaveHash(atool.amiiboInfo, mgrSaveBuff.hash);
        fileToAmiiboName(entry->name(), mgrSaveBuff.name, sizeof(SaveRecord::name), true);
        strlcpy(mgrSaveBuff.file, entry->name(), sizeof(SaveRecord::file));
        mgrSaveBuff.last_update = 0;
        mgrSaveBuff.is_custom = false;

        AmiiboDBAO::insertSave(mgrSaveBuff);

        incrementProgressValue();

        printHeapUsage();
    });

    PRINTLN("____ END ____");
    printHeapUsage();
}

void rebuildCustomSaveData(const char* custom_save_path) {
    readDirectoryDetails("Scanning", "Scanning your Custom Save library...", custom_save_path, &adbDirDets);
    adbFileCount = adbDirDets.file_count;

    if (pb != nullptr) {
        free(pb);
    }

    pb = new ezProgressBar("Processing...", "Processing power saves...");

    PRINTV("Total file count: ", adbFileCount);

    adbItemCounter = 0;
    M5ez::yield();

    auto incrementProgressValue = []() {
        adbItemCounter++;

        if (adbItemCounter % 2 != 0) {
            return;
        }

        PRINTV("Item counter: ", adbItemCounter);
        PRINTV("File count: ", adbFileCount);
        pb->value(((float)adbItemCounter / (float)adbFileCount) * 100);
        M5ez::redraw();
        M5ez::yield();
    };

    M5ez::yield();

    PRINTLN("____ START ____");
    printHeapUsage();

    FSTools::traverseFiles(custom_save_path, [&incrementProgressValue](File *entry) -> void {
        if (! loadAmiiboFile(entry->name())) {
            return;
        }

        M5ez::yield();

        AmiiboDBAO::calculateAmiiboInfoHash(atool.amiiboInfo, adbHashCache);

        mgrSaveBuff.amiibo_id = AmiiboDBAO::findAmiiboIdByHash(adbHashCache);
        AmiiboDBAO::calculateSaveHash(atool.amiiboInfo, mgrSaveBuff.hash);
        String saveName = String(entry->name());
        PRINTV("Save name:", saveName);
        // strip first /
        saveName = saveName.substring(saveName.indexOf("/") + 1);
        PRINTV("Save name:", saveName);
        // strip sd/
        saveName = saveName.substring(saveName.indexOf("/") + 1);
        PRINTV("Save name:", saveName);
        // strip <amiibo hash>/
        saveName = saveName.substring(saveName.indexOf("/") + 1);
        PRINTV("Save name:", saveName);
        // strip <save hash>_
        saveName = saveName.substring(saveName.indexOf("_") + 1);
        PRINTV("Save name:", saveName);
        // strip .bin
        saveName = saveName.substring(0, saveName.length() - 4);
        PRINTV("Save name:", saveName);

        strlcpy(mgrSaveBuff.name, saveName.c_str(), sizeof(SaveRecord::name));
        strlcpy(mgrSaveBuff.file, entry->name(), sizeof(SaveRecord::file));
        mgrSaveBuff.last_update = 0;
        mgrSaveBuff.is_custom = true;

        AmiiboDBAO::insertSave(mgrSaveBuff);

        incrementProgressValue();

        printHeapUsage();
    });

    PRINTLN("____ END ____");
    printHeapUsage();
}

bool AmiiboDatabaseManager::initialize(const char* library_path, const char* power_saves_path, const char* custom_saves_path) {
    if (! AmiiboDBAO::truncate()) {
        return false;
    }

    rebuildFileData(library_path);

    rebuildPowerSaveData(power_saves_path);

    rebuildCustomSaveData(custom_saves_path);

    AmiiboDBAO::end();

    ESP.restart();
    return true;
}