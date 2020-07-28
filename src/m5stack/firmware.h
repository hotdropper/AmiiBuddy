//
// Created by hotdropper on 7/18/20.
//

#ifndef AMIIBUDDY_FIRMWARE_H
#define AMIIBUDDY_FIRMWARE_H

#define AMIIBUDDY_VERSION "0.1.1"
#define FIRMWARE_FILE "/firmware.bin"

#include <M5StackUpdater.h>

SDUpdater* getSdUpdater();
bool hasFirmwareUpdate();
void updateFirmware();

#endif //AMIIBUDDY_FIRMWARE_H
