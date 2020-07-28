//
// Created by hotdropper on 7/27/20.
//

#include <ArduinoDebug.h>
#include "AmiiboDatabaseManager.h"
#include <string>
#include <list>
#include <M5ez.h>
#include <MD5Builder.h>
#include "AmiiBuddy.h"
#include "../fs_tools.h"
#include "../utils.h"

DirectoryDetails directoryDetails = {0, 0 };
MD5Builder md5Builder;

struct DirectoryComparator
{
    bool operator ()(String & str1, String & str2)
    {
        if(str1.length() == str2.length())
            return str2.compareTo(str1);
        return str2.length() < str1.length();
    }
};

void AmiiboDatabaseManager::empty(const char* data_path) {
    if (! AMIIBUDDY_FS_DEFAULT.exists(data_path)) {
        return;
    }

    PRINTLN("Reading library details.");

    size_t total = readDirectoryDetails(data_path, directoryDetails);
    PRINTLN("Got library details.");
    PRINTV("Files: ", directoryDetails.file_count);
    PRINTV("Directories: ", directoryDetails.dir_count);
    size_t count = 0;

    ezProgressBar pb ("Cleaning...", "Removing files...");
    M5ez::redraw();
    M5ez::yield();

    auto incrementProgress = [&total, &count, &pb]() {
        count++;
        if (count % 2 != 0) {
            return;
        }
        float progress = (float)count / (float)total * 100;
        pb.value(progress);
        M5ez::redraw();
        M5ez::yield();
    };

    File dir = AMIIBUDDY_FS_DEFAULT.open(data_path);
    std::list<String> directories = {};
    traverseEntries(&dir, [&directories, &incrementProgress](File *entry) -> void {
        if (entry->isDirectory()) {
            directories.push_front(String(entry->name()));
            return;
        }
        PRINTV("Removing file: ", entry->name());
        AMIIBUDDY_FS_DEFAULT.remove(entry->name());
        incrementProgress();
    });

    directories.unique();
    directories.sort(DirectoryComparator());

    for (auto & directory : directories) {
        PRINTV("Remove directory: ", directory);
        AMIIBUDDY_FS_DEFAULT.rmdir(directory);
        incrementProgress();
    }
}

void AmiiboDatabaseManager::getSaveChecksum(const uint8_t* data, uint8_t* checksum) {
    memcpy(checksum, &data[AMIIBO_SAVE_CHECKSUM_POS], AMIIBO_SAVE_CHECKSUM_LEN);
//    PRINTLN("Save Checksum:");
//    PRINTHEX(checksum, AMIIBO_SAVE_CHECKSUM_LEN);
}

void AmiiboDatabaseManager::calculateAmiiboInfoHash(const uint8_t* data, char* hash) {
    amiiBuddy.loadKey();
    atool.loadFileFromData(data, NTAG215_SIZE, false);
    AmiiboDatabaseManager::calculateAmiiboInfoHash(&atool.amiiboInfo, hash);
}

void AmiiboDatabaseManager::calculateAmiiboInfoHash(const amiiboInfoStruct* amiiboInfo, char* hash) {
    AmiiBuddyAmiiboInfo amiiboDetailsCache = {
            0, 0, 0, 0, 0,
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 },
            { 0 },
            { 0 }
    };
    auto * amiiboDetailsCacheBytes = reinterpret_cast<uint8_t*>(&amiiboDetailsCache);

    amiiboDetailsCache.characterNumber = amiiboInfo->amiiboCharacterNumber;
    amiiboDetailsCache.variation = amiiboInfo->amiiboVariation;
    amiiboDetailsCache.form = amiiboInfo->amiiboForm;
    amiiboDetailsCache.number = amiiboInfo->amiiboNumber;
    amiiboDetailsCache.set = amiiboInfo->amiiboSet;

    for (int i = 0; i < 4; i++) {
        amiiboDetailsCache.head[i] = amiiboInfo->amiiboHead[i];
        amiiboDetailsCache.tail[i] = amiiboInfo->amiiboTail[i];
    }

    md5Builder.begin();
    md5Builder.add(amiiboDetailsCacheBytes, sizeof(AmiiBuddyAmiiboInfo));
    md5Builder.calculate();
    md5Builder.getChars(hash);
}


