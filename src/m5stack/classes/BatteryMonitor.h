//
// Created by hotdropper on 7/17/20.
//

#ifndef WIFIIBO_CLION_BATTERYMONITOR_H
#define WIFIIBO_CLION_BATTERYMONITOR_H

#include <M5ez.h>

#define BATTERY_MONITOR_PERCENT 1
#define BATTERY_MONITOR_VOLTAGE 2

class BatteryMonitor {
public:
    static void begin(int mode = BATTERY_MONITOR_PERCENT);
    static void restart();
    static uint16_t loop();
    static void draw(uint16_t x, uint16_t w);

private:
    static int _mode;
    static int _perc;
    static bool _chargeFlip;
};

extern BatteryMonitor batMonitor;

#endif //WIFIIBO_CLION_BATTERYMONITOR_H
