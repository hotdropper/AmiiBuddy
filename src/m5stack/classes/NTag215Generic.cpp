//
// Created by Jacob Mather on 8/14/20.
//

#include "NTag215Generic.h"
#define PRINT_DEBUG 1
#include <ArduinoDebug.h>

NTag215Generic::NTag215Generic(PN532 *adapter) : NTag215(adapter) {
}

int NTag215Generic::writeAmiibo() {
    PRINTLN("Writing from NTag215Generic");
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
