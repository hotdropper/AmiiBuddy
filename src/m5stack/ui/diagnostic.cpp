//
// Created by Jacob Mather on 8/17/20.
//

#include "diagnostic.h"
#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <M5ez.h>
#include <Esp.h>
#include <map>
#include <string>
#include <AmiiboDBAO.h>
#include <Preferences.h>
#include <FSTools.h>
#include <m5stack/classes/NTag215Magic.h>
#include "../menus.h"
#include "../init.h"
#include "../firmware.h"
#include "../utils.h"
#include "../classes/AmiiboDatabaseManager.h"
#include "../classes/NFCMonitor.h"
#include "../classes/NTag215Reader.h"

void doWipe() {
    TargetTagType type = NTag215Reader::getTagType();
    if (type != TARGET_MAGIC_NTAG_215) {
        M5ez::msgBox(TEXT_WARNING, "We did not detect a Magic NTag 21x card present.", TEXT_OK);
        return;
    }
    auto tag = (NTag215Magic*)NTag215Reader::getTag();
    tag->reset();
    M5ez::msgBox("AmiiBuddy - Wipe", "Wiped");
    NTag215Reader::releaseTag(tag);
}

void showDiagnostic() {
    char menuTitle[20] = "Diagnostics";
    strcat(menuTitle, AMIIBUDDY_VERSION);
    ezMenu myMenu(menuTitle);


    myMenu.buttons("up#Back###down#");
    uint32_t versionData = pn532.getFirmwareVersion();
    if (! versionData) {
        myMenu.addItem("A PN53x was not found.");
    } else {
        char chipset[30] = "Chip Model: PN5";
        char firmware[30] = "Firmware: ";
        sprintf(&chipset[strlen(chipset)], "%02x", (versionData >> 24) & 0xFF);
        sprintf(&firmware[strlen(firmware)], "%d", (versionData >> 16) & 0xFF);
        sprintf(&firmware[strlen(firmware)], ".%d", (versionData >> 8) & 0xFF);
        myMenu.addItem(chipset);
        myMenu.addItem(firmware);
    }

    TargetTagType type = NTag215Reader::getTagType();
    NTag215* tag = NTag215Reader::getTag(type);

    if (tag == nullptr || tag->inList() != 0) {
        myMenu.addItem("Could not detect a tag.");
    } else {
        String typeMsg("Type: ");
        switch (type) {
            case TARGET_NTAG_215:
                myMenu.addItem(typeMsg + "Standard NTag215");
                break;
            case TARGET_MAGIC_NTAG_215:
                myMenu.addItem(typeMsg + "Magic NTag21x");
                break;
        }
        String uid = String("UID: ") + tag->uidStr;
        myMenu.addItem(uid);
    }

    NTag215Reader::releaseTag(tag);

    myMenu.runOnce();
}
