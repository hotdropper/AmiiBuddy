//
// Created by hotdropper on 7/20/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "MagicNTag215.h"
#include "m5stack/utils.h"
#define ENFORCE_INLIST() { int ilr = inList(); if (ilr != 0) return ilr; }

#define CONFIG_EMPTY_TEMPLATE "A2%02hhX00000000"
#define CONFIG_1_TEMPLATE "A2%02hhX000000FF"
#define CONFIG_2_TEMPLATE "A2%02hhX00050000"

MagicNTag215::MagicNTag215(PN532 *adapter) : NTag215(adapter) {
    NTag215::setUid("04912FD2D56480");
}

int MagicNTag215::writeAmiibo() {
    PRINTLN("Fixing funky bits...");

    int base = 2 * NTAG215_PAGESIZE;
    data[base] = 0xE3;
    data[base + 1] = 0x48;
    data[base + 2] = 0x0F;
    data[base + 3] = 0xE0;

    base = 130 * NTAG215_PAGESIZE;
    data[base] = 0x01;
    data[base + 1] = 0x00;
    data[base + 2] = 0x0F;
    data[base + 3] = 0xBD;

    ENFORCE_INLIST();

    reset(uidStr);

    for (uint8_t i = 3; i < 133; i++) {
        base = i * NTAG215_PAGESIZE;
        uint8_t cmd[] = { 0xA2, i, data[base], data[base + 1], data[base + 2], data[base + 3] };

        if (! sendCommand(cmd, sizeof(cmd))) {
            return -3;
        }

        PRINTV("Wrote block", i);
    }

    PRINTLN("Writing data blocks");
    PRINTHEX(data, NTAG215_SIZE);

    PRINTLN("Setting password...");
    if (! setPassword("E9AF1C00")) {
        return -4;
    }

    PRINTLN("Setting pack...");
    if (! setPack("80800000")) {
        return -5;
    }

    PRINTLN("Fixing checksum...");
    if (! sendCommandString("A202E3480FE0")) {
        return -6;
    }

    return 0;
}

bool MagicNTag215::setUid(const char *newUid) {
    uint8_t bytes[strlen(newUid)];
    charToByte(newUid, strlen(newUid), bytes, sizeof(bytes));

    memcpy(uid, bytes, 7);

    uint8_t bcc1 = bcc(uid[0], uid[1], uid[2], 0x88);

    uint8_t bcc2 = bcc(uid[3], uid[4], uid[5], uid[6]);
    uint8_t cmdBlocks[3][6] = {
            { 0xA2, 0x00, uid[0], uid[1], uid[2], bcc1 },
            { 0xA2, 0x01, uid[3], uid[4], uid[5], uid[6] },
            { 0xA2, 0x02, bcc2, 0x48, 0x00, 0x00 },
    };

    for (auto & cmdBlock : cmdBlocks) {
        if (! sendCommand(cmdBlock, 6)) {
            return false;
        }
    }

    return true;
}

bool MagicNTag215::setVersion(const char* version) {
    uint8_t bytes[strlen(version)];
    charToByte(version, strlen(version), bytes, sizeof(bytes));

    uint8_t cmdBlocks[2][6] = {
            { 0xA2, 0xFA, bytes[0], bytes[1], bytes[2], bytes[3] },
            { 0xA2, 0xFB, bytes[4], bytes[5], bytes[6], bytes[7] },
    };

    for (auto & cmdBlock : cmdBlocks) {
        if (! sendCommand(cmdBlock, 6)) {
            return false;
        }
    }

    return true;
}

bool MagicNTag215::setSignature(const char* signature) {
    int sigLen = strlen(signature);
    uint8_t bytes[sigLen];
    charToByte(signature, strlen(signature), bytes, sizeof(bytes));

    uint8_t blockNum = 0xF2;

    int base = 0;
    for (int i = 0; i < sigLen; i += 8) {

        uint8_t cmd[] = { 0xA2, blockNum, bytes[base], bytes[base + 1], bytes[base + 2], bytes[base + 3] };

        if (! sendCommand(cmd, 6)) {
            return false;
        }

        base += 4;
        blockNum++;
    }

    return true;
}