void buildAmiiboInfoJsonDoc(StaticJsonDocument<100>* jsonDoc, amiiboInfoStruct* amiiboInfo, const char* filePath) {
    char hashCache[33] = "";

    jsonDoc->clear();
    (*jsonDoc)["name"] = String(amiiboInfo->amiiboName);
    (*jsonDoc)["owner_name"] = String(amiiboInfo->amiiboOwnerMiiName);

    sprintf(hashCache, "%04hX", amiiboInfo->amiiboCharacterNumber);
    (*jsonDoc)["character_number"] = String(hashCache);
    sprintf(hashCache, "%02hhX", amiiboInfo->amiiboVariation);
    (*jsonDoc)["variation"] = String(hashCache);
    sprintf(hashCache, "%02hhX", amiiboInfo->amiiboForm);
    (*jsonDoc)["form"] = String(hashCache);
    sprintf(hashCache, "%04hX", amiiboInfo->amiiboNumber);
    (*jsonDoc)["number"] = String(hashCache);
    sprintf(hashCache, "%02hhX", amiiboInfo->amiiboSet);
    (*jsonDoc)["set"] = String(hashCache);
    (*jsonDoc)["head"] = String(amiiboInfo->amiiboHeadChar);
    (*jsonDoc)["tail"] = String(amiiboInfo->amiiboTailChar);

    AmiiboDatabaseManager::calculateAmiiboInfoHash(amiiboInfo, hashCache);

    (*jsonDoc)["hash"] = String(hashCache);
    (*jsonDoc)["file_path"] = String(filePath);

//    jsonDoc.clear();
//    jsonDoc["name"] = amiiboInfo->amiiboName;
//    jsonDoc["owner_name"] = amiiboInfo->amiiboOwnerMiiName;
//
//    sprintf(hashCache, "%04hX", amiiboInfo->amiiboCharacterNumber);
//    jsonDoc["character_number"] = hashCache;
//    sprintf(hashCache, "%02hhX", amiiboInfo->amiiboVariation);
//    jsonDoc["variation"] = hashCache;
//    sprintf(hashCache, "%02hhX", amiiboInfo->amiiboForm);
//    jsonDoc["form"] = hashCache;
//    sprintf(hashCache, "%04hX", amiiboInfo->amiiboNumber);
//    jsonDoc["number"] = hashCache;
//    sprintf(hashCache, "%02hhX", amiiboInfo->amiiboSet);
//    jsonDoc["set"] = hashCache;
//    jsonDoc["head"] = amiiboInfo->amiiboHeadChar;
//    jsonDoc["tail"] = amiiboInfo->amiiboTailChar;
//
//    calculateAmiiboInfoHash(amiiboInfo, hashCache);
//
//    jsonDoc["hash"] = hashCache;
//    jsonDoc["file_path"] = filePath;
}

void writeJsonFile(const char* filePath, JsonDocument& jsonDoc) {
    PRINTV("Writing to file: ", filePath);
    serializeJsonPretty(jsonDoc, Serial);

    File fileHandler;
    openForWrite(filePath, &fileHandler);

    if (! fileHandler || serializeJson(jsonDoc, fileHandler) == 0) {
        if (fileHandler) {
            fileHandler.close();
        }
        String warning = String("Failed to write to file: ") + filePath;
        PRINTLN(warning);
        M5ez::msgBox("Warning", warning, "Dismiss");
        return;
    } else {
        fileHandler.close();
    }
}

