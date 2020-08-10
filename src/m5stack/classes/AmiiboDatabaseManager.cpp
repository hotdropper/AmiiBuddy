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

void fileToAmiiboName(const char* filePath, char* name, size_t nameLen, bool powerSave) {
    String file(filePath);
    String nameStr = file.substring(file.lastIndexOf("/") + 1);
    nameStr = nameStr.substring(0, nameStr.lastIndexOf("."));
    int dashPos = nameStr.indexOf("-");
    if (dashPos > -1) {
        nameStr = nameStr.substring(dashPos + 2);
    }

    if (powerSave) {
        nameStr = nameStr.substring(nameStr.lastIndexOf("[") + 1);
        nameStr = nameStr.substring(nameStr.lastIndexOf("[") + 1);
        nameStr = nameStr.substring(0, nameStr.lastIndexOf("]"));
    } else {
        nameStr = nameStr.substring(0, nameStr.indexOf("["));
        nameStr.trim();
    }

    strlcpy(name, nameStr.c_str(), nameLen);
}

bool loadAmiiboFile(const char* path) {
    if (FSTools::readData(path, amiiboData, NTAG215_SIZE) != 0) {
        String msg(ERROR_LOAD_FILE);
        msg = msg + path;
        PRINTLN(msg);
        M5ez::msgBox(TEXT_WARNING, msg, TEXT_DISMISS);
        return false;
    }

    // PRINTHEX(amiiboData, NTAG215_SIZE);

    atool.loadKey(KEY_FILE);

    if (! atool.loadFileFromData(amiiboData, NTAG215_SIZE, false)) {
        String msg(ERROR_INVALID_AMIIBO);
        msg = msg + path;
        PRINTLN(msg);
        M5ez::msgBox(TEXT_WARNING, msg, TEXT_DISMISS);
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
    File dir;
    FSTools::open(library_path, &dir);
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
    AmiiboRecord amiibo;
    SaveRecord save;

    FSTools::traverseFiles(&dir, [&incrementProgressValue, &amiibo, &save](File *entry) -> void {
        PRINTV("Processing file: ", entry->name());
        const char* path = entry->name();
        int len = strlen(path);

        if (strcasecmp(path + (len - 4), ".bin") != 0 || strcasecmp(path + (len - 10), "/.ds_store") == 0) {
            PRINTV("skipping file: ", path);

            M5ez::yield();
            return;
        }

        printHeapUsage();
        loadAndProcessAmiiboFile(path, amiibo);
        AmiiboDBAO::insertAmiibo(amiibo);

        save.amiibo_id = amiibo.id;
//        PRINTV("Save hash:", save.hash);
        AmiiboDBAO::calculateSaveHash(atool.original, save.hash);
//        PRINTV("Save hash:", save.hash);
        strcpy(save.file, path);
        fileToAmiiboName(path, save.name, sizeof(SaveRecord::name), false);
        save.last_update = now();
        save.is_custom = false;

        AmiiboDBAO::insertSave(save);

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
    File dir;
    FSTools::open(power_save_path, &dir);
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
    SaveRecord save;

    FSTools::traverseFiles(&dir, [&incrementProgressValue, &save](File *entry) -> void {
        if (! loadAmiiboFile(entry->name())) {
            return;
        }

        M5ez::yield();

        AmiiboDBAO::calculateAmiiboInfoHash(atool.amiiboInfo, adbHashCache);

        save.amiibo_id = AmiiboDBAO::findAmiiboIdByHash(adbHashCache);
        AmiiboDBAO::calculateSaveHash(atool.original, save.hash);
        fileToAmiiboName(entry->name(), save.name, sizeof(SaveRecord::name), true);
        strlcpy(save.file, entry->name(), sizeof(SaveRecord::file));
        save.last_update = now();
        save.is_custom = false;

        AmiiboDBAO::insertSave(save);

        incrementProgressValue();

        printHeapUsage();
    });

    PRINTLN("____ END ____");
    printHeapUsage();
}

bool AmiiboDatabaseManager::initialize(const char* library_path, const char* power_saves_path) {
    if (! AmiiboDBAO::truncate()) {
        return false;
    }

    rebuildFileData(library_path);

    rebuildPowerSaveData(power_saves_path);

    ESP.restart();
    return true;
}