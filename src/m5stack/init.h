//
// Created by hotdropper on 7/18/20.
//

#ifndef AMIIBUDDY_INIT_H
#define AMIIBUDDY_INIT_H

#include <PN532.h>
#include <FS.h>
#include "classes/TrackablePN532Interface.h"

extern TrackablePN532Interface trackablePN532;
extern PN532 pn532;
extern bool PN532_PRESENT;
enum TargetTagType {
    TARGET_NTAG_215,
    TARGET_MAGIC_NTAG_215,
};
extern TargetTagType TARGET_TAG_TYPE;


bool initSD();
void initPN532();
void runInit();

#endif //AMIIBUDDY_INIT_H
