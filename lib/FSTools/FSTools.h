//
// Created by Jacob Mather on 7/31/20.
//

#ifndef AMIIBUDDY_FSTOOLS_H
#define AMIIBUDDY_FSTOOLS_H

#define PRINT_DEBUG 0
#include <ArduinoDebug.h>
#include <functional>
#include <list>
#if defined(FSTools_SD) || defined(FSTools_SD_MMC) || defined(FSTools_SPIFFS) || !defined(FSTools_Native)
#include <FS.h>
#endif
#ifdef FSTools_Native
#include "FSTools_Native.h"
#endif

#define FSTOOLS_MAX_PATH_LENGTH 256

struct DirectoryDetails {
    int file_count;
    int dir_count;
};

class FSTools {
public:
    static void init();
    static FS* getFSByPath(const char* path, char* outPath);
    static void traverseEntries(std::list<File>* dirs, const bool recursive, const bool callOnFile, const bool callOnDir, const std::function <void (File*)>& callback);
    static void traverseEntries(File* dir, const bool recursive, const std::function <void (File*)>& callback);
    static void traverseFiles(File* dir, const std::function <void (File*)>& callback);
    static void traverseDirs(File* dir, const std::function <void (File*)>& callback);
    static void forEachDir(const char* path, const std::function <void (const char*)>& callback, FS* fs);
    static void forEachDir(const char* path, const std::function <void (const char*)>& callback) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return;
        }

        return forEachDir(pathBuffer, callback, fsBuffer);
    }
    static void mkdirDeep(const char* path, FS* fs);
    static void mkdirDeep(const char* path) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return;
        }

        return mkdirDeep(pathBuffer, fsBuffer);
    }

    static void openForWrite(const char* path, File* file, FS* fs);
    static void openForWrite(const char* path, File* file) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return;
        }

        return openForWrite(pathBuffer, file, fsBuffer);
    }

    static void openForRead(const char* path, File* file, FS* fs);
    static void openForRead(const char* path, File* file) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return;
        }

        return openForRead(pathBuffer, file, fsBuffer);
    }

    static void open(const char* path, File* file, FS* fs);
    static void open(const char* path, File* file) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return;
        }

        return openForRead(pathBuffer, file, fsBuffer);
    }

    static int rename(const char* from, const char* to, FS* fs);
    static int rename(const char* from, const char* to) {
        if (! (fsBuffer = getFSByPath(from, pathBuffer))) {
            return -1;
        }

        if (! (fsBuffer2 = getFSByPath(to, pathBuffer2))) {
            return -2;
        }

        if (fsBuffer != fsBuffer2) {
            return -3;
        }

        return rename(pathBuffer, pathBuffer2, fsBuffer);
    }

    static void remove(const char* path, FS* fs);
    static void remove(const char* path) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return;
        }

        return remove(pathBuffer, fsBuffer);
    }

    static int readData(const char *path, uint8_t *data, int dataLength, FS* fs);
    static int readData(const char *path, uint8_t *data, int dataLength) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return -100;
        }

        return readData(pathBuffer, data, dataLength, fsBuffer);
    }
    static int writeData(const char *path, const uint8_t *data, int dataLength, FS* fs);
    static int writeData(const char *path, const uint8_t *data, int dataLength) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return -100;
        }

        return writeData(pathBuffer, data, dataLength, fsBuffer);
    }

    static int readDirectoryDetails(const char* path, DirectoryDetails& details, FS* fs, const std::function <void (File*)>& callback = nullptr);
    static int readDirectoryDetails(const char* path, DirectoryDetails& details, const std::function <void (File*)>& callback = nullptr) {
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            return -100;
        }

        return readDirectoryDetails(pathBuffer, details, fsBuffer, callback);
    }

    static int getFileCount(const char *path, FS* fs);
    static int getFileCount(const char *path) {
        PRINTV("Asking for file count on:", path);
        if (! (fsBuffer = getFSByPath(path, pathBuffer))) {
            PRINTLN("Could not find fs");
            return -100;
        }
        PRINTV("Found fs, new path:", pathBuffer);

        return getFileCount(pathBuffer, fsBuffer);
    }

    static char* pathBuffer;
    static char* pathBuffer2;
    static FS* fsBuffer;
    static FS* fsBuffer2;
};


#endif //AMIIBUDDY_FSTOOLS_H
