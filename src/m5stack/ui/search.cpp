//
// Created by Jacob Mather on 8/17/20.
//

#include "search.h"
#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <M5ez.h>
#include <Esp.h>
#include <map>
#include <string>
#include <AmiiboDBAO.h>
#include <Preferences.h>
#include <FSTools.h>
#include "../menus.h"
#include "../init.h"
#include "../firmware.h"
#include "../utils.h"
#include "../classes/AmiiboDatabaseManager.h"
#include "../classes/NFCMonitor.h"
#include "../classes/NTag215Magic.h"
#include "../classes/NTag215Generic.h"
#include "write.h"

void showSearch() {
    String searchText = M5ez::textInput("Search for...");
    searchText.trim();
    if (searchText == "") {
        return;
    }

    ezMenu myMenu("Search");
    myMenu.buttons("up#Back#select##down#");
    myMenu.txtSmall();

    printHeapUsage();
    bool searchResult = AmiiboDBAO::findAmiibosByNameMatch(searchText.c_str(), [&myMenu](AmiiboRecord& amiibo) {
        auto item = String(amiibo.id);
        item = item + "|" + amiibo.name;
        myMenu.addItem(item);
    });

    if (searchResult == false) {
        M5ez::msgBox(TEXT_WARNING, "No amiibos were matched.", TEXT_OK);
        return;
    }

    myMenu.runOnce();

    if (myMenu.pickButton() == BTN_BACK) {
        return;
    }

    printHeapUsage();

    AmiiboRecord amiibo;
    int id = myMenu.pickName().toInt();
    bool lookupResult = AmiiboDBAO::getAmiiboById(id, amiibo);

    printHeapUsage();

    PRINTV("Lookup result: ", lookupResult);

    PRINTV("File path: ", amiibo.file);

    selectFile(amiibo.file);
}
