//
// Created by hotdropper on 7/20/20.
//

#ifndef AMIIBUDDY_TAG_H
#define AMIIBUDDY_TAG_H
#include <PN532.h>
#include <amiitool.h>
#include "../init.h"

class NTag215 {
public:
    uint8_t uid[7] = { 0, 0, 0, 0, 0, 0, 0 };
    char uidStr[15] = "";
    uint8_t data[NTAG215_SIZE] = { 0 };

    explicit NTag215(PN532* adapter = &pn532);

    void setPN532(PN532* adapter);
    virtual bool setUid(const char *newUid);
    virtual int writeAmiibo();
    int readAmiibo();
    bool sendCommandString(const char* command, char* response = nullptr);
    bool sendCommand(const uint8_t* cmd, uint8_t cmdLength, uint8_t* resp = nullptr, uint8_t *respLen = nullptr);
    bool authenticate(const uint8_t* password, uint8_t* pack);
    int inList();
    int inRelease();

protected:
    PN532* _pn532;
    bool _inListed = false;
    uint8_t bcc(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
};


#endif //AMIIBUDDY_TAG_H
