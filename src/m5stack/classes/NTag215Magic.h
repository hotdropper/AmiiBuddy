//
// Created by hotdropper on 7/20/20.
//

#ifndef AMIIBUDDY_NTAG215MAGIC_H
#define AMIIBUDDY_NTAG215MAGIC_H

#include "NTag215.h"

class NTag215Magic : public NTag215 {
public:
    explicit NTag215Magic(PN532* adapter = &pn532);

    int reset(const char* newUid);
    int reset();
    bool wipe();
    int writeAmiibo() override;
    bool setUid(const char* newUid) override;
    bool setVersion(const char* version);
    bool setSignature(const char* signature);
    bool setPassword(const char* password);
    bool setPack(const char* pack);
private:
};


#endif //AMIIBUDDY_NTAG215MAGIC_H
