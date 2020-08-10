//
// Created by hotdropper on 7/20/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "NTag215.h"
#include "m5stack/utils.h"
#include "AmiiboDatabaseManager.h"

NTag215::NTag215(PN532 *adapter) {
    _inListed = false;
    _pn532 = adapter;
}

void NTag215::setPN532(PN532* adapter) {
    _pn532 = adapter;
}

bool NTag215::setUid(const char *newUid) {
    uint8_t bytes[strlen(newUid)];
    charToByte(newUid, strlen(newUid), bytes, sizeof(bytes));

    memcpy(uid, bytes, 7);

    return true;
}
int NTag215::inList() {
    if (_inListed) {
        return 0;
    }

    _pn532->begin();
    uint8_t uidLength;

    bool success = _pn532->inListPassiveTarget();

    if (! success) {
        PRINTLNS("We did not find an NFC Card!");
        return -1;
    }

    success = _pn532->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (! success) {
        PRINTLNS("We did not find an NFC Card to read the uid of!");
        return -2;
    }

    if (uidLength != 7) {
        PRINTLNS("This doesn't seem to be Amiibo!");
        return -3;
    }

    byteToChar(uid, 7, uidStr, sizeof(uidStr));
    PRINTV("Tag UID:", uidStr);

    _inListed = true;

    return 0;
}

int NTag215::writeAmiibo() {
    int inListResult = inList();

    if (inListResult != 0) {
        return inListResult;
    }

    for (byte uidbyte = 0; uidbyte < 7; uidbyte++) {
        byte actualbyte = uidbyte;
        if (actualbyte > 2) {
            actualbyte++;
        }

        if (data[actualbyte] != uid[uidbyte]) {
            PRINTLNS("UID mismatch!");
            PRINTLNS("UID of the tag doesn't match the UID specified in the dump.");
            PRINTHEXV("UID Value: ", uid, 7);

            return -3;
        }
    }

    PRINTLN();
    PRINTLNS("Tag found, writing...");
    PRINTLN();

    bool success;

    // Write main data
    for (byte page = 3; page < AMIIBO_PAGES; page++) {
        // Write data to the page
        PRINTV("Writing data into page ", page);

        success = _pn532->mifareultralight_WritePage(page, &data[(page * 4)]);

        if (success) {
            PRINTLNS("OK");
        } else {
            PRINTLNS("Fail");
            PRINTLNS("Write process failed, please try once more.");
            return -4;
        }
    }

    // Write lock bytes - the last thing you should do.
    // If you write them too early - your tag is wasted.
    PRINTLNS("Writing lock bytes");

    // Write Dynamic lock bytes
    success = _pn532->mifareultralight_WritePage(130, (uint8_t*)DynamicLockBytes);

    if (success) {
        PRINTLNS("Dynamic lock bytes OK");
    } else {
        PRINTLNS("Write process failed, please try once more.");
        PRINTLNS("Your tag is probably still fine, just remove it and put back again in 3 seconds.");
        PRINTLNS("Try a new tag if that didn't help.");
        return -5;
    }

    byte page2[] = { 0x00, 0x00, 0x00, 0x00 };

    if (! _pn532->mifareultralight_ReadPage(2, page2)) {
        return -6;
    }

    page2[2] = 0x0F;
    page2[3] = 0xE0;

    // Now we write Static lock bytes
    if (! _pn532->mifareultralight_WritePage(2, page2)) {
        return -7;
    }

    return 0;
}

int NTag215::readAmiibo() {
    _pn532->begin();

    uint8_t success;
    int inListResult = inList();

    if (inListResult != 0) {
        return inListResult;
    }

    PRINTLNS("Starting dump...");
    uint8_t password[4] = { 0xE9, 0xAF, 0x1C, 0x00 };
    uint8_t pack[2] = { 0x00, 0x00 };
    authenticate(password, pack);
    PRINTHEXV("Auth Resp", pack, 2);

    int cmdResult;


    for (uint8_t i = 0; i < AMIIBO_PAGES; i++) {
        PRINTV("Fetching block", i);
        uint16_t offset = i * 4;
        success = _pn532->mifareultralight_ReadPage(i, &data[offset]);
        PRINTHEXV("Got next block", &data[offset], 4);
        // Display the results, depending on 'success'
        if (success == 0) {
            PRINTLNS("Unable to read the requested page!");
            return -3;
        }
    }

    cmdResult = _pn532->ntag2xx_Halt();
    PRINTV("Halt result: ", cmdResult);

    PRINTLN("Done.");
    PRINTHEXV("Read tag data: ", data, NTAG215_SIZE);

    return 0;
}

uint8_t NTag215::bcc(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
    return (((b1 ^ b2) ^ b3) ^ b4);
}

bool NTag215::authenticate(const uint8_t* password, uint8_t* pack) {
    PRINTHEXV("Password: ", password, 4);

    if (_pn532->ntag2xx_Auth(password, pack) < 0) {
        PRINTLN("Auth failed.");
        return false;
    }

    PRINTLN("Auth succeeded.");

    return true;
}

bool NTag215::sendCommand(const uint8_t *cmd, uint8_t cmdLength, uint8_t* resp, uint8_t *respLen) {
    PRINTHEXV("Cmd: ", cmd, cmdLength);

    bool freePtrs = false;
    if (resp == nullptr) {
        freePtrs = true;
        resp = (uint8_t*) malloc(sizeof(uint8_t[255]));
        respLen = (uint8_t*) malloc(sizeof(uint8_t[1]));
        *respLen = 255;
    }

    if (! _pn532->inDataExchange((uint8_t*) cmd, cmdLength, resp, respLen)) {
        PRINTHEXV("sendCommand: Failed, received: ", resp, *respLen);
        return false;
    }

    PRINTHEXV("sendCommand: Success, received: ", resp, *respLen);

    if (freePtrs) {
        free(resp);
        free(respLen);
    }

    return true;
}

bool NTag215::sendCommandString(const char* command, char* response) {
    int cmdLen = strlen(command);
    uint8_t bytes[cmdLen];
    int cmdByteCount = charToByte(command, strlen(command), bytes, sizeof(bytes));

    uint8_t respBytes[256];
    uint8_t respBytesLen = 0;

    bool cmdResp = sendCommand(bytes, cmdByteCount, respBytes, &respBytesLen);

    if (response != nullptr) {
        byteToChar(respBytes, respBytesLen, response, sizeof(response));
    }

    return cmdResp;
}
