//
// Created by hotdropper on 7/18/20.
//

#define PRINT_DEBUG 1

#include "ArduinoDebug.h"
#include "AmiiboDatabase.h"
#include <M5ez.h>
#include <functional>
#include <SD.h>
#include <MD5Builder.h>
#include <amiitool.h>
#include "NTag215.h"
#include "AmiiBuddy.h"
#include "m5stack/utils.h"
#include <FSTools.h>
#include <AmiiboDBAO.h>


AmiiboDatabase amiiboDatabase;

DirectoryDetails dataDetails = {0, 0 };

int AmiiboDatabase::lookupSave(const char* amiibo_hash, const char* save_hash, const search_callback_t& callback) {
    String filePath = String(AMIIBO_SAVE_FILE_PATH) + "/" + amiibo_hash + ".json";

    PRINTV("Looking up file: ", filePath);

    char fsBuff[FSTOOLS_MAX_PATH_LENGTH] = "";

    File f;
    FSTools::openForRead(filePath.c_str(), &f);

    if (! f) {
        PRINT("Could not open file.");
        return -2;
    }

//    AmiiboDataDoc jsonDoc;
//    deserializeJson(jsonDoc, f);
//
    f.close();
//
//    serializeJsonPretty(jsonDoc, Serial);
//
//    JsonObject obj = jsonDoc.as<JsonObject>();
//
//    if (callback != nullptr) {
//        for (auto pair : obj) {
//            if (strcmp(pair.key().c_str(), save_hash) != 0) {
//                continue;
//            }
//            String k(pair.key().c_str());
//            String v = pair.value().as<String>();
//            callback(k, v);
//            return 1;
//        }
//    }

    return 0;
}

int lookupSaves(const char* filePath, const search_callback_t& callback) {
    PRINTV("Looking up file: ", filePath);

    File f;
    FSTools::openForRead(filePath, &f);

    if (! f) {
        PRINT("Could not open file.");
        return -2;
    }

//    AmiiboDataDoc jsonDoc;
//    deserializeJson(jsonDoc, f);

    f.close();

//    serializeJsonPretty(jsonDoc, Serial);
//
//    JsonObject obj = jsonDoc.as<JsonObject>();
//
//    if (callback != nullptr) {
//        for (auto pair : obj) {
//            String k(pair.key().c_str());
//            String v = pair.value().as<String>();
//            callback(k, v);
//        }
//    }

    return 0;
}


int AmiiboDatabase::lookupSaves(const char* amiibo_hash, const bool onlyUserSaves, const search_callback_t& callback) {
    String filePath = String(AMIIBO_SAVE_FILE_PATH) + "/" + amiibo_hash + ".json";
    String userFilePath = String(AMIIBO_SAVE_FILE_PATH) + "/" + amiibo_hash + ".user.json";

    int ret = ::lookupSaves(userFilePath.c_str(), callback);
    if (ret == -2) {
        return ret;
    }

    return onlyUserSaves ? ret : ::lookupSaves(filePath.c_str(), callback);
}

int AmiiboDatabase::lookupAmiibo(const char* id, const search_callback_t& callback) {
    String filePath = String(AMIIBO_DATA_FILE_PATH) + "/" + id + ".json";

    PRINTV("Looking up file: ", filePath);

    File f;
    FSTools::openForRead(filePath.c_str(), &f);

    if (! f) {
        PRINT("Could not open file.");
        return -2;
    }

//    AmiiboDataDoc jsonDoc;
//    deserializeJson(jsonDoc, f);

    f.close();

//    serializeJsonPretty(jsonDoc, Serial);
//
//    JsonObject obj = jsonDoc.as<JsonObject>();
//
//    if (callback != nullptr) {
//        for (auto pair : obj) {
//            String k(pair.key().c_str());
//            String v = pair.value().as<String>();
//            callback(k, v);
//        }
//    }

    return 0;
}

int AmiiboDatabase::search(const char* text, const search_callback_t& callback) {

//    JsonObject db = fullRef.as<JsonObject>();
//    String searchText = String(text);
//    searchText.toLowerCase();
//
//    printHeapUsage();
//
//    if (callback != nullptr) {
//        for (auto pair : db) {
//            String k(pair.key().c_str());
//            String v = pair.value().as<String>();
//            v.toLowerCase();
//            if (v.indexOf(text) > -1) {
//                String r = pair.value().as<String>();
//                PRINTV("Found key:", k);
//                r = r.substring(r.lastIndexOf("/") + 1);
//                PRINTV("Found value:", r);
//                callback(k, r);
//                PRINTLN("Inserted");
//            }
//        }
//    }

    printHeapUsage();
    return 0;
}

int AmiiboDatabase::findByData(const uint8_t* data, const search_callback_t& callback) {
    atool.loadKey(KEY_FILE);
    atool.loadFileFromData(data, NTAG215_SIZE, false);

    char hash[33] = "";
    AmiiboDBAO::calculateAmiiboInfoHash(atool.amiiboInfo, hash);
    PRINTV("Hash: ", hash);
    String filePath = String(AMIIBO_DATA_FILE_PATH) + "/" + hash + ".json";

    PRINTV("Looking up file: ", filePath);

//    char saveChecksum[AMIIBO_SAVE_CHECKSUM_LEN];
//    AmiiboDatabaseManager::calculateSaveHash(data, saveChecksum);

    File f;
    FSTools::openForRead(filePath.c_str(), &f);

    if (! f) {
        PRINT("Could not open file.");
        return -2;
    }

//    AmiiboDataDoc jsonDoc;
//    deserializeJson(jsonDoc, f);

    f.close();

//    serializeJsonPretty(jsonDoc, Serial);

//    JsonObject obj = jsonDoc.as<JsonObject>();
//
//    if (callback != nullptr) {
//        for (auto pair : obj) {
//            String key = String(pair.key().c_str());
//            String val = pair.value().as<String>();
//            callback(key, val);
//        }
//    }

    return 0;
}

