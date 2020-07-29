//#define USE_SDFAT
#include <ArduinoDebug.h>
#include <M5ez.h>

#include <SD.h>

#include "menus.h"
#include "firmware.h"
#include "init.h"

void setup() {
    M5ez::begin();
    M5.Power.begin();
    initPN532();
    initSD();

    if(digitalRead(BUTTON_A_PIN) == 0) {
      updateFirmware();
    }

    runInit();
}

void loop() {
    showMainMenu();
}