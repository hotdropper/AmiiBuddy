//
// Created by hotdropper on 7/10/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "utils.h"

#include "Arduino.h"

int charToByte(const char *src, size_t srcLen, uint8_t *dest, size_t destLen) {
    uint8_t maxLen = destLen * 2;

    if (srcLen > maxLen) {
        PRINTLN("charToByte: Too long!");
        return -1;
    }

    if (srcLen % 2 != 0) {
        PRINTLN("charToByte: odd number of chars!");
        return -2;
    }

    int destPos = 0;
    for (size_t i = 0; i < srcLen; i += 2) {
        char hexStr[] = "0x  ";
        hexStr[2] = src[i];
        hexStr[3] = src[i + 1];
        dest[destPos++] = (uint8_t) strtoul(hexStr, nullptr, 0);
    }

    return destPos;
}

int byteToChar(const uint8_t *src, size_t srcLen, char *dest, size_t destLen) {
//    PRINT("srcLen: ");
//    PRINTLN(srcLen);
//
//    PRINT("destLen: ");
//    PRINTLN(destLen);

    if ((srcLen * 2) + 1 > destLen) {
        PRINTLN("byteToChar: Too long!");
        return -1;
    }

//    char check[10];
    for (size_t i = 0; i < srcLen; i++) {
//        sprintf(check, "%02hhX", bytes[i]);
//        PRINT("byteToChar: ");
//        PRINTLN(check);
        sprintf(&dest[(i * 2)], "%02hhX", src[i]);
    }

    return strlen(dest);
}

void processRawAmiiboKeyData(const uint8_t *amiiboKeyRawData, nfc3d_amiibo_keys *keys) {
    int offset = 0;
    for (int i = 0; i < 16; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        keys->data.hmacKey[i] = amiiboKeyRawData[i + offset];
        PRINT(" ");
        PRINTLN(keys->data.hmacKey[i], HEX);
    }
    offset = 16;
    for (int i = 0; i < 14; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        PRINT(" ");
        keys->data.typeString[i] = amiiboKeyRawData[i + offset];
        PRINTLN(keys->data.typeString[i], HEX);
    }
    PRINT(amiiboKeyRawData[30], HEX);
    PRINT(" ");
    keys->data.rfu = amiiboKeyRawData[30];
    PRINTLN(keys->data.rfu, HEX);
    PRINT(amiiboKeyRawData[31], HEX);
    PRINT(" ==MBS== ");
    keys->data.magicBytesSize = amiiboKeyRawData[31];
    PRINTLN(keys->data.magicBytesSize, HEX);
    offset = 32;
    for (int i = 0; i < 16; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        PRINT(" ");
        keys->data.magicBytes[i] = amiiboKeyRawData[i + offset];
        PRINTLN(keys->data.magicBytes[i], HEX);
    }
    offset = 48;
    for (int i = 0; i < 16; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        PRINT(" ");
        keys->data.xorPad[i] = amiiboKeyRawData[i + offset];
        PRINTLN(keys->data.xorPad[i], HEX);
    }

    offset = 80;
    for (int i = 0; i < 16; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        PRINT(" ");
        keys->tag.hmacKey[i] = amiiboKeyRawData[i + offset];
        PRINTLN(keys->tag.hmacKey[i], HEX);
    }
    offset = 80 + 16;
    for (int i = 0; i < 14; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        PRINT(" ");
        keys->tag.typeString[i] = amiiboKeyRawData[i + offset];
        PRINTLN(keys->tag.typeString[i], HEX);
    }
    PRINT(amiiboKeyRawData[80 + 30], HEX);
    PRINT(" ");
    keys->tag.rfu = amiiboKeyRawData[80 + 30];
    PRINTLN(keys->tag.rfu, HEX);
    PRINT(amiiboKeyRawData[80 + 31], HEX);
    PRINT(" ==MBS== ");
    keys->tag.magicBytesSize = amiiboKeyRawData[80 + 31];
    PRINTLN(keys->tag.magicBytesSize, HEX);
    offset = 80 + 32;
    for (int i = 0; i < 16; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        PRINT(" ");
        keys->tag.magicBytes[i] = amiiboKeyRawData[i + offset];
        PRINTLN(keys->tag.magicBytes[i], HEX);
    }
    offset = 80 + 48;
    for (int i = 0; i < 16; i++) {
        PRINT(amiiboKeyRawData[i + offset], HEX);
        PRINT(" ");
        keys->tag.xorPad[i] = amiiboKeyRawData[i + offset];
        PRINTLN(keys->tag.xorPad[i], HEX);
    }
}

void printAmiibo(amiiboInfoStruct& info) {
    PRINTV("Name: ", info.amiiboName);
    PRINTV("Owner Mii Name: ", info.amiiboOwnerMiiName);
    PRINTV("Character Number: ",info.amiiboCharacterNumber);
    PRINTX("Variation: ", info.amiiboVariation);
    PRINTV("Form: ", info.amiiboForm);
    PRINTV("Number: ", info.amiiboNumber);
    PRINTX("Set: ", info.amiiboSet);
    PRINTV("Head Char: ", info.amiiboHeadChar);
    PRINTV("Tail Char: ", info.amiiboTailChar);
}

void printHeapUsage() {
    String heap(String(ESP.getFreeHeap()) + " of " + String(ESP.getHeapSize()));
    String heapPercent(" (" + String((float)ESP.getFreeHeap() / (float)ESP.getHeapSize() * 100) + "% free)");
    PRINTV("Heap usage: ", heap + heapPercent);
//    PRINTV("Free heap: ", ESP.getFreeHeap());
//    PRINTV("Total PSRAM: ", ESP.getPsramSize());
//    PRINTV("Free PSRAM: ", ESP.getFreePsram());
}