int AmiiboDatabase::writeSave(const char* amiibo_hash, const char* save_hash, const char* save_name, const uint8_t* data) {
    String dataPath = String(SAVES_PATH) + "/" + amiibo_hash + "/" + save_hash + ".bin";

    int writeResult = FSTools::writeData(dataPath.c_str(), data, NTAG215_SIZE);
    if (writeResult < 0) {
        return writeResult;
    }

    String path = String(AMIIBO_SAVE_FILE_PATH) + "/" + amiibo_hash + ".user.json";

//    DynamicJsonDocument jsonDoc(AMIIBO_SEARCH_FILE_SIZE);
//    File f = AMIIBUDDY_FS_DEFAULT.open(path, FILE_READ);
//
//    if (! f) {
//        return -2;
//    }
//
//    deserializeJson(jsonDoc, f);
//    f.close();
//
//    std::map<String, String> newFile;
//    newFile[save_hash] = path + "|" + save_name;
//
//    JsonObject obj = jsonDoc.as<JsonObject>();
//    for (auto pair : obj) {
//        newFile[pair.key().c_str()] = pair.value().as<String>();
//    }
//
//    jsonDoc.clear();
//
//    for (const auto& pair : newFile) {
//        jsonDoc[pair.first] = pair.second;
//    }
//
//    File* fr;
//    openForWrite(path.c_str(), fr);
//
//    if (! *fr) {
//        return -2;
//    }
//
//    serializeJson(jsonDoc, *fr);
//
//    fr->close();

    return 0;
}

int AmiiboDatabase::removeSave(const char* amiibo_hash, const char* save_hash) {
    String path = String(SAVES_PATH) + "/" + amiibo_hash + "/" + save_hash + ".bin";

    FSTools::remove(path.c_str());

    path = String(AMIIBO_SAVE_FILE_PATH) + "/" + amiibo_hash + ".user.json";

//    DynamicJsonDocument jsonDoc(AMIIBO_SEARCH_FILE_SIZE);
//    File f = AMIIBUDDY_FS_DEFAULT.open(path, FILE_READ);
//
//    if (! f) {
//        return -2;
//    }
//
//    deserializeJson(jsonDoc, f);
//    f.close();
//
//    jsonDoc.remove(save_hash);
//
//    File* fr;
//    openForWrite(path.c_str(), fr);
//
//    if (! *fr) {
//        return -2;
//    }
//
//    serializeJson(jsonDoc, *fr);
//
//    fr->close();

    return 0;
}


AmiiboDatabase::AmiiboDatabase(const char* libraryPath, const char* dataPath) {
    _library_path = libraryPath;
    _data_path = dataPath;
}

void initializeDatabase() {
    AmiiboDatabaseManager::initialize();
}

bool AmiiboDatabase::begin() {
    if (! AmiiboDBAO::begin()) {
        M5ez::msgBox(TEXT_WARNING, "Could not initialize the database.", TEXT_DISMISS);
        return false;
    }

    ezSettings::menuObj.addItem("Re-initialize database", initializeDatabase);
    return true;
}


//void rebuildSearchIndex(const char* amiibo_data_path) {
//    uint8_t amiiboData[NTAG215_SIZE] = { 0 };
//
//    readDirectoryDetails(amiibo_data_path, libraryDetails);
//
//    ezProgressBar pb ("Processing...", "Building search index...");
//
//    int fileCount = libraryDetails.file_count;
//
//    PRINTV("Total file count: ", fileCount);
//
//    int itemCounter = 0;
//    File dir = AMIIBUDDY_FS_DEFAULT.open(amiibo_data_path);
//    M5ez::yield();
//
//    auto incrementProgressValue = [&itemCounter, &pb, fileCount]() {
//        itemCounter++;
//
//        if (itemCounter % 2 != 0) {
//            return;
//        }
//
//        PRINTV("Item counter: ", itemCounter);
//        PRINTV("File count: ", fileCount);
//        pb.value(((float)itemCounter / (float)fileCount) * 100);
//        M5ez::redraw();
//        M5ez::yield();
//    };
//
//    StaticJsonDocument<AMIIBO_DATA_FILE_SIZE> jsonDoc;
//    DynamicJsonDocument fullRef(AMIIBO_SEARCH_FILE_SIZE);
//    M5ez::yield();
//
//    printHeapUsage();
//
//    traverseFiles(&dir, [&incrementProgressValue, &jsonDoc, &fullRef](File *entry) -> void {
//        String filePath(entry->name());
//        PRINTV("Processing file: ", filePath);
//
//        File dataFile = AMIIBUDDY_FS_DEFAULT.open(filePath, FILE_READ);
//
//        deserializeJson(jsonDoc, dataFile);
//        dataFile.close();
//
//        PRINTLN("File read:");
//        serializeJsonPretty(jsonDoc, dataFile);
//
//        M5ez::yield();
//
//        auto key = new String(jsonDoc["hash"].as<String>());
//        auto value = new String(jsonDoc["file_path"].as<String>());
//
//        fullRef[*key] = *value;
//        jsonDoc.clear();
//
//        incrementProgressValue();
//
//        printHeapUsage();
//    });
//
//    printHeapUsage();
//    writeJsonFile(AMIIBO_SEARCH_FILE_PATH, fullRef);
//    fullRef.clear();
//    printHeapUsage();
//}
