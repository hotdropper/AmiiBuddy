//
// Created by hotdropper on 7/18/20.
//

#ifndef AMIIBUDDY_AMIIBODATABASE_H
#define AMIIBUDDY_AMIIBODATABASE_H

#include <list>
#include <SD.h>
#include <amiitool.h>
#include <string>
#include <functional>
#include <map>
#include "AmiiboDatabaseManager.h"

typedef std::function<void(String&, String&)> search_callback_t;

class AmiiboDatabase {
public:
    explicit AmiiboDatabase(const char* libraryPath = LIBRARY_PATH, const char* dataPath = AMIIBO_DATA_PATH);
    bool begin();
    int search(const char* text, const search_callback_t& callback);
    int lookupAmiibo(const char* id, const search_callback_t& callback);
    int lookupSave(const char* amiibo_hash, const char* save_hash, const search_callback_t& callback);
    int lookupSaves(const char* amiibo_hash, const bool onlyUserSaves, const search_callback_t& callback);
    int findByData(const uint8_t* data, const search_callback_t& callback);
    int writeSave(const char* amiibo_hash, const char* save_hash, const char* save_name, const uint8_t* data);
    int removeSave(const char* amiibo_hash, const char* save_hash);

private:
    int getFileCount(const char* path);
    void empty();
    FS* _fs;
    File* _dbFile;

    const char* _library_path;
    const char* _data_path;
};

extern AmiiboDatabase amiiboDatabase;

#endif //AMIIBUDDY_AMIIBODATABASE_H
