//
// Created by hotdropper on 7/18/20.
//

#include "firmware.h"
#include <Esp.h>
#include <SD.h>
#include <M5ez.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <M5StackUpdater.h>
#define PRINT_DEBUG 0
#include <ArduinoDebug.h>
#include "../amiibuddy_constants.h"

SDUpdater* getSdUpdater();

void updateFirmware() {
    PRINTLN("Will Load menu binary");
    SDUpdater sdUpdater;
    sdUpdater.updateFromFS(SD, FIRMWARE_FILE);
    ESP.restart();
}

bool getDownloadUrl(char* s3Url, int s3UrlLength) {
    String releasesUrl = "https://api.github.com/repos/hotdropper/AmiiBuddy/releases/latest";
    bool foundUrl = false;

    HTTPClient http;
    http.begin(releasesUrl.c_str());
    http.setReuse(false);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        PRINTV("HTTP Response code: ", httpResponseCode);
        String payload = http.getString();
        PRINTLN(payload);
        DynamicJsonDocument doc(payload.length());
        deserializeJson(doc, payload);

        if (doc["tag_name"].as<String>() == AMIIBUDDY_VERSION) {
            M5ez::msgBox("Firmware Update", "You are running the latest version!", TEXT_OK);
            return false;
        }

        String msg("There is an update available!\n\nUpdate to ");
        msg += doc["tag_name"].as<String>() + "?";

        if (M5ez::msgBox("Firmware Update", msg, "Cancel##Update") == "Cancel") {
            return false;
        }

        JsonArray assets = doc["assets"];
        JsonObject firmwareAsset = assets[0];
        for (const auto asset : assets) {
            if (asset["name"].as<String>().endsWith(".bin")) {
                firmwareAsset = asset;
            }
        }

        if (firmwareAsset["name"].as<String>().endsWith(".bin") == false) {
            M5ez::msgBox(TEXT_WARNING, "Release did not contain a bin! Skipping update.", TEXT_OK);
            return false;
        }

        String downloadUrl = firmwareAsset["browser_download_url"].as<String>();

        http.end();
        http.setReuse(false);
        PRINTV("Asset Download URL: ", downloadUrl);

        const char* STATUS_HEADER = "status";
        const char* LOCATION_HEADER = "location";
        const char* headersToCollect[] = { STATUS_HEADER, LOCATION_HEADER };
        int headerCount = 2;

        http.begin(downloadUrl.c_str());
        http.setReuse(false);
        http.collectHeaders(headersToCollect, headerCount);

        int response2 = http.GET();

        PRINTV("Response: ", response2);
        PRINTV("Status: ", http.header("status"));

        for (int i = 0; i < http.headers(); i++) {
            PRINTV("Header name:", http.headerName(i));
            PRINTV("Header value:", http.header(i));
        }

        if (http.header("status") == "302 Found") {
            strncpy(s3Url, http.header("location").c_str(), s3UrlLength);
            PRINTV("Download url: ", s3Url);
            foundUrl = true;
        }

        http.end();
        doc.clear();
        doc.shrinkToFit();
    } else {
        http.end();
    }

    return foundUrl;
}

void searchForFirmwareUpdate() {
    if (WiFiClass::status() != WL_CONNECTED) {
        M5ez::msgBox(TEXT_WARNING, "You are not connected to Wifi.", TEXT_OK);
        return;
    }
    char s3Url[768];
    if (getDownloadUrl(s3Url, sizeof(s3Url))) {
        ezProgressBar progress_bar("OTA update in progress", "Downloading ...", "Abort");
        if (ezWifi::update(s3Url, BALTIMORE_ROOT_CERT, &progress_bar)) {
            M5ez::msgBox("Over The Air updater", "OTA download successful. Reboot to new firmware", "Reboot");
            ESP.restart();
        } else {
            M5ez::msgBox("OTA error", ezWifi::updateError(), "OK");
        }
    }
    else {
        M5ez::msgBox("OTA error", "Unable to find update.", "OK");
    }
    // Free resources
}

bool hasFirmwareUpdate() {
    return SD.exists(FIRMWARE_FILE);
}

void firmwareMenu() {
    ezMenu myMenu("Update Firmware");
    myMenu.txtSmall();
    myMenu.buttons("up#Back#select##down#");

    myMenu.addItem("CheckOnline|Check online for update");
    if (hasFirmwareUpdate()) {
        myMenu.addItem("SDCard|Reload /sd/firmware.bin");
    }

    myMenu.runOnce();

    if (myMenu.pickButton() != "select") {
        return;
    }

    if (myMenu.pickName() == "SDCard") {
        return updateFirmware();
    }

    if (myMenu.pickName() == "CheckOnline") {
        searchForFirmwareUpdate();
    }
}