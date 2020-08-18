//
// Created by hotdropper on 7/18/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <M5ez.h>
#include <Esp.h>
#include <Preferences.h>
#include "menus.h"
#include "init.h"
#include "FSTools.h"
#include "utils.h"
#include "classes/AmiiboDatabaseManager.h"
#include "ui/diagnostic.h"
#include "ui/read.h"
#include "ui/search.h"

void selectDataSet() {
    ezMenu myMenu("Select dataset");
    myMenu.buttons("up#Back#select##down#");

    int count = FSTools::getFileCount("/sd/amiibos/test");
    bool usingTestAmiibos = (count == 0);
    char fullAmiiboSet[40] = "";
    char testAmiiboSet[40] = "";
    sprintf(fullAmiiboSet, TMPL_QUAD_STR, "Full", "|", usingTestAmiibos ? "  " : "* ", TEXT_FULL_AMIIBO_SET);
    sprintf(testAmiiboSet, TMPL_QUAD_STR, "Test", "|", usingTestAmiibos ? "* " : "  ", TEXT_TEST_AMIIBO_SET);

    myMenu.addItem(fullAmiiboSet);
    myMenu.addItem(testAmiiboSet);

    myMenu.runOnce();

    if (myMenu.pickName() == BTN_BACK) {
        return;
    }

    printHeapUsage();
    if (myMenu.pickName() == "Full" && usingTestAmiibos) {
        FSTools::rename(LIBRARY_PATH, "/sd/amiibos/test/library");
        FSTools::rename(POWER_SAVES_PATH, "/sd/amiibos/test/powersaves");
        FSTools::rename("/sd/amiibos/full/library", LIBRARY_PATH);
        FSTools::rename("/sd/amiibos/full/powersaves", POWER_SAVES_PATH);
        String msg(TMPL_SWITCHED_AMIIBO_SET);
        msg = msg + TEXT_FULL_AMIIBO_SET;
        M5ez::msgBox(TEXT_COMPLETE, msg, TEXT_OK);
        return;
    }

    if (myMenu.pickName() == "Test" && ! usingTestAmiibos) {
        FSTools::rename(LIBRARY_PATH, "/sd/amiibos/full/library");
        FSTools::rename(POWER_SAVES_PATH, "/sd/amiibos/full/powersaves");
        FSTools::rename("/sd/amiibos/test/library", LIBRARY_PATH);
        FSTools::rename("/sd/amiibos/test/powersaves", POWER_SAVES_PATH);
        String msg(TMPL_SWITCHED_AMIIBO_SET);
        msg = msg + TEXT_TEST_AMIIBO_SET;
        M5ez::msgBox(TEXT_COMPLETE, msg, TEXT_OK);
        return;
    }
    printHeapUsage();

}

bool initialized = false;

void showMainMenu() {
    if (initialized == false) {
        ezSettings::menuObj.addItem("Select data set", selectDataSet);
        initialized = true;
    }
    ezMenu myMenu("AmiiBuddy");
    myMenu.addItem("Browse", showBrowse);
    myMenu.addItem("Search", showSearch);
    myMenu.addItem("Read", doRead);

    if (PN532_PRESENT) {
        myMenu.addItem("Diagnostic", showDiagnostic);

    }
    myMenu.addItem("Settings", ezSettings::menu);
    myMenu.run();
}
