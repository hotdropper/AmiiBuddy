//
// Created by hotdropper on 7/17/20.
//

#ifndef WIFIIBO_CLION_NFCPROGRESSTRACKER_H
#define WIFIIBO_CLION_NFCPROGRESSTRACKER_H

#include <M5ez.h>
#include <M5Display.h>
#include <PN532.h>
#include <amiitool.h>
#include "NTag215.h"

enum NFCMode {
    INIT,
    UNKNOWN,
    NO_READER,
    SEARCHING,
    NO_CARD,
    CARD_LOADED,
    READING,
    WRITING,
};

class NFCMonitor {
public:
    NFCMonitor();
    ezProgressBar* _update_progressbar;
    void begin(PN532* nfc, FS* fs, M5Display* tft);
    void restart();
    int monitorAmiiboWrite(ezProgressBar* pb, NTag215* tag);
    bool running();
    bool isBusy();
    bool isAvailable();
    void stop();
    uint16_t loop();
    void draw(uint16_t x, uint16_t w);
    int monitorAmiiboRead(ezProgressBar* pb, NTag215* tag);

private:
    uint16_t loopProgress();
    uint16_t loopState();
    void loadSvg(const char* fileName, char* dest);
    int _avgCount;
    int _count;
    bool _running;
    PN532* _nfc;
    FS* _fs;
    M5Display* _tft;
    NFCMode _state;
    NFCMode _lastState;
    char* _svg_journal;
    char* _svg_question;
};

extern NFCMonitor nfcMonitor;

#endif //WIFIIBO_CLION_NFCPROGRESSTRACKER_H
