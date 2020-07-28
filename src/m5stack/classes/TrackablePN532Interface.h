//
// Created by hotdropper on 7/20/20.
//

#ifndef AMIIBUDDY_TRACKABLEPN532INTERFACE_H
#define AMIIBUDDY_TRACKABLEPN532INTERFACE_H

#include <PN532.h>
#include <list>
#include <functional>


class TrackablePN532Interface : public PN532Interface {
public:
    explicit TrackablePN532Interface(PN532Interface& iface);
    void begin() override;
    void wakeup() override;
    int8_t writeCommand(const uint8_t *header, uint8_t hlen, const uint8_t *body, uint8_t blen) override;
    int16_t readResponse(uint8_t buf[], uint8_t len, uint16_t timeout) override;

private:
    PN532Interface* _iface;
    std::list<uint8_t> _cmdEvents;
};


#endif //AMIIBUDDY_TRACKABLEPN532INTERFACE_H
