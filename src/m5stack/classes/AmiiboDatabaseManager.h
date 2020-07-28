//
// Created by hotdropper on 7/27/20.
//

#ifndef AMIIBUDDY_AMIIBODATABASEMANAGER_H
#define AMIIBUDDY_AMIIBODATABASEMANAGER_H

#include <ArduinoJson.h>
#include <amiitool.h>

#define LIBRARY_PATH_FRAGMENT /library
#define LIBRARY_PATH "/library"
#define AMIIBO_DATA_PATH "/data"
#define AMIIBO_DATA_FILE_SIZE 512
#define POWERSAVES_PATH "/powersaves"
#define AMIIBO_SAVE_CHECKSUM_LEN (9 * 4) - 2
#define AMIIBO_SAVE_CHECKSUM_STR_LEN (((9 * 4) - 2) * 2) + 1
#define AMIIBO_SAVE_CHECKSUM_POS (4 * 4) + 2
#define AMIIBO_SEARCH_FILE_PATH "/data/index.json"
#define AMIIBO_SEARCH_FILE_SIZE 102400

#define AMIIBO_DATA_FILE_PATH "/data/byAmiiboHash"
#define AMIIBO_SAVE_FILE_PATH "/data/savesByAmiiboHash"
#define AMIIBO_DATA_FILE_SIZE 512

typedef StaticJsonDocument<AMIIBO_DATA_FILE_SIZE> AmiiboDataDoc;

struct AmiiBuddyAmiiboInfo {
    unsigned short characterNumber;
    byte variation;
    byte form;
    unsigned short number;
    byte set;
    uint8_t head[4];
    uint8_t tail[4];
    char name[AMIIBO_NAME_LEN*6+1];
    char ownerName[AMIIBO_NAME_LEN*6+1];
};

class AmiiboDatabaseManager {
public:
    static void initialize(const char* library_path = LIBRARY_PATH, const char* power_saves_path = POWERSAVES_PATH, const char* data_path = AMIIBO_DATA_PATH);
    static void getSaveChecksum(const uint8_t* data, uint8_t* checksum);
    static void calculateAmiiboInfoHash(const uint8_t* data, char* hash);
    static void calculateAmiiboInfoHash(const amiiboInfoStruct* amiiboInfo, char* hash);
    static void empty(const char* data_path = AMIIBO_DATA_PATH);
};

#endif //AMIIBUDDY_AMIIBODATABASEMANAGER_H
