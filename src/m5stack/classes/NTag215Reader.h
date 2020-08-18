//
// Created by Jacob Mather on 8/17/20.
//

#ifndef AMIIBUDDY_NTAG215READER_H
#define AMIIBUDDY_NTAG215READER_H

#include "NTag215.h"
#include <PN532.h>
#include "amiibuddy_constants.h"

class NTag215Reader {
public:
    static void init(PN532* adapter);
    static TargetTagType getTagType();
    static NTag215* getTag(TargetTagType);
    static NTag215* getTag() {
        return getTag(getTagType());
    }
    static void releaseTag(NTag215* tag);

private:
    static PN532* _pn532;
    static std::list<NTag215*> _ptrList;
};


#endif //AMIIBUDDY_NTAG215READER_H
