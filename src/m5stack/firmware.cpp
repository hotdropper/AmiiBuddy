//
// Created by hotdropper on 7/18/20.
//

#include "firmware.h"
#include <Esp.h>
#include <SD.h>
#include <ArduinoDebug.h>

void updateFirmware() {
    PRINTLN("Will Load menu binary");
    SDUpdater sdUpdater;
    sdUpdater.updateFromFS(SD, FIRMWARE_FILE);
    ESP.restart();
}

bool hasFirmwareUpdate() {
    return SD.exists(FIRMWARE_FILE);
}