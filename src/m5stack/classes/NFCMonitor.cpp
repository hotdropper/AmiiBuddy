//
// Created by hotdropper on 7/17/20.
//

#include <mutex>
#include "AmiiBuddyEventBus.h"
#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "NFCMonitor.h"
#include <Preferences.h>
#include "../init.h"
#include <FSTools.h>
#include "m5stack/utils.h"

ezProgressBar* nfcProgressBar;

class NFCEventListener {
public:
    NFCEventListener() {
        enabled = false;
        op = 0;
        count = 0;
        expectedTotal = 0;
    };

    void update() {
        float progress = (float)count / (float)expectedTotal * 100;
        PRINTV("Progress: ", progress);

        if (nfcProgressBar == nullptr) {
            return;
        }

        nfcProgressBar->value(progress);
        M5ez::redraw();
    };

    void receive(const EventNFCCommand& evt) {
        if (enabled && evt.op == op) {
            count++;
            update();
        }
    };

    void start(uint8_t opCode, int expectedCount) {
        op = opCode;
        count = 0;
        expectedTotal = expectedCount;
        enabled = true;
    };

    int stop() {
        enabled = false;
        PRINTHEXV("NFC Operation: ", &op, 1);
        PRINTV("NFC Operation Count: ", count);
        free(nfcProgressBar);
        nfcProgressBar = nullptr;
        return count;
    };

    bool enabled = false;
    uint8_t op = 0;
    int count;
    int expectedTotal;
};

NFCEventListener nfcEventListener;

NFCMonitor nfcMonitor;

NFCMonitor::NFCMonitor() {
    _nfc = nullptr;
    _update_progressbar = nullptr;
    _count = 0;
    _avgCount = 0;
    _running = false;
    _state = UNKNOWN;
    _lastState = INIT;
    _svg_journal = nullptr;
    _svg_question = nullptr;
}

void fireListener(const EventNFCCommand& evt) {
    nfcEventListener.receive(evt);
}

void NFCMonitor::begin(PN532* nfc, FS* fs, M5Display* tft) {
    _nfc = nfc;
    _fs = fs;
    _tft = tft;
    AmiiBuddyEventBus.add<EventNFCCommand, fireListener>();

//    M5ez::addEvent(runNfcMonitorLoop);
//    restart();
}



bool NFCMonitor::isBusy() {
    return _state == SEARCHING || _state == READING || _state == WRITING;
}

bool NFCMonitor::isAvailable() {
    return _state == CARD_LOADED;
}

uint16_t runNfcMonitorLoop() {
    return nfcMonitor.loop();
}

int NFCMonitor::monitorAmiiboRead(ezProgressBar *pb, NTag215 *tag) {

    _state = READING;

    nfcProgressBar = pb;

    nfcEventListener.start(0x30, AMIIBO_PAGES);

    int result = tag->readAmiibo();

    nfcEventListener.stop();

    _state = CARD_LOADED;
    return result;
}

int NFCMonitor::monitorAmiiboWrite(ezProgressBar* pb, NTag215* tag) {
    nfcProgressBar = pb;
    Preferences prefs;
    prefs.begin("amiiBuddy", true);	// read-only
    _avgCount = prefs.getInt("average_count", 385);
    prefs.end();

    PRINTV("Average count: ", _avgCount);

    _count = 0;
    _running = true;
    _state = WRITING;
    nfcEventListener.start(0xA2, _avgCount);

    int res = tag->writeAmiibo();

    int totalCount = nfcEventListener.stop();

    _running = false;
    _state = CARD_LOADED;

    prefs.begin("amiiBuddy", false);	// read-only
    prefs.putInt("average_count", (_avgCount + totalCount) / 2);
    prefs.end();

    return res;
}

void NFCMonitor::loadSvg(const char* fileName, char* dest) {
//    if (! AMIIBUDDY_FS_DEFAULT.exists(fileName)) {
//        return;
//    }
//
//    File f = AMIIBUDDY_FS_DEFAULT.open(fileName, FILE_READ);
//    size_t s = f.size();
//    f.readBytes(dest, s);
//    f.close();
//    dest[s] = 0;
}

void redraw(uint16_t x, uint16_t w) {
//    nfcMonitor.draw(x, w);
}

void NFCMonitor::restart() {
//    ezHeader::remove("nfc");
//    uint8_t width = 20 + M5ez::theme->header_hmargin * 2;
//    ezHeader::insert(RIGHTMOST, "nfc", width, redraw);
}


void NFCMonitor::stop() {
    _running = false;
    _state = UNKNOWN;
}

bool NFCMonitor::running() {
    return _running;
}

uint16_t NFCMonitor::loop() {
    PRINTLN("NFCMonitor::loop");
    if (! _running) {
        return 0;
    }

    return loopProgress();
}

uint16_t NFCMonitor::loopState() {
    if (! isBusy()) {
        return 0;
    }

    return 250;
}

uint16_t NFCMonitor::loopProgress() {
    PRINTLN("NFCMonitor::loopProgress running");

    PRINTV("Current count: ", _count);

    float progress = (float)_count / (float)_avgCount * 100;
    _update_progressbar->value(progress);

    PRINTV("Progress: ", progress);

    M5ez::redraw();

    return 50;
}

void NFCMonitor::draw(uint16_t x, uint16_t w) {
//
//    m5.lcd.fillRect(x, 0, w, ez.theme->header_height, ez.theme->header_fgcolor);
//    Serial.println("Loading file into parser");
//    _svg_parser->setFile(_svg_question, strlen(_svg_question));
//    Serial.println("Printing");
//    Serial.print("X: ");
//    Serial.println(x);
//    _svg_parser->print(x, 0);
//    if (_state != _lastState) {
//
//        if (AMIIBUDDY_FS_DEFAULT.exists("/png/question.png")) {
//            Serial.println("File exists.");
//        } else {
//            Serial.println("File not found.");
//        }
//
//        _tft->drawPngFile(
//                *_fs,
//                "/png/question.png",
//                x, 0,
//                20,
//                ez.theme->header_height,
//                0,
//                0,
//                2
//                );
//        _lastState = _state;
//    }
}
