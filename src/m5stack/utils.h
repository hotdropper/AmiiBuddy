//
// Created by hotdropper on 7/10/20.
//


#ifndef WIFIIBO_CLION_UTILS_H
#define WIFIIBO_CLION_UTILS_H

#include <amiibo.h>
#include <amiitool.h>
#include <functional>

const uint8_t tagUid[] = { 0x04, 0x91, 0x2F, 0xD2, 0xD5, 0x64, 0x80 };

int byteToChar(const uint8_t *src, size_t srcLen, char *dest, size_t destLen);
int charToByte(const char *src, size_t srcLen, uint8_t *dest, size_t destLen);
void processRawAmiiboKeyData(const uint8_t *amiiboKeyRawData, nfc3d_amiibo_keys *keys);
void printAmiibo(amiiboInfoStruct& info);
void printHeapUsage();

#endif //WIFIIBO_CLION_UTILS_H
