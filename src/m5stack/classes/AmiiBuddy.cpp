//
// Created by hotdropper on 7/19/20.
//

#define PRINT_DEBUG 0
#include <ArduinoDebug.h>
#include "AmiiBuddy.h"
#include <M5ez.h>
#include <FSTools.h>

nfc3d_amiibo_keys amiiboKeyData;
uint8_t * amiiboKeyDataBytes = reinterpret_cast<uint8_t*>(&amiiboKeyData);
uint8_t amiiboTag[NTAG215_SIZE];
bool haveReadKey = false;

AmiiBuddy amiiBuddy;

void AmiiBuddy::begin(FS* fs, PN532* nfc, AmiiboDatabase* db) {
    _fs = fs;
    _nfc = nfc;
    _db = db;
}

bool AmiiBuddy::loadKey() {
    if (! _hasCachedKey) {
        FSTools::readData(KEY_FILE, amiiboKeyDataBytes, AMIIBO_KEY_FILE_SIZE);
        PRINTLN("Key data: ");
        PRINTHEX(amiiboKeyDataBytes, AMIIBO_KEY_FILE_SIZE);

        haveReadKey = true;
    }
    return atool.setAmiiboKeys(amiiboKeyData);
}

int AmiiBuddy::writeAmiibo(NTag215 *tag, const char *path) {
    if (path != nullptr) {
        PRINT("Reading: ");
        PRINTLN(path);
        if (FSTools::readData(path, tag->data, NTAG215_SIZE) != 0) {
            return -1;
        }
    } else {
        PRINTLN("Using data on tag");
    }

    loadKey();

    PRINTLN("Loading tag into amiitool.");
    if (! atool.loadFileFromData(tag->data, NTAG215_SIZE, false)) {
        return -2;
    }

    PRINT("Encrypting file with uuid: ");
    PRINTHEX(tag->uid, 7);

    if (atool.encryptLoadedFile(tag->uid) < 0) {
        return -3;
    }

    memcpy(tag->data, atool.original, NTAG215_SIZE);

    int writeRet = tag->writeAmiibo();

    if (writeRet != 0) {
        return writeRet - 3;
    }
}