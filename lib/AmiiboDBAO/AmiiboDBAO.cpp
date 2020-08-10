//
// Created by Jacob Mather on 7/31/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "AmiiboDBAO.h"
#include <sqlite3.h>
#include <MD5Builder.h>
#include <FS.h>

//#define DEBUG_SQL

#define KEY_FILE "/sd/keys/retail.bin"

#ifdef UNIT_TEST
#define SQLITE_DB_PATH "/sd/test_data/amiibos.sqlite"
#define FS_DB_PATH "/test_data/amiibos.sqlite"
#else
#define SQLITE_DB_PATH "/sd/db/amiibos.sqlite"
#define FS_DB_PATH "/db/amiibos.sqlite"
#endif

#define AMIIBO_FIELDS "amiibos.id, amiibos.name, amiibos.amiibo_name, amiibos.owner_name, amiibos.variation, amiibos.form, amiibos.number, amiibos.\"set\", amiibos.head, amiibos.tail, amiibos.file, amiibos.hash, amiibos.last_written"
#define SAVE_FIELDS "saves.id, saves.amiibo_id, saves.hash, saves.name, saves.file, saves.last_update, saves.is_custom"

extern FS SD;
extern const char amiibos_sql_start[] asm("_binary_db_amiibos_sql_start");
extern const char saves_sql_start[] asm("_binary_db_saves_sql_start");
const prog_char PROGMEM SQL_INSERT_AMIIBO[] = "INSERT INTO amiibos (name, amiibo_name, owner_name, variation, form, number, \"set\", head, tail, file, hash, last_written) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
const prog_char PROGMEM SQL_INSERT_SAVE[] = "INSERT INTO saves (amiibo_id, hash, name, file, last_update, is_custom) VALUES (?, ?, ?, ?, ?, ?)";
const prog_char PROGMEM SQL_SELECT_AMIIBO_ID_BY_HASH[] = "SELECT id FROM amiibos WHERE hash = ?";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_FILE_LIKE[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE file LIKE ? ORDER BY last_written DESC";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_ID[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE id = ?";
const prog_char PROGMEM SQL_SELECT_SAVE_BY_ID[] = "SELECT " SAVE_FIELDS " FROM saves WHERE id = ?";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_HASH[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE hash = ?";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_FILE_NAME[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE file = ?";
const prog_char PROGMEM SQL_SELECT_SAVE_BY_AMIIBO_HASH[] = "SELECT " SAVE_FIELDS " FROM saves INNER JOIN amiibos a on a.id = saves.amiibo_id WHERE a.hash = ? ORDER BY saves.last_update DESC";
const prog_char PROGMEM SQL_SELECT_SAVE_BY_AMIIBO_FILE[] = "SELECT " SAVE_FIELDS " FROM saves INNER JOIN amiibos a on a.id = saves.amiibo_id WHERE a.file = ? ORDER BY saves.last_update DESC";
const prog_char PROGMEM SQL_UPDATE_SAVE_TIMESTAMP_BY_ID[] = "UPDATE saves SET last_update = ? WHERE id = ?";
const prog_char PROGMEM SQL_UPDATE_AMIIBO_TIMESTAMP_BY_ID[] = "UPDATE amiibos SET last_written = ? WHERE id = ?";

sqlite3 *dbh;
AmiiboHashDetails amiiboHashDetailsCache;
auto * amiiboHashDetailsCacheBytes = reinterpret_cast<uint8_t*>(&amiiboHashDetailsCache);
MD5Builder md5Builder = MD5Builder();
sqlite3_stmt *res;
const char *tail;
int rc = 0;
char adbaoHashCache[33];
const char* data = "Callback function called";
char *zErrMsg = 0;
SaveRecord saveRecordBuff = SaveRecord();
AmiiboRecord amiiboRecordBuff = AmiiboRecord();
int i;
int countBuff;
unsigned long start;

static int callback(void *data, int argc, char **argv, char **azColName){
    Serial.printf("%s: ", (const char*)data);
    for (i = 0; i<argc; i++){
        Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    Serial.printf("\n");
    return 0;
}

int openDb(const char *filename, sqlite3 **db) {
    rc = sqlite3_open(filename, db);
    if (rc) {
        Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
        return rc;
    } else {
        Serial.printf("Opened database successfully\n");
    }
    return rc;
}

int db_exec(sqlite3 *db, const char *sql) {
#ifdef DEBUG_SQL
    Serial.println(sql);
#endif

    start = micros();
    rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        Serial.printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        Serial.printf("Operation done successfully\n");
    }
    Serial.print(F("Time taken:"));
    Serial.println(micros()-start);
    return rc;
}

bool AmiiboDBAO::begin() {
    sqlite3_initialize();

    if (openDb(SQLITE_DB_PATH, &dbh)) {
        return false;
    }

    return true;
}

bool AmiiboDBAO::end() {
    sqlite3_close(dbh);
    return true;
}

bool AmiiboDBAO::insertAmiibo(AmiiboRecord& amiibo) {
    rc = sqlite3_prepare_v2(dbh, SQL_INSERT_AMIIBO, strlen(SQL_INSERT_AMIIBO), &res, &tail);
    if (rc != SQLITE_OK) {
        Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

    sqlite3_bind_text(res, 1, amiibo.name, strlen(amiibo.name), SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 2, amiibo.amiiboName, strlen(amiibo.ownerName), SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 3, amiibo.ownerName, strlen(amiibo.ownerName), SQLITE_TRANSIENT);
    sqlite3_bind_blob(res, 4, &amiibo.variation, 1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(res, 5, &amiibo.form, 1, SQLITE_TRANSIENT);
    sqlite3_bind_int(res, 6, amiibo.number);
    sqlite3_bind_blob(res, 7, &amiibo.set, 1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(res, 8, &amiibo.head, 4, SQLITE_TRANSIENT);
    sqlite3_bind_blob(res, 9, &amiibo.tail, 4, SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 10, amiibo.file, strlen(amiibo.file), SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 11, amiibo.hash, 32, SQLITE_TRANSIENT);
    sqlite3_bind_int(res, 12, amiibo.last_written);

#ifdef DEBUG_SQL
    char* sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("INSERT:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    if (sqlite3_step(res) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

//    PRINTV("Inserting hash:", amiibo.hash);

    sqlite3_clear_bindings(res);
    sqlite3_reset(res);
    amiibo.id = sqlite3_last_insert_rowid(dbh);
    sqlite3_finalize(res);
    PRINTV("Inserted amiibo id:", amiibo.id);

    return true;
}

bool AmiiboDBAO::insertSave(SaveRecord& save) {
    rc = sqlite3_prepare_v2(dbh, SQL_INSERT_SAVE, strlen(SQL_INSERT_SAVE), &res, &tail);
    if (rc != SQLITE_OK) {
        Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

    sqlite3_bind_int(res, 1, save.amiibo_id);
    sqlite3_bind_text(res, 2, save.hash, 32, SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 3, save.name, strlen(save.file), SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 4, save.file, strlen(save.file), SQLITE_TRANSIENT);
    sqlite3_bind_int(res, 5, save.last_update);
    sqlite3_bind_int(res, 6, save.is_custom);

#ifdef DEBUG_SQL
    char* sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("INSERT:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    if (sqlite3_step(res) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        return false;
    }
    sqlite3_clear_bindings(res);
    sqlite3_reset(res);
    save.id = sqlite3_last_insert_rowid(dbh);
    sqlite3_finalize(res);
    PRINTV("Inserted save id:", save.id);

    return true;
}

bool AmiiboDBAO::truncate() {
    sqlite3_close(dbh);
    SD.remove(FS_DB_PATH);
    openDb(SQLITE_DB_PATH, &dbh);

    rc = db_exec(dbh, amiibos_sql_start);
    if (rc != SQLITE_OK) {
        return false;
    }

    rc = db_exec(dbh, saves_sql_start);
    if (rc != SQLITE_OK) {
        return false;
    }

    return true;
}


void AmiiboDBAO::calculateSaveHash(uint8_t* tagData, char* checksum) {
    md5Builder.begin();
    md5Builder.add((uint8_t*) &tagData[AMIIBO_SAVE_CHECKSUM_POS], AMIIBO_SAVE_CHECKSUM_BYTE_LEN);
    md5Builder.calculate();
    return md5Builder.getChars(checksum);
}

void AmiiboDBAO::calculateAmiiboInfoHash(uint8_t* tagData, char* hash) {
    atool.loadKey(KEY_FILE);
    atool.loadFileFromData(tagData, NTAG215_SIZE, false);
    return AmiiboDBAO::calculateAmiiboInfoHash(atool.amiiboInfo, hash);
}


void AmiiboDBAO::calculateAmiiboInfoHash(const amiiboInfoStruct& amiiboInfo, char* hash) {
    amiiboHashDetailsCache.characterNumber = amiiboInfo.amiiboCharacterNumber;
    amiiboHashDetailsCache.variation = amiiboInfo.amiiboVariation;
    amiiboHashDetailsCache.form = amiiboInfo.amiiboForm;
    amiiboHashDetailsCache.number = amiiboInfo.amiiboNumber;
    amiiboHashDetailsCache.set = amiiboInfo.amiiboSet;

    for (int i = 0; i < 4; i++) {
        amiiboHashDetailsCache.head[i] = amiiboInfo.amiiboHead[i];
        amiiboHashDetailsCache.tail[i] = amiiboInfo.amiiboTail[i];
    }

    md5Builder.begin();
    md5Builder.add(amiiboHashDetailsCacheBytes, sizeof(AmiiboHashDetails));
    md5Builder.calculate();
    return md5Builder.getChars(hash);
}

void readAmiiboFromResult(sqlite3_stmt* stmt, AmiiboRecord& amiibo) {
    amiibo.id = sqlite3_column_int(stmt, 0);
    strcpy(amiibo.name, (const char*)sqlite3_column_text(stmt, 1));
    strcpy(amiibo.amiiboName, (const char*)sqlite3_column_text(stmt, 2));
    strcpy(amiibo.ownerName, (const char*)sqlite3_column_text(stmt, 3));
    memcpy(&amiibo.variation, sqlite3_column_blob(stmt, 4), 1);
    memcpy(&amiibo.form, sqlite3_column_blob(stmt, 5), 1);
    amiibo.number = sqlite3_column_int(stmt, 6);
    memcpy(&amiibo.set, sqlite3_column_blob(stmt, 7), 1);
    memcpy(&amiibo.head, sqlite3_column_blob(stmt, 8), 4);
    memcpy(&amiibo.tail, sqlite3_column_blob(stmt, 9), 4);
    strcpy(amiibo.file, (const char*)sqlite3_column_text(stmt, 10));
    strcpy(amiibo.hash, (const char*)sqlite3_column_text(stmt, 11));
    amiibo.last_written = sqlite3_column_int(stmt, 12);
}

void readSaveFromResult(sqlite3_stmt* stmt, SaveRecord& save) {
    PRINTLN("Reading Save Result from query");
    save.id = sqlite3_column_int(stmt, 0);
    save.amiibo_id = sqlite3_column_int(stmt, 1);
    strcpy(save.hash, (const char*)sqlite3_column_text(stmt, 2));
    strcpy(save.name, (const char*)sqlite3_column_text(stmt, 3));
    strcpy(save.file, (const char*)sqlite3_column_text(stmt, 4));
    save.last_update = sqlite3_column_int(stmt, 5);
    save.is_custom = sqlite3_column_int(stmt, 6) == 1;
}

bool prepareStatement(const char* sql, sqlite3_stmt* stmt, const char* stmtTail) {
    rc = sqlite3_prepare_v2(dbh, sql, strlen(sql), &stmt, &stmtTail);
    if (rc != SQLITE_OK) {
        Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

    return true;
}

bool readAmiibosFromQuery(sqlite3_stmt* stmt, int limit, AmiiboRecord& amiibo, const std::function<void(AmiiboRecord& record)>& callback = nullptr) {
    countBuff = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && (limit == 0 || countBuff < limit)) {
        countBuff++;
        readAmiiboFromResult(stmt, amiibo);
        if (callback != nullptr) {
            callback(amiibo);
        }
    }

//    sqlite3_clear_bindings(res);
//    sqlite3_reset(res);
    sqlite3_finalize(res);

    return countBuff > 0;
}

bool readSavesFromQuery(sqlite3_stmt* stmt, int limit, SaveRecord& save, const std::function<void(SaveRecord& record)>& callback = nullptr) {
    PRINTLN("Reading save results from query");
    countBuff = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && (limit == 0 || countBuff < limit)) {
        countBuff++;
        readSaveFromResult(stmt, save);
        if (callback != nullptr) {
            callback(save);
        }
    }

//    sqlite3_clear_bindings(res);
    sqlite3_reset(res);
    sqlite3_finalize(res);

    return countBuff > 0;
}

int AmiiboDBAO::findAmiiboIdByHash(const char* hash) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_ID_BY_HASH, res, tail)) {
        return -1;
    }

    PRINTV("Searching for hash:", hash);

    sqlite3_bind_text(res, 1, hash, strlen(hash), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    char* sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    rc = 0;
    if (sqlite3_step(res) == SQLITE_ROW) {
        PRINTV("Column count:",sqlite3_column_count(res));
        rc = sqlite3_column_int(res, 0);
        PRINTV("Column 0 value:",rc);
    }

//    sqlite3_clear_bindings(res);
    sqlite3_reset(res);
    sqlite3_finalize(res);
    PRINTV("Got amiibo id:", rc);

    return rc;
}

bool AmiiboDBAO::findAmiiboById(const int id, AmiiboRecord& amiibo) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_ID_BY_HASH, res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, id);

#ifdef DEBUG_SQL
    char* sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readAmiibosFromQuery(res, 1, amiibo);
}

bool AmiiboDBAO::findSaveById(const int id, SaveRecord& save) {
    if (! prepareStatement(SQL_SELECT_SAVE_BY_ID, res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, id);

#ifdef DEBUG_SQL
    char* sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readSavesFromQuery(res, 1, save);
}

bool AmiiboDBAO::findAmiiboByHash(const char* hash, AmiiboRecord& amiibo) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_BY_HASH, res, tail)) {
        return false;
    }

    sqlite3_bind_text(res, 1, hash, sizeof(AmiiboRecord::hash), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    char* sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readAmiibosFromQuery(res, 1, amiibo);
}

bool AmiiboDBAO::findAmiiboByFileName(const char *filename, AmiiboRecord &amiibo) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_BY_FILE_NAME, res, tail)) {
        return false;
    }

    sqlite3_bind_text(res, 1, filename, strlen(filename), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    char* sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readAmiibosFromQuery(res, 1, amiibo);
}

