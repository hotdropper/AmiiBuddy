//
// Created by Jacob Mather on 8/10/20.
//

#define UNIT_TEST
#ifdef UNIT_TEST
#include "tests.h"

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include <unity.h>
#include <AmiiboDBAO.h>
#include <SD.h>
#include <M5ez.h>

#include "m5stack/menus.h"
#include "m5stack/firmware.h"
#include "m5stack/init.h"
#include "utils.h"

//INSERT INTO amiibos (name, amiibo_name, owner_name, variation, form, number, "set", head, tail, file, hash, last_written) VALUES ('Isabelle', '', '', x'00', x'01', 68, x'05', x'01810001', x'00440502', '/library/Animal Crossing/Cards/Series 1/[AC] 001 - Isabelle.bin', '596b6bf33fbbcea4d653e78738a763f0', 0)
//INSERT INTO saves (amiibo_id, hash, name, file, last_update, is_custom) VALUES (2, 'af0d18859d0b6c502d03f8a7cda9222b', 'Isabelle', '/library/Animal Crossing/Cards/Series 1/[AC] 001 - Isabelle.bin', 331, 0)
//INSERT INTO amiibos (name, amiibo_name, owner_name, variation, form, number, "set", head, tail, file, hash, last_written) VALUES ('Kiki', '', '', x'00', x'01', 101, x'05', x'02610001', x'00650502', '/library/Animal Crossing/Cards/Series 1/[AC] 034 - Kiki.bin', 'dff0306e3c4b99b6d18409929c81a488', 0)
//INSERT INTO saves (amiibo_id, hash, name, file, last_update, is_custom) VALUES (3, 'a43d8cf2ee5ae2cd6544c7029079df5d', 'Kiki', '/library/Animal Crossing/Cards/Series 1/[AC] 034 - Kiki.bin', 332, 0)
//INSERT INTO saves (amiibo_id, hash, name, file, last_update, is_custom) VALUES (2, 'b52108fab0041563e3811aaf80e6a5eb', 'Monster Hunter DLC Furniture', '/powersaves/[AC] 001 - Isabelle [Monster Hunter DLC Furniture].bin', 334, 0)
//INSERT INTO saves (amiibo_id, hash, name, file, last_update, is_custom) VALUES (2, 'a00c34b9c3b579b0ec84923b51761274', 'Video Game-Related Items DLC Furniture', '/powersaves/[AC] 001 - Isabelle [Video Game-Related Items DLC Furniture].bin', 334, 0)

AmiiboRecord aIsabelle;
const char aIsabelleHash[33] = "596b6bf33fbbcea4d653e78738a763f0";
SaveRecord sIsabelle;
const char sIsabelleHash[33] = "af0d18859d0b6c502d03f8a7cda9222b";
SaveRecord sIsabelleMH;
const char sIsabelleMHHash[33] = "b52108fab0041563e3811aaf80e6a5eb";
SaveRecord sIsabelleVG;
const char sIsabelleVGHash[33] = "a00c34b9c3b579b0ec84923b51761274";
AmiiboRecord aKiki;
const char aKikiHash[33] = "dff0306e3c4b99b6d18409929c81a488";
SaveRecord sKiki;
const char sKikiHash[33] = "a43d8cf2ee5ae2cd6544c7029079df5d";

void test_setup() {
    const uint8_t isabelleHead[4] = { 0x01, 0x81, 0x00, 0x01 };
    const uint8_t isabelleTail[4] = { 0x00, 0x44, 0x05, 0x02 };
    const uint8_t kikiHead[4] = { 0x02, 0x61, 0x00, 0x01 };
    const uint8_t kikiTail[4] = { 0x00, 0x65, 0x05, 0x02 };
    strcpy(aIsabelle.name, "Isabelle");
    aIsabelle.variation = 0;
    aIsabelle.form = 1;
    aIsabelle.number = 68;
    aIsabelle.set = 5;
    aIsabelle.setHead(isabelleHead);
    aIsabelle.setTail(isabelleTail);
    strcpy(aIsabelle.file, "/test_data/Isabelle.bin");
    strcpy(aIsabelle.hash, aIsabelleHash);

    strcpy(sIsabelle.hash, sIsabelleHash);
    strcpy(sIsabelle.name, "Isabelle");
    strcpy(sIsabelle.file, "/test_data/Isabelle.bin");
    sIsabelle.is_custom = false;
    sIsabelle.last_update = 0;

    strcpy(sIsabelleMH.hash, sIsabelleMHHash);
    strcpy(sIsabelleMH.name, "Monster Hunter DLC Furniture");
    strcpy(sIsabelleMH.file, "/test_data/Isabelle-MonsterHunter.bin");
    sIsabelleMH.is_custom = false;
    sIsabelleMH.last_update = 0;

    strcpy(sIsabelleVG.hash, sIsabelleVGHash);
    strcpy(sIsabelleVG.name, "Video Game-Related Items DLC Furniture");
    strcpy(sIsabelleVG.file, "/test_data/Isabelle-VideoGame.bin");
    sIsabelleVG.is_custom = false;
    sIsabelleVG.last_update = 0;

    strcpy(aKiki.name, "Kiki");
    aKiki.variation = 0;
    aKiki.form = 1;
    aKiki.number = 101;
    aKiki.set = 5;
    aKiki.setHead(kikiHead);
    aKiki.setTail(kikiTail);
    strcpy(aKiki.file, "/test_data/Kiki.bin");
    strcpy(aKiki.hash, aKikiHash);

    strcpy(sKiki.hash, sKikiHash);
    strcpy(sKiki.name, "Kiki");
    strcpy(sKiki.file, "/test_data/Kiki.bin");
    sKiki.is_custom = false;
    sKiki.last_update = 0;

    AmiiboDBAO::truncate();

    AmiiboDBAO::insertAmiibo(aIsabelle);
    AmiiboDBAO::insertAmiibo(aKiki);
    sIsabelle.amiibo_id = aIsabelle.id;
    sIsabelleMH.amiibo_id = aIsabelle.id;
    sIsabelleVG.amiibo_id = aIsabelle.id;
    sKiki.amiibo_id = aKiki.id;

    AmiiboDBAO::insertSave(sIsabelle);
    AmiiboDBAO::insertSave(sKiki);
    AmiiboDBAO::insertSave(sIsabelleMH);
    AmiiboDBAO::insertSave(sIsabelleVG);
    AmiiboDBAO::end();
    AmiiboDBAO::begin();
}

void test_truncate(void) {
    TEST_ASSERT_TRUE(AmiiboDBAO::truncate());
    TEST_ASSERT_TRUE(AmiiboDBAO::insertAmiibo(aIsabelle));
    TEST_ASSERT_EQUAL(true, SD.exists("/test_data/amiibos.sqlite"));
}

AmiiboRecord amiibo;

void test_find_amiibo_by_hash(void) {
//    TEST_ASSERT_TRUE(AmiiboDBAO::truncate());
//    TEST_ASSERT_TRUE(AmiiboDBAO::insertAmiibo(aIsabelle));
    PRINTV("Hash:", aIsabelleHash);
    char hash[33] = "";
    strcpy(hash, aIsabelleHash);
    printHeapUsage();
    bool res = AmiiboDBAO::findAmiiboByHash(hash, amiibo);
    TEST_ASSERT_TRUE(res);
    TEST_ASSERT_EQUAL(aIsabelle.id, amiibo.id);
}


#endif