//
// Created by Jacob Mather on 7/31/20.
//

#ifndef AMIIBUDDY_AMIIBODBAO_H
#define AMIIBUDDY_AMIIBODBAO_H

#define AMIIBO_SAVE_CHECKSUM_POS (4 * 4) + 2
#define AMIIBO_SAVE_CHECKSUM_BYTE_LEN (9 * 4) - 2

#include <cstdint>
#include <amiitool.h>
#include <functional>

struct HashInfo {
    char amiiboHash[33] = "";
    char saveHash[33] = "";
};

struct SaveRecord {
    int id = 0;
    int amiibo_id = 0;
    char hash[33] = "";
    char name[40] = "";
    char file[120] = "";
    int last_update = 0;
    bool is_custom = false;
};

struct AmiiboHashDetails {
    unsigned short characterNumber = 0;
    byte variation = 0;
    byte form = 0;
    unsigned short number = 0;
    byte set = 0;
    uint8_t head[4] = {0, 0, 0, 0};
    uint8_t tail[4] = {0, 0, 0, 0};
};

struct AmiiboRecord {
    unsigned short characterNumber = 0;
    byte variation = 0;
    byte form = 0;
    unsigned short number = 0;
    byte set = 0;
    uint8_t head[4] = {0, 0, 0, 0};
    uint8_t tail[4] = {0, 0, 0, 0};
    char hash[33] = "";
    char name[40] = "";
    char amiiboName[AMIIBO_NAME_LEN*6+1] = "";
    char ownerName[AMIIBO_NAME_LEN*6+1] = "";
    char file[120] = "";
    int id = 0;
    int last_written = 0;
    void setHead(const uint8_t* newHead) {
        for (int i = 0; i < 4; i++) {
            this->head[i] = newHead[i];
        }
    }
    void setTail(const uint8_t* newTail) {
        for (int i = 0; i < 4; i++) {
            this->tail[i] = newTail[i];
        }
    }
};

class AmiiboDBAO {
public:
    static bool begin();
    static bool end();
    static bool truncate();
    static bool insertAmiibo(AmiiboRecord& amiibo);
    static bool insertSave(SaveRecord& save);
    static bool updateSave(SaveRecord& save);
    static bool updateAmiiboTimestamp(const int id, const int timestamp = 0);
    static bool updateSaveTimestamp(const int id, const int timestamp = 0);
    static int findAmiiboIdByHash(const char* hash);
    static bool findAmiiboById(const int id, AmiiboRecord& amiibo);
    static bool findSaveById(const int id, SaveRecord& save);
    static bool findAmiiboByHash(const char* hash, AmiiboRecord& amiibo);
    static bool findAmiiboByFileName(const char* filename, AmiiboRecord& amiibo);
    static bool findAmiibosByFileNameMatch(const char* filename, const std::function<void(AmiiboRecord& record)>& callback = nullptr, int limit = 0);
    static bool findSavesByAmiiboHash(const char* hash, const std::function<void(SaveRecord& record)>& callback = nullptr, int limit = 0);
    static bool findSavesByAmiiboFileName(const char* filename, const std::function<void(SaveRecord& record)>& callback = nullptr, int limit = 0);
    static void calculateHashes(uint8_t* tagData, HashInfo& hashes);
    static void calculateHashes(const amiiboInfoStruct& amiiboInfo, HashInfo& hashes);
    static void calculateSaveHash(const amiiboInfoStruct& amiiboInfo, char* checksum);
    static void calculateAmiiboInfoHash(const amiiboInfoStruct& amiiboInfo, char* hash);
};


#endif //AMIIBUDDY_AMIIBODBAO_H