bool AmiiboDBAO::findAmiibosByFileNameMatch(const char* filename, const std::function<void(AmiiboRecord& record)>& callback, int limit) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_BY_FILE_LIKE, res, tail)) {
        return false;
    }

    String partial("%");
    partial = partial + filename + "%";
    sqlite3_bind_text(res, 1, partial.c_str(), partial.length(), SQLITE_TRANSIENT);

    return readAmiibosFromQuery(res, limit, amiiboRecordBuff, callback);
}

bool AmiiboDBAO::findSavesByAmiiboHash(const char* hash, const std::function<void(SaveRecord& record)>& callback, int limit) {
    if (! prepareStatement(SQL_SELECT_SAVE_BY_AMIIBO_HASH, res, tail)) {
        return false;
    }

    sqlite3_bind_text(res, 1, hash, strlen(hash), SQLITE_TRANSIENT);

    return readSavesFromQuery(res, limit, saveRecordBuff, callback);
}

bool AmiiboDBAO::findSavesByAmiiboFileName(const char* filename, const std::function<void(SaveRecord& record)>& callback, int limit) {
    PRINTLN("Preparing statement");
    if (! prepareStatement(SQL_SELECT_SAVE_BY_AMIIBO_FILE, res, tail)) {
        return false;
    }

    PRINTLN("Binding parameter");
    sqlite3_bind_text(res, 1, filename, strlen(filename), SQLITE_TRANSIENT);

    PRINTLN("Initializing save record");
    return readSavesFromQuery(res, limit, saveRecordBuff, callback);
}

bool executeStatement(sqlite3_stmt* stmt) {
    if (sqlite3_step(res) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

//    sqlite3_clear_bindings(res);
    sqlite3_reset(res);
    sqlite3_finalize(res);

    return true;
}

bool AmiiboDBAO::updateAmiiboTimestamp(const int id, const int timestamp) {
    if (! prepareStatement(SQL_UPDATE_AMIIBO_TIMESTAMP_BY_ID, res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, timestamp);
    sqlite3_bind_int(res, 2, id);

    return executeStatement(res);
}

bool AmiiboDBAO::updateSaveTimestamp(const int id, const int timestamp) {
    AmiiboDBAO::findSaveById(id, saveRecordBuff);
    if (! prepareStatement(SQL_UPDATE_SAVE_TIMESTAMP_BY_ID, res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, timestamp);
    sqlite3_bind_int(res, 2, id);

    if (! executeStatement(res)) {
        return false;
    }

    return AmiiboDBAO::updateAmiiboTimestamp(saveRecordBuff.amiibo_id, timestamp);
}