//
// Created by hotdropper on 7/18/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "init.h"
#include <Wire.h>
#include <PN532_I2C.h>
#include <SD.h>
#include <FSTools.h>
#include <AmiiboDBAO.h>
#include <Preferences.h>
#include "../amiibuddy_constants.h"
#include "firmware.h"
#include "classes/AmiiboDatabaseManager.h"
#include "../../lib/M5ezBatteryMonitor/M5ezBatteryMonitor.h"
#include "classes/NFCMonitor.h"

PN532_I2C pn532i2c(Wire);
TrackablePN532Interface trackablePN532(pn532i2c);
PN532 pn532(trackablePN532);
bool PN532_PRESENT = false;
TargetTagType TARGET_TAG_TYPE = TARGET_MAGIC_NTAG_215;

bool foundPN532() {
    return PN532_PRESENT;
}

void initPN532() {
    pn532.begin();

    uint32_t versionData = pn532.getFirmwareVersion();
    if (! versionData) {
        PRINTLN("Didn't find PN53x board");
        return;
    }

    PN532_PRESENT = true;

    // Got ok data, print it out!
    PRINT("Found chip PN5"); PRINTLN((versionData >> 24) & 0xFF, HEX);
    PRINT("Firmware ver. "); PRINT((versionData >> 16) & 0xFF, DEC);
    PRINT('.'); PRINTLN((versionData >> 8) & 0xFF, DEC);

    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is
    // the default behaviour of the PN532.
    pn532.setPassiveActivationRetries(0xFF);

    // configure board to read RFID tags
    pn532.SAMConfig();
}

bool initSD() {
    PRINT("Initializing SD card...");

    bool sdStarted = SD.begin(TFCARD_CS_PIN);
    if (!sdStarted) {
        PRINTLN("No SD card found.");
        //sd.initErrorHalt();
    } else {
        PRINTLN("SD initialization done.");
    }

    return sdStarted;
}

void initializeDatabase() {
    AmiiboDatabaseManager::reinitialize();
}

void showInit() {
    if (! AmiiboDBAO::initialize()) {
        M5ez::msgBox(TEXT_WARNING, "Could not initialize the database.", TEXT_DISMISS);
    } else {
        ezSettings::menuObj.addItem("Re-initialize database", initializeDatabase);
    }

    nfcMonitor.begin(&pn532, &SD, &M5.Lcd);
    M5ezBatteryMonitor::begin();

    if (! atool.loadKey(KEY_FILE)) {
        M5ez::msgBox("amiiBuddy - Error", "The key in /keys/retail.bin is invalid.");
    }

    if (hasFirmwareUpdate()) {
        ezSettings::menuObj.addItem("Update firmware", firmwareMenu);
    }

    Preferences prefs;
    prefs.begin("amiiBuddy", true);	// read-only
    TARGET_TAG_TYPE = (TargetTagType) prefs.getInt("target_tag_type", TARGET_MAGIC_NTAG_215);
    prefs.end();
}

void runInit() {
    FSTools::init();
    showInit();
}