bool MagicNTag215::setPassword(const char* password) {
    int passLen = strlen(password);
    uint8_t bytes[passLen];
    charToByte(password, strlen(password), bytes, sizeof(bytes));

    uint8_t cmd[] = { 0xA2, 0xF0, bytes[0], bytes[1], bytes[2], bytes[3] };

    return sendCommand(cmd, 6);
}

bool MagicNTag215::setPack(const char* pack) {
    int packLen = strlen(pack);
    uint8_t bytes[packLen];
    charToByte(pack, strlen(pack), bytes, sizeof(bytes));

    uint8_t cmd[] = { 0xA2, 0xF1, bytes[0], bytes[1], bytes[2], bytes[3] };

    return sendCommand(cmd, 6);
}

#define RETURN_ON_FAIL(c) { if (! c) return retCounter; retCounter++; }

int MagicNTag215::reset(const char* newUid) {
    uint8_t newUidBytes[strlen(newUid)];
    charToByte(newUid, strlen(newUid), newUidBytes, sizeof(newUidBytes));
    int retCounter = 0;

    ENFORCE_INLIST();

    RETURN_ON_FAIL(setPassword("FFFFFFFF"));
    RETURN_ON_FAIL(setPack("0000"));

    char tmpl[sizeof(CONFIG_EMPTY_TEMPLATE)];

    char cmdStrBuffer[50];
    uint8_t cmdBuffer[50];

    for (int b = 3; b <= 0xFB; b++) {
        if (b == 0x29 || b == 0x83 || b == 0xe3) {
            strcpy(tmpl, CONFIG_1_TEMPLATE);
        } else if (b == 0x2a || b == 0x84 || b == 0xe4) {
            strcpy(tmpl, CONFIG_2_TEMPLATE);
        } else {
            strcpy(tmpl, CONFIG_EMPTY_TEMPLATE);
        }

        PRINTV("Template: ", tmpl);

        sprintf(cmdStrBuffer, tmpl, b);
        charToByte(cmdStrBuffer, strlen(cmdStrBuffer), cmdBuffer, sizeof(cmdBuffer));

        if (! sendCommand(cmdBuffer, 6)) {
            return false;
        }
    }

    PRINTLN("** Setting Card Type **");
    RETURN_ON_FAIL(sendCommandString("a2fa00040402"));
    RETURN_ON_FAIL(sendCommandString("a2fb01001103"));
    RETURN_ON_FAIL(sendCommandString("a2fc01000000"));

    PRINTLN("** Setting NTAG215 default CC block456 **");
    RETURN_ON_FAIL(sendCommandString("a203e1103e00"));
    RETURN_ON_FAIL(sendCommandString("a2040300fe00"));
    RETURN_ON_FAIL(sendCommandString("a20500000000"));

    PRINTLN("** Setting cfg1/cfg2 **");
    RETURN_ON_FAIL(sendCommandString("a283000000ff"));
    RETURN_ON_FAIL(sendCommandString("a28400050000"));

    PRINTLN("** Setting uid **");
    RETURN_ON_FAIL(setUid(uidStr));

    PRINTLN("** Setting version **");
    RETURN_ON_FAIL(setVersion("0004040201001103"));

    PRINTLN("** Setting signature **");
    RETURN_ON_FAIL(setSignature("D6BE0C8B3EC1306EF6A6B8F7E0D234907101BD79867192B81FF3B0B32A5D348B"));

    PRINTLN("** Setting end blocks **");
    RETURN_ON_FAIL(sendCommandString("a22900000000"));
    RETURN_ON_FAIL(sendCommandString("a22a00000000"));
    RETURN_ON_FAIL(sendCommandString("a282000000BD"));
    RETURN_ON_FAIL(sendCommandString("a283040000FF"));

    return true;
}

int MagicNTag215::reset() {
    return reset((const char*)tagUid);
}