void rebuildFileData(const char* library_path, const char* data_path) {
    uint8_t amiiboData[NTAG215_SIZE] = { 0 };

    readDirectoryDetails(library_path, directoryDetails);
    int file_count = directoryDetails.file_count;

    ezProgressBar pb ("Processing...", "Processing your library...");

    PRINTV("Total file count: ", file_count);

    String byAmiiboHashPath(String(data_path) + "/byAmiiboHash");
    String savesByAmiiboHashPath(String(data_path) + "/savesByAmiiboHash");
    mkdirDeep(byAmiiboHashPath.c_str());
    mkdirDeep(savesByAmiiboHashPath.c_str());
    M5ez::yield();

    int itemCounter = 0;
    File dir = AMIIBUDDY_FS_DEFAULT.open(library_path);
    M5ez::yield();

    auto incrementProgressValue = [&itemCounter, &pb, file_count]() {
        itemCounter++;

        if (itemCounter % 2 != 0) {
            return;
        }

        PRINTV("Item counter: ", itemCounter);
        PRINTV("File count: ", file_count);
        pb.value(((float)itemCounter / (float)file_count) * 100);
        M5ez::redraw();
        M5ez::yield();
    };

    DynamicJsonDocument fullRef(AMIIBO_SEARCH_FILE_SIZE);
    AmiiboDataDoc jsonDoc;
    M5ez::yield();
    char hashCache[33] = "";

    printHeapUsage();

    traverseFiles(&dir, [&incrementProgressValue, &amiiboData, byAmiiboHashPath, savesByAmiiboHashPath, &jsonDoc, &fullRef, &hashCache](File *entry) -> void {
        String filePath(entry->name());
        PRINTV("Processing file: ", filePath);
        String lcFilePath(entry->name());
        lcFilePath.toLowerCase();

        if (! lcFilePath.endsWith(".bin") || lcFilePath.endsWith("/.ds_store")) {
            M5ez::yield();
            return;
        }

        if (readData(filePath.c_str(), amiiboData, NTAG215_SIZE) != 0) {
            String warning = "Could not load file: " + filePath;
            PRINTLN(warning);
            M5ez::msgBox("Warning", warning, "Dismiss");
            return;
        }

        // PRINTHEX(amiiboData, NTAG215_SIZE);

        amiiBuddy.loadKey();

        if (! atool.loadFileFromData(amiiboData, NTAG215_SIZE, false)) {
            String warning = "Encountered invalid amiibo file: " + filePath;
            PRINTLN(warning);
            M5ez::msgBox("Warning", warning, "Dismiss");
            return;
        }

        M5ez::yield();

        //buildAmiiboInfoJsonDoc(&jsonDoc, &atool.amiiboInfo, filePath.c_str());
        jsonDoc.clear();
        jsonDoc["name"] = String(atool.amiiboInfo.amiiboName);
        jsonDoc["owner_name"] = String(atool.amiiboInfo.amiiboOwnerMiiName);

        sprintf(hashCache, "%04hX", atool.amiiboInfo.amiiboCharacterNumber);
        jsonDoc["character_number"] = String(hashCache);
        sprintf(hashCache, "%02hhX", atool.amiiboInfo.amiiboVariation);
        jsonDoc["variation"] = String(hashCache);
        sprintf(hashCache, "%02hhX", atool.amiiboInfo.amiiboForm);
        jsonDoc["form"] = String(hashCache);
        sprintf(hashCache, "%04hX", atool.amiiboInfo.amiiboNumber);
        jsonDoc["number"] = String(hashCache);
        sprintf(hashCache, "%02hhX", atool.amiiboInfo.amiiboSet);
        jsonDoc["set"] = String(hashCache);
        jsonDoc["head"] = String(atool.amiiboInfo.amiiboHeadChar);
        jsonDoc["tail"] = String(atool.amiiboInfo.amiiboTailChar);

        AmiiboDatabaseManager::calculateAmiiboInfoHash(&atool.amiiboInfo, hashCache);

        jsonDoc["hash"] = String(hashCache);
        jsonDoc["file_path"] = filePath;

//        serializeJson(jsonDoc, Serial);
        Serial.println();

        String metadataPath = byAmiiboHashPath + "/" + hashCache + ".json";
        writeJsonFile(metadataPath.c_str(), jsonDoc);

        uint8_t saveChecksum[AMIIBO_SAVE_CHECKSUM_LEN];
        AmiiboDatabaseManager::getSaveChecksum(amiiboData, saveChecksum);
        char checksumStr[AMIIBO_SAVE_CHECKSUM_STR_LEN];
        byteToChar(saveChecksum, AMIIBO_SAVE_CHECKSUM_LEN, checksumStr, AMIIBO_SAVE_CHECKSUM_STR_LEN);

        jsonDoc.clear();
        jsonDoc[String(checksumStr)] = filePath;
        fullRef[String(hashCache)] = filePath;

        String saveFilePath = savesByAmiiboHashPath + "/" + hashCache + ".json";
        writeJsonFile(saveFilePath.c_str(), jsonDoc);

        incrementProgressValue();

        printHeapUsage();
    });

    printHeapUsage();
    writeJsonFile(AMIIBO_SEARCH_FILE_PATH, fullRef);
    printHeapUsage();
}


