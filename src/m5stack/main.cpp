//#define USE_SDFAT
#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <M5ez.h>

#include <SD.h>

#include "menus.h"
#include "firmware.h"
#include "init.h"

void setup() {
    M5ez::begin();
    M5.Power.begin();
    PRINTV("AmiiBuddy Version ", AMIIBUDDY_VERSION);
    initPN532();
    initSD();

    if(digitalRead(BUTTON_A_PIN) == 0) {
      updateFirmware();
    }

    runInit();
}

#ifndef UNIT_TEST
void loop() {
    showMainMenu();
}
#endif