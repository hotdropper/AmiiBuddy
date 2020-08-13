//
// Created by hotdropper on 7/27/20.
//

#ifndef AMIIBUDDY_AMIIBODATABASEMANAGER_H
#define AMIIBUDDY_AMIIBODATABASEMANAGER_H

#include <amiitool.h>
#include "amiibuddy_constants.h"

#define LIBRARY_PATH_FRAGMENT /sd/library
#define LIBRARY_PATH "/sd/library"
#define AMIIBO_DATA_PATH "/sd/data"
#define AMIIBO_DATA_FILE_SIZE 512
#define POWER_SAVES_PATH "/sd/powersaves"
#define SAVES_PATH "/sd/saves"
#define AMIIBO_SAVE_CHECKSUM_BYTE_LEN (9 * 4) - 2
#define AMIIBO_HASH_LEN 33
#define AMIIBO_SAVE_CHECKSUM_POS (4 * 4) + 2
#define AMIIBO_SEARCH_FILE_PATH "/sd/data/index.json"
#define AMIIBO_SEARCH_FILE_SIZE 102400

#define AMIIBO_DATA_FILE_PATH "/sd/data/byAmiiboHash"
#define AMIIBO_SAVE_FILE_PATH "/sd/data/savesByAmiiboHash"
#define AMIIBO_DATA_FILE_SIZE 512
#define AMIIBO_DATA_FILE_PATH_SIZE 80

struct DirectoryComparator
{
    bool operator ()(String & str1, String & str2)
    {
        if(str1.length() == str2.length())
            return str2.compareTo(str1);
        return str2.length() < str1.length();
    }
};

class AmiiboDatabaseManager {
public:
    static bool initialize(const char* library_path = LIBRARY_PATH, const char* power_saves_path = POWER_SAVES_PATH, const char* custom_saves_path = SAVES_PATH);
};

#endif //AMIIBUDDY_AMIIBODATABASEMANAGER_H
