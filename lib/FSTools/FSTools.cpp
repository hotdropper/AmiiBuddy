//
// Created by Jacob Mather on 7/31/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "FSTools.h"
#include <string>

#if !defined(FSTools_SD) && !defined(FSTools_SD_MMC) && !defined(FSTools_SPIFFS) && !defined(FSTools_Native)
#define FSTools_SD
//#define FSTools_SD_MMC
//#define FSTools_SPIFFS
#endif

#ifdef FSTools_SD
#include <SD.h>
#endif
#ifdef FSTools_SD_MMC
#include <SD_MMC.h>
#endif
#ifdef FSTools_SPIFFS
#include <SPIFFS.h>
#endif
#ifdef FSTools_Native
//#include <FSTools_Native.h>
#endif

char* FSTools::pathBuffer = (char*) malloc(sizeof(char[FSTOOLS_MAX_PATH_LENGTH]));
char* FSTools::pathBuffer2 = (char*) malloc(sizeof(char[FSTOOLS_MAX_PATH_LENGTH]));
FS* FSTools::fsBuffer = nullptr;
FS* FSTools::fsBuffer2 = nullptr;

void FSTools::init() {
}

void FSTools::traverseEntries(FS* fs, std::list<String>& dirs, const bool recursive, const bool callOnFile, const bool callOnDir, const std::function <void (File*)>& callback) {
    if (dirs.empty()) {
        return;
    }

    File dir;
    File entry;
    for (const auto& dirPath : dirs) {
        PRINTV("Scanning:", dirPath);
        dir = fs->open(dirPath, FILE_READ);
        while ((entry = dir.openNextFile(FILE_READ))) {
            if (entry.isDirectory()) {
                if (recursive) {
                    const auto dirEntry = String(entry.name());
                    dirs.push_back(dirEntry);
                }

                if (callOnDir) {
                    callback(&entry);
                }
                entry.close();
                continue;
            }

            if (callOnFile) {
                callback(&entry);
                entry.close();
            }
        }
        dir.close();
    }
}

void FSTools::traverseDirs(const char* dir, const std::function <void (File*)>& callback, FS* fs) {
    std::list<String> dirs;
    String dirPath = String(dir);
    dirs.push_front(dirPath);
    return traverseEntries(fs, dirs, true, false, true, callback);
}

void FSTools::traverseEntries(const char* dir, const bool recursive, const std::function <void (File*)>& callback, FS* fs) {
    std::list<String> dirs;
    String dirPath = String(dir);
    dirs.push_front(dirPath);
    return traverseEntries(fs, dirs, recursive, true, true, callback);
}

void FSTools::traverseFiles(const char* dir, const std::function <void (File*)>& callback, FS* fs) {
    std::list<String> dirs;
    String dirPath = String(dir);
    dirs.push_front(dirPath);
    return traverseEntries(fs, dirs, true, true, false, callback);
}

void FSTools::forEachDir(const char* path, const std::function <void (const char*)>& callback, FS* fs) {
    String filePath(path);
    int nextIndex = filePath.indexOf("/");
    while (nextIndex > -1) {
        if (nextIndex != 0) {
            callback(filePath.substring(0, nextIndex).c_str());
        }

        nextIndex = filePath.indexOf("/", nextIndex + 1);
    }

    if (fs->exists(path)) {
        File f = fs->open(path);
        if (! f.isDirectory()) {
            f.close();
            return;
        }
        f.close();
    }

    callback(path);
}

bool FSTools::mkdirDeep(const char* path, FS* fs) {
    bool ret = true;
    forEachDir(path, [&fs, &ret](const char* pathFragment) {
        if (! fs->exists(pathFragment)) {
            if (ret == false || ! fs->mkdir(pathFragment)) {
                ret = false;
            }
        }
    }, fs);
    return ret;
}

