//
// Created by hotdropper on 7/27/20.
//

#ifndef AMIIBUDDY_AMIIBUDDYEVENTBUS_H
#define AMIIBUDDY_AMIIBUDDYEVENTBUS_H

#include <event.hpp>
#include <bus.hpp>
#include <FSTools.h>

struct EventNFCCommand: public eventpp::Event<EventNFCCommand> {
    explicit EventNFCCommand(uint8_t opCode): op{opCode} { }
    uint8_t op;
};
struct EventFSScanItem: public eventpp::Event<EventFSScanItem> {
    explicit EventFSScanItem(File* f): file{f} { }
    File* file;
};

extern eventpp::Bus<EventNFCCommand, EventFSScanItem> AmiiBuddyEventBus;

#endif //AMIIBUDDY_AMIIBUDDYEVENTBUS_H
