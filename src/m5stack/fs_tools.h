//
// Created by hotdropper on 7/19/20.
//

#ifndef AMIIBUDDY_FS_TOOLS_H
#define AMIIBUDDY_FS_TOOLS_H

#ifndef AMIIBUDDY_FS_DEFAULT
#define AMIIBUDDY_FS_SD
#define AMIIBUDDY_FS_DEFAULT SD
#endif

#ifdef AMIIBUDDY_FS_SD
#include <SD.h>
#endif

#ifdef AMIIBUDDY_FS_SPIFFS
#include <SPIFFS.h>
#endif

#include <functional>
#include "utils.h"

struct DirectoryDetails {
    int file_count;
    int dir_count;
};

void traverseEntries(File* dir, const std::function <void (File*)>& callback);
void traverseFiles(File* dir, const std::function <void (File*)>& callback);
void traverseDirs(File* dir, const std::function <void (File*)>& callback);
void forEachDir(const char* path, const std::function <void (const char*)>& callback, FS* fs = &AMIIBUDDY_FS_DEFAULT);
void mkdirDeep(const char* path, FS* fs = &AMIIBUDDY_FS_DEFAULT);
void openForWrite(const char* path, File* file, FS* fs = &AMIIBUDDY_FS_DEFAULT);
int readData(const char *path, uint8_t *data, int dataLength, FS* fs = &AMIIBUDDY_FS_DEFAULT);
int writeData(const char *path, uint8_t *data, int dataLength, FS* fs = &AMIIBUDDY_FS_DEFAULT);
int readDirectoryDetails(const char* path, DirectoryDetails& details, FS* fs = &AMIIBUDDY_FS_DEFAULT);
int getFileCount(const char *path, FS* fs = &AMIIBUDDY_FS_DEFAULT);

#endif //AMIIBUDDY_FS_TOOLS_H
