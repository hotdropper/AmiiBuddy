//
// Created by Jacob Mather on 8/17/20.
//

#include "NTag215Reader.h"
#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "NTag215Generic.h"
#include "NTag215Magic.h"

PN532* NTag215Reader::_pn532 = nullptr;
std::list<NTag215*> NTag215Reader::_ptrList = {};

void NTag215Reader::init(PN532* adapter) {
    NTag215Reader::_pn532 = adapter;
}

TargetTagType NTag215Reader::getTagType() {
    if (PN532_PRESENT == false) {
        return NO_TAG_PRESENT;
    }

    NTag215Reader::_pn532->begin();
    uint8_t uidLength;

    bool success = NTag215Reader::_pn532->inListPassiveTarget();

    if (! success) {
        PRINTLNS("We did not find an NFC Card!");
        return NO_TAG_PRESENT;
    }

    uint8_t cmd[2] = { 0x30, 0xf0 };
    uint8_t resp[20];
    memset(resp, 0x0, 20);
    uint8_t respLen = 20;

    if (! NTag215Reader::_pn532->inDataExchange((uint8_t*) cmd, 2, resp, &respLen)) {
        return TARGET_NTAG_215;
    }

    return TARGET_MAGIC_NTAG_215;
}

NTag215* NTag215Reader::getTag(TargetTagType type) {
    if (type == NO_TAG_PRESENT) {
        return nullptr;
    }

    NTag215* tag;

    switch (type) {
        case TARGET_NTAG_215:
            tag = new NTag215Generic(NTag215Reader::_pn532);
            break;
        case TARGET_MAGIC_NTAG_215:
            tag = new NTag215Magic(NTag215Reader::_pn532);
            break;
        default:
        PRINTLN("No tag type identified.");
            return nullptr;
    }

    NTag215Reader::_ptrList.push_front(tag);
    return tag;
}

void NTag215Reader::releaseTag(NTag215 *tag) {
    tag->inRelease();
    NTag215Reader::_ptrList.remove(tag);
    free(tag);
}