//
// Created by Jacob Mather on 8/17/20.
//

#include "browse.h"
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

bool goHome = false;
char lastPath[200] = LIBRARY_PATH;
char adjustedPath[200] = "";

void showBrowse(const char *path) {
    M5ez::yield();
    PRINTLN(path);

    if (goHome) {
        return;
    }

    strcpy(lastPath, path);
    FSTools::getFSByPath(path, adjustedPath);

    ezMenu myMenu("Browse");
    myMenu.txtSmall();
    myMenu.buttons("up#Back#select##down#Home");
    char filename[256];

    String partialPath = String(path);
    int lastSlash = partialPath.lastIndexOf("/");
    partialPath = partialPath.substring(lastSlash + 1);
    ezProgressBar pb("Listing...", String("Finding files in ") + partialPath);
    M5ez::yield();

    if (! FSTools::exists(path)) {
        char msg[80] = "Could not find: ";
        strcat(msg, path);
        M5ez::msgBox("Error", msg);
        return;
    }

    int counter = -15;

    FSTools::traverseEntries(path, false, [&counter, &pb, &filename, &myMenu, &path](File* directory, File* entry) {
        counter++;
        if (counter > 25) {
            counter = 0;
        }

        if (counter >= 0) {
            float val = (float)counter / (float)25 * 100;
            pb.value(val);
            M5ez::redraw();
        }

        if (counter % 5 == 0) {
            M5ez::yield();
        }

        PRINTV("Path: ", path);
        PRINTV("Path Len: ", strlen(adjustedPath));
        PRINTV("File path: ", entry->name());
        PRINTV("File name: ", &entry->name()[strlen(adjustedPath) + 1]);

        if (entry->isDirectory()) {
            strncpy(filename, "./", sizeof(filename));
            strncat(filename, &entry->name()[strlen(adjustedPath) + 1], (sizeof(filename) - strlen(filename) - 1));
            myMenu.addItem(filename);
        } else {
            if (strncmp(&entry->name()[strlen(adjustedPath) + 1], ".DS_Store", strlen(".DS_Store")) == 0) {
                return;
            }
            strncpy(filename, &entry->name()[strlen(adjustedPath) + 1], sizeof(filename));
            myMenu.addItem(filename);
        }
    });

    M5ez::yield();
    myMenu.runOnce();
    M5ez::yield();

//    PRINTV("Selected item:", myMenu.pick());
//    PRINTV("Selected item name:", myMenu.pickName());
//    PRINTV("Selected item caption:", myMenu.pickCaption());
//    PRINTV("Selected item button:", myMenu.pickButton());

    if (myMenu.pickButton() == BTN_BACK) {
        String strPath(path);
        const size_t last_slash_idx = strPath.lastIndexOf("/");
        String priorPath = strPath.substring(0, last_slash_idx);
        if (priorPath == "/" || priorPath == "" || priorPath == "/sd") {
            return;
        }

        M5ez::yield();

        return showBrowse(priorPath.c_str());
    }

    if (myMenu.pickButton() == "Home") {
        goHome = true;
        return;
    }

    if (strncmp(myMenu.pickName().c_str(), "./", 2) == 0) {
        strncpy(filename, path, sizeof(filename));
        strncat(filename, &myMenu.pickName().c_str()[1], (sizeof(filename) - strlen(filename) - 1));
        M5ez::yield();
        return showBrowse(filename);
    } else {
        strncpy(filename, path, sizeof(filename));
        M5ez::yield();
        strncat(filename, "/", (sizeof(filename) - strlen(filename) - 1));
        M5ez::yield();
        strncat(filename, myMenu.pickName().c_str(), (sizeof(filename) - strlen(filename) - 1));
        M5ez::yield();
        PRINTV("File: ", filename);

        selectFile(filename);
    }
}

void showBrowse() {
    goHome = false;
    PRINTV("Last path:", lastPath);
    showBrowse(lastPath);
}
