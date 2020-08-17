//
// Created by hotdropper on 7/17/20.
//

#include "M5ezBatteryMonitor.h"
#include <M5Stack.h>
#include <M5ez.h>

int M5ezBatteryMonitor::_mode = 1;
int M5ezBatteryMonitor::_perc = 0;
bool M5ezBatteryMonitor::_chargeFlip = false;

void M5ezBatteryMonitor::begin(int mode) {
    _mode = mode;
    M5ez::addEvent(M5ezBatteryMonitor::loop);
    M5ezBatteryMonitor::restart();
}

void M5ezBatteryMonitor::restart() {
    ezHeader::remove("battery");
    _chargeFlip = false;

    M5ez::setFont(M5ez::theme->clock_font);
    uint8_t width = m5.lcd.textWidth("+100%") + M5ez::theme->header_hmargin;

    ezHeader::insert(RIGHTMOST, "battery", width, M5ezBatteryMonitor::draw);
}

uint16_t M5ezBatteryMonitor::loop() {
    uint8_t batLevel = M5.Power.getBatteryLevel();

    if (batLevel != _perc || (M5.Power.isCharging() && M5.Power.isChargeFull() == false)) {
        _perc = batLevel;
        _chargeFlip = !_chargeFlip;
        ezHeader::draw("battery");
    }

    return 1000;
}

const char notChargingIndicator[] = " ";
const char chargingIndicator[] = "+";

void M5ezBatteryMonitor::draw(uint16_t x, uint16_t w) {
    char tmpl[] = "%s%s%i%%";
    char chargeIndicator[2];
    bool showIndicator = M5.Power.isChargeFull() || (M5.Power.isCharging() && _chargeFlip);
    strcpy(chargeIndicator, showIndicator ? chargingIndicator : notChargingIndicator);
    char padding[2];
    strcpy(padding, _perc < 100 ? notChargingIndicator : emptyString.c_str());
    char output[6];

    sprintf(output, tmpl, padding, chargeIndicator, _perc);

    m5.lcd.fillRect(x, 0, w, M5ez::theme->header_height, M5ez::theme->header_bgcolor);
    M5ez::setFont(M5ez::theme->clock_font);
    m5.lcd.setTextColor(M5ez::theme->header_fgcolor);
    m5.lcd.setTextDatum(TL_DATUM);
    m5.lcd.drawString(output, x + M5ez::theme->header_hmargin, M5ez::theme->header_tmargin + 2);
}

M5ezBatteryMonitor batMonitor;