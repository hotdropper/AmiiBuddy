//
// Created by hotdropper on 7/20/20.
//

#include "TrackablePN532Interface.h"
#include "AmiiBuddyEventBus.h"

TrackablePN532Interface::TrackablePN532Interface(PN532Interface& iface) {
    _iface = &iface;
}

void TrackablePN532Interface::begin() {
    _iface->begin();
}

void TrackablePN532Interface::wakeup() {
    _iface->wakeup();
};

int8_t TrackablePN532Interface::writeCommand(const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen) {
    uint8_t cmdOp = 0;
    if (hlen < 3) {
        int pos = 2 - hlen;
        if (blen > pos) {
            cmdOp = body[pos];
        }
    } else {
        cmdOp = header[2];
    }

    int8_t ret = _iface->writeCommand(header, hlen, body, blen);

    _cmdEvents.push_back(cmdOp);

    return ret;
}

int16_t TrackablePN532Interface::readResponse(uint8_t buf[], uint8_t len, uint16_t timeout) {
    int16_t resp = _iface->readResponse(buf, len, timeout);

    for (auto op : _cmdEvents) {
        AmiiBuddyEventBus.publish<EventNFCCommand>(op);
    }

    _cmdEvents.clear();

    return resp;
}