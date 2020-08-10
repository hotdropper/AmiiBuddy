//
// Created by hotdropper on 7/19/20.
//

#ifndef AMIIBUDDY_AMIIBUDDY_H
#define AMIIBUDDY_AMIIBUDDY_H

#include <FS.h>
#include <PN532.h>
#include "AmiiboDatabase.h"
#include "NTag215.h"

#define KEY_FILE "/sd/keys/retail.bin"


class AmiiBuddy {
public:
    void begin(FS* fs, PN532* nfc, AmiiboDatabase* db);
    int writeAmiibo(NTag215* tag, const char* path = nullptr);
    bool loadKey();

private:
    FS* _fs = nullptr;
    PN532* _nfc = nullptr;
    AmiiboDatabase* _db = nullptr;
    nfc3d_amiibo_keys _amiiboKeyData;
    uint8_t * _amiiboKeyDataBytes = reinterpret_cast<uint8_t*>(&_amiiboKeyData);
    uint8_t _amiiboTag[NTAG215_SIZE];
    bool _hasCachedKey = false;

};

extern AmiiBuddy amiiBuddy;

#endif //AMIIBUDDY_AMIIBUDDY_H
