//
// Created by hotdropper on 7/17/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "BatteryMonitor.h"
#include <M5Stack.h>
#include <M5ez.h>

int BatteryMonitor::_mode = 1;
int BatteryMonitor::_perc = 0;
bool BatteryMonitor::_chargeFlip = false;

void BatteryMonitor::begin(int mode) {
    _mode = mode;
    ez.addEvent(batMonitor.loop);
    batMonitor.restart();
}

void BatteryMonitor::restart() {
    ez.header.remove("battery");
    _chargeFlip = false;
    uint8_t length = 5;

    ez.setFont(ez.theme->clock_font);
    uint8_t width = length * m5.lcd.textWidth("5") + ez.theme->header_hmargin * 2;

    PRINTV("Char width: ", m5.lcd.textWidth("5"));

    ez.header.insert(RIGHTMOST, "battery", width, batMonitor.draw);
}

uint16_t BatteryMonitor::loop() {
    int batLevel = M5.Power.getBatteryLevel();

    if (batLevel != _perc || (M5.Power.isCharging() && M5.Power.isChargeFull() == false)) {
        _perc = batLevel;
        _chargeFlip = !_chargeFlip;
        ez.header.draw("battery");
    }

    return 1000;
}

const char notChargingIndicator[] = " ";
const char chargingIndicator[] = "+";

void BatteryMonitor::draw(uint16_t x, uint16_t w) {
    char tmpl[] = "%s%s%i%%";
    char chargeIndicator[2];
    bool showIndicator = M5.Power.isChargeFull() || (M5.Power.isCharging() && _chargeFlip);
    strcpy(chargeIndicator, showIndicator ? chargingIndicator : notChargingIndicator);
    char padding[2];
    strcpy(padding, _perc < 100 ? notChargingIndicator : emptyString.c_str());
    char output[6];

    sprintf(output, tmpl, padding, chargeIndicator, _perc);

    m5.lcd.fillRect(x, 0, w, ez.theme->header_height, ez.theme->header_bgcolor);
    ez.setFont(ez.theme->clock_font);
    m5.lcd.setTextColor(ez.theme->header_fgcolor);
    m5.lcd.setTextDatum(TL_DATUM);
    m5.lcd.drawString(output, x + ez.theme->header_hmargin, ez.theme->header_tmargin + 2);
}

BatteryMonitor batMonitor;