void rebuildPowerSaveData(const char* power_save_path, const char* data_path) {
    uint8_t amiiboData[NTAG215_SIZE] = { 0 };

    readDirectoryDetails(power_save_path, directoryDetails);
    int file_count = directoryDetails.file_count;

    ezProgressBar pb ("Processing...", "Processing power saves...");

    PRINTV("Total file count: ", file_count);

    String byAmiiboHashPath(String(data_path) + "/byAmiiboHash");
    String savesByAmiiboHashPath(String(data_path) + "/savesByAmiiboHash");

    int itemCounter = 0;
    File dir = AMIIBUDDY_FS_DEFAULT.open(power_save_path);
    M5ez::yield();

    auto incrementProgressValue = [&itemCounter, &pb, file_count]() {
        itemCounter++;

        if (itemCounter % 2 != 0) {
            return;
        }

        PRINTV("Item counter: ", itemCounter);
        PRINTV("File count: ", file_count);
        pb.value(((float)itemCounter / (float)file_count) * 100);
        M5ez::redraw();
        M5ez::yield();
    };

    DynamicJsonDocument jsonDoc(AMIIBO_SEARCH_FILE_SIZE);
    M5ez::yield();
    char hashCache[33] = "";

    printHeapUsage();

    traverseFiles(&dir, [&incrementProgressValue, &amiiboData, savesByAmiiboHashPath, &jsonDoc, &hashCache](File *entry) -> void {
        String filePath(entry->name());
        PRINTV("Processing file: ", filePath);

        if (readData(filePath.c_str(), amiiboData, NTAG215_SIZE) != 0) {
            String warning = "Could not load file: " + filePath;
            PRINTLN(warning);
            M5ez::msgBox("Warning", warning, "Dismiss");
            return;
        }

        amiiBuddy.loadKey();

        if (! atool.loadFileFromData(amiiboData, NTAG215_SIZE, false)) {
            String warning = "Encountered invalid amiibo file: " + filePath;
            PRINTLN(warning);
            M5ez::msgBox("Warning", warning, "Dismiss");
            return;
        }

        M5ez::yield();

        AmiiboDatabaseManager::calculateAmiiboInfoHash(&atool.amiiboInfo, hashCache);

        uint8_t saveChecksum[AMIIBO_SAVE_CHECKSUM_LEN];
        AmiiboDatabaseManager::getSaveChecksum(amiiboData, saveChecksum);

        char saveChecksumStr[AMIIBO_SAVE_CHECKSUM_STR_LEN];
        byteToChar(saveChecksum, AMIIBO_SAVE_CHECKSUM_LEN, saveChecksumStr, AMIIBO_SAVE_CHECKSUM_STR_LEN);

        String saveList = savesByAmiiboHashPath + "/" + hashCache + ".json";
        PRINTV("Reading save list: ", saveList);

        File saveFileRead = AMIIBUDDY_FS_DEFAULT.open(saveList, FILE_READ);
        deserializeJson(jsonDoc, saveFileRead);
        saveFileRead.close();
        serializeJsonPretty(jsonDoc, Serial);

        jsonDoc[saveChecksumStr] = filePath;

//        serializeJson(jsonDoc, Serial);

        writeJsonFile(saveList.c_str(), jsonDoc);
        jsonDoc.clear();

        incrementProgressValue();

        printHeapUsage();
    });
}

void AmiiboDatabaseManager::initialize(const char* library_path, const char* power_saves_path, const char* data_path) {
    empty();

    rebuildFileData(library_path, data_path);

    rebuildPowerSaveData(power_saves_path, data_path);

    ESP.restart();
}