void FSTools::openForWrite(const char* path, File* file, FS* fs) {
    *file = fs->open(path, FILE_WRITE);

    if (! *file) {
        *file = fs->open(path, FILE_WRITE);
    }
}

void FSTools::openForRead(const char* path, File* file, FS* fs) {
    *file = fs->open(path, FILE_READ);
}

int FSTools::rename(const char* from, const char* to, FS* fs) {
    if (! fs->rename(from, to)) {
        return -5;
    }

    return 0;
}

void FSTools::open(const char* path, File* file, FS* fs) {
    *file = fs->open(path);
}

bool FSTools::remove(const char* path, FS* fs) {
    if (! fs->exists(path)) {
        return fs->remove(path);
    }
}

int FSTools::readData(const char *path, uint8_t *data, int dataLength, FS* fs) {
    File f = fs->open(path, FILE_READ);
    if (!f) {
        return -1;
    }

    if (!f.read(data, dataLength)) {
        f.close();
        return -2;
    }

    f.close();

    return 0;
}

int FSTools::writeData(const char *path, const uint8_t *data, int dataLength, FS* fs) {
    String rootPath(path);
    rootPath = rootPath.substring(0, rootPath.lastIndexOf("/"));
    PRINTV("Ensuring directory exists:", rootPath);
    if (! exists(rootPath.c_str(), fs) && ! mkdirDeep(rootPath.c_str(), fs)) {
        PRINTLN("Making the directory failed");
        return -3;
    }

    File f = fs->open(path, FILE_WRITE);
    if (!f) {
        return -1;
    }

    PRINTV("File", path);
    PRINTV("Length", dataLength);
    int writeResponse = f.write(data, dataLength);
    PRINTV("Write response", writeResponse);
    if (writeResponse != dataLength) {
        f.close();
        return writeResponse;
    }

    f.close();

    return writeResponse;
}

int FSTools::readDirectoryDetails(const char* path, DirectoryDetails& details, FS* fs, const std::function <void (File*)>& callback) {
    if (! fs->exists(path)) {
        return -1;
    }

    details.file_count = 0;
    details.dir_count = 0;

    traverseEntries(path, true, [&details, &callback](File* entry) {
        if (entry->isDirectory()) {
            details.dir_count++;
        } else {
            details.file_count++;
        }

        if (callback != nullptr) {
            callback(entry);
        }
    }, fs);

    return details.file_count + details.dir_count;
}


int FSTools::getFileCount(const char *path, FS* fs) {
    if (! fs->exists(path)) {
        return -1;
    }

    int fileCount = 0;

    traverseFiles(path, [&fileCount](File *entry) -> void {
        fileCount++;
    }, fs);

    return fileCount;
}

void cpypath(char* outPath, const char* path, int len) {
    PRINTV("Path:", path);
    PRINTV("Len:", len);
    if (strlen(path) <= len) {
        strcpy(outPath, "/");
    } else {
        PRINTV("Stub Path", &path[len]);
        strcpy(outPath, &path[len]);
    }
    PRINTV("OutPath:", outPath);
}

FS* FSTools::getFSByPath(const char* path, char* outPath) {
#ifdef FSTools_SD
    PRINTLN("Checking SD");
    PRINTV("Path:", path);
    if (strncmp(path, "/sd/", 4) == 0 || strcmp(path, "/sd") == 0) {
        cpypath(outPath, path, 3);
        return &SD;
    }
#endif

#ifdef FSTools_SPIFFS
    if (strncmp(path, "/fs/", 4) == 0 || strcmp(path, "/fs") == 0) {
        cpypath(outPath, path, 3);
        return &SPIFFS;
    }
#endif

#ifdef FSTools_SD_MMC
    if (strncmp(path, "/mmc/", 5) == 0 || strcmp(path, "/mmc") == 0) {
        cpypath(outPath, path, 4);
        return &SD_MMC;
    }
#endif

#ifdef FSTools_SD
    strcpy(outPath, path);
    return &SD;
#endif

    return nullptr;
}