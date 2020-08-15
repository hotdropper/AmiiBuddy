//
// Created by Jacob Mather on 8/14/20.
//

#ifndef AMIIBUDDY_NTAG215GENERIC_H
#define AMIIBUDDY_NTAG215GENERIC_H

#include "NTag215.h"

class NTag215Generic : public NTag215 {
public:
    explicit NTag215Generic(PN532* adapter = &pn532);
    int writeAmiibo() override;

};


#endif //AMIIBUDDY_NTAG215GENERIC_H
