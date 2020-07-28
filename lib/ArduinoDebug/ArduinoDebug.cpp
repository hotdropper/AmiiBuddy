//
// Created by hotdropper on 7/18/20.
//

#include "ArduinoDebug.h"
#include "Arduino.h"

void printHexDump(const uint8_t *data, int dataLength, int bytesPerLine) {
    for (int i=0; i < dataLength; i++)
    {
        if (i % bytesPerLine == 0 && i != 0) {
            Serial.println("");
        } else if (i != 0) {
            Serial.print(" ");
        }

        if (data[i] < 16) {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
    }
    Serial.println("");
}
