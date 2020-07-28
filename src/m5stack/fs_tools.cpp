//
// Created by hotdropper on 7/19/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <list>
#include <FS.h>
#include "fs_tools.h"
#include <M5ez.h>

void traverseEntries(std::list<File>* dirs, bool callOnFile, bool callOnDir, const std::function <void (File*)>& callback) {
    if (dirs->empty()) {
        return;
    }

    File entry;
    for (auto dir : *dirs) {
        PRINTV("Processing dir: ", dir.name());
        while ((entry = dir.openNextFile(FILE_READ))) {
            if (entry.isDirectory()) {
                dirs->push_back(entry);
                if (callOnDir) {
                    callback(&entry);
                }
                continue;
            }

            if (callOnFile) {
                callback(&entry);
            }
        }
    }
}

void traverseDirs(File* dir, const std::function <void (File*)>& callback) {
    std::list<File> dirs;
    dirs.push_front(*dir);
    return traverseEntries(&dirs, false, true, callback);
}

void traverseEntries(File* dir, const std::function <void (File*)>& callback) {
    std::list<File> dirs;
    dirs.push_front(*dir);
    return traverseEntries(&dirs, true, true, callback);
}

void traverseFiles(File* dir, const std::function <void (File*)>& callback) {
    std::list<File> dirs;
    dirs.push_front(*dir);
    return traverseEntries(&dirs, true, false, callback);
}

void forEachDir(const char* path, const std::function <void (const char*)>& callback, FS* fs) {
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

void mkdirDeep(const char* path, FS* fs) {
    forEachDir(path, [&fs](const char* pathFragment) {
        if (! fs->exists(pathFragment)) {
            PRINTV("Making directory", pathFragment);
            fs->mkdir(pathFragment);
        }
    }, fs);
}

void openForWrite(const char* path, File* file, FS* fs) {
    *file = fs->open(path, FILE_WRITE);

    if (! *file) {
        *file = fs->open(path, FILE_WRITE);
    }
}

int readData(const char *path, uint8_t *data, int dataLength, FS* fs) {
    File f = fs->open(path, FILE_READ);
    if (!f) {
        PRINTLN("Could not load key");
        return -1;
    }

    if (!f.read(data, dataLength)) {
        f.close();
        PRINTLN("Reading failed.");
        return -2;
    }

    f.close();

    return 0;
}

int writeData(const char *path, uint8_t *data, int dataLength, FS* fs) {
    File f = fs->open(path, FILE_WRITE);
    if (!f) {
        PRINTLN("Could open file to write");
        return -1;
    }

    if (!f.write(data, dataLength)) {
        f.close();
        PRINTLN("Writing failed.");
        return -2;
    }

    f.close();

    return 0;
}

int readDirectoryDetails(const char* path, DirectoryDetails& details, FS* fs) {
    if (! fs->exists(path)) {
        return -1;
    }

    ezProgressBar pb("Scanning...", String("Scanning ") + path + "...");

    size_t break_point = 10;
    size_t counter = 0;

    File dir = fs->open(path);
    traverseEntries(&dir, [&details, &counter, &break_point, &pb](File* entry) {
        if (entry->isDirectory()) {
            details.dir_count++;
        } else {
            details.file_count++;
        }

        counter++;
        if (counter % 10 != 0) {
            return;
        }

        float value = (float)counter / (float)break_point * 100;

        if (counter >= break_point) {
            counter = 0;
        }

        pb.value(value);
        M5ez::redraw();
        M5ez::yield();
    });

    return details.file_count + details.dir_count;
}


int getFileCount(const char *path, FS* fs) {
    if (! fs->exists(path)) {
        return -1;
    }

    File dir = fs->open(path);
    int fileCount = 0;

    traverseFiles(&dir, [&fileCount](File *entry) -> void {
        fileCount++;
    });

    return fileCount;
}
