//
// Created by Jacob Mather on 7/31/20.
//

#define PRINT_DEBUG 1
#include <ArduinoDebug.h>
#include "AmiiboDBAO.h"
#include <sqlite3.h>
#include <MD5Builder.h>
#include <FS.h>

#define DEBUG_SQL

#define KEY_FILE "/sd/keys/retail.bin"

#ifdef UNIT_TEST
#define SQLITE_DB_PATH "/sd/test_data/amiibos.sqlite"
#define FS_DB_PATH "/test_data/amiibos.sqlite"
#else
#define SQLITE_DB_PATH "/sd/db/amiibos.sqlite"
#define FS_DB_PATH "/db/amiibos.sqlite"
#endif

#define AMIIBO_FIELDS "amiibos.id, amiibos.name, amiibos.amiibo_name, amiibos.owner_name, amiibos.variation, amiibos.form, amiibos.number, amiibos.\"set\", amiibos.head, amiibos.tail, amiibos.file, amiibos.hash, amiibos.last_updated, amiibos.directory_id"
#define SAVE_FIELDS "saves.id, saves.amiibo_id, saves.hash, saves.name, saves.file, saves.last_update, saves.is_custom"
#define DIRECTORY_FIELDS "directories.id, directories.parent_id, directories.name, directories.path"

extern FS SD;
extern const char amiibos_sql_start[] asm("_binary_db_amiibos_sql_start");
extern const char saves_sql_start[] asm("_binary_db_saves_sql_start");
extern const char directories_sql_start[] asm("_binary_db_directories_sql_start");
const prog_char PROGMEM SQL_INSERT_AMIIBO[] = "INSERT INTO amiibos (name, amiibo_name, owner_name, variation, form, number, \"set\", head, tail, file, hash, last_updated, directory_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
const prog_char PROGMEM SQL_INSERT_SAVE[] = "INSERT INTO saves (amiibo_id, hash, name, file, last_update, is_custom) VALUES (?, ?, ?, ?, ?, ?)";
const prog_char PROGMEM SQL_INSERT_DIRECTORY[] = "INSERT INTO directories (parent_id, name, path) VALUES (?, ?, ?)";
const prog_char PROGMEM SQL_SELECT_DIRECTORY_BY_PATH[] = "SELECT " DIRECTORY_FIELDS " FROM directories WHERE path = ? LIMIT 1";
const prog_char PROGMEM SQL_SELECT_DIRECTORIES_BY_PARENT_ID[] = "SELECT " DIRECTORY_FIELDS " FROM directories WHERE parent_id = ?";
const prog_char PROGMEM SQL_SELECT_AMIIBO_ID_BY_HASH[] = "SELECT id FROM amiibos WHERE hash = ?";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_NAME_LIKE[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE name LIKE ? ORDER BY last_updated DESC LIMIT ?";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_ID[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE id = ? LIMIT 1";
const prog_char PROGMEM SQL_SELECT_SAVE_BY_ID[] = "SELECT " SAVE_FIELDS " FROM saves WHERE id = ?";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_HASH[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE hash = ? LIMIT 1";
const prog_char PROGMEM SQL_SELECT_AMIIBO_BY_FILE_NAME[] = "SELECT " AMIIBO_FIELDS " FROM amiibos WHERE file = ? LIMIT 1";
const prog_char PROGMEM SQL_SELECT_SAVE_BY_AMIIBO_HASH[] = "SELECT " SAVE_FIELDS " FROM saves INNER JOIN amiibos a on a.id = saves.amiibo_id WHERE a.hash = ? ORDER BY saves.last_update DESC";
const prog_char PROGMEM SQL_SELECT_SAVE_BY_AMIIBO_FILE[] = "SELECT " SAVE_FIELDS " FROM saves INNER JOIN amiibos a on a.id = saves.amiibo_id WHERE a.file = ? ORDER BY saves.last_update DESC";
const prog_char PROGMEM SQL_UPDATE_SAVE_TIMESTAMP_BY_ID[] = "UPDATE saves SET last_update = ? WHERE id = ?";
const prog_char PROGMEM SQL_UPDATE_AMIIBO_TIMESTAMP_BY_ID[] = "UPDATE amiibos SET last_updated = ? WHERE id = ?";
const prog_char PROGMEM SQL_SELECT_NEXT_AMIIBO_UPDATE_INCREMENT[] = "SELECT MAX(last_updated) + 1 FROM amiibos";
const prog_char PROGMEM SQL_SELECT_NEXT_SAVE_UPDATE_INCREMENT[] = "SELECT MAX(last_update) + 1 FROM saves";

sqlite3 *dbh;
AmiiboHashDetails amiiboHashDetailsCache;
auto * amiiboHashDetailsCacheBytes = reinterpret_cast<uint8_t*>(&amiiboHashDetailsCache);
MD5Builder md5Builder = MD5Builder();
sqlite3_stmt *res;
sqlite3_stmt *insertAmiiboStatement = nullptr;
sqlite3_stmt *insertSaveStatement = nullptr;
sqlite3_stmt *insertDirectoryStatement = nullptr;
sqlite3_stmt *selectDirectoryByPathStmt = nullptr;
sqlite3_stmt *findDirectoryByParentIdStmt = nullptr;
const char *tail;
int rc = 0;
char adbaoHashCache[33];
const char* data = "Callback function called";
char *zErrMsg = 0;
char* sqlBuff;
SaveRecord saveRecordBuff = SaveRecord();
AmiiboRecord amiiboRecordBuff = AmiiboRecord();
DirectoryRecord directoryRecordBuff = DirectoryRecord();
int i;
int countBuff;
unsigned long start;
int updateCount = 0;

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

bool AmiiboDBAO::initialize() {
    sqlite3_initialize();

    if (openDb(SQLITE_DB_PATH, &dbh)) {
        return false;
    }

    return true;
}

void maybeFlush(bool forceFlush = false) {
    return;
    updateCount++;

    PRINTLN("Attempting to free up some memory...");
    if (sqlite3_db_release_memory(dbh) != SQLITE_OK) {
        PRINTLN("Freeing memory failed?!?!");
    }

    if (updateCount > 250 || forceFlush) {
        PRINTLN("Flushing DB to disk...");
        if (sqlite3_db_cacheflush(dbh) != SQLITE_OK) {
            PRINTLN("Flush failed?!?!");
        }
        updateCount = 0;
    }
}

bool AmiiboDBAO::end() {
    maybeFlush(true);
    sqlite3_close(dbh);
    return true;
}

bool AmiiboDBAO::insertAmiibo(AmiiboRecord& amiibo) {
    if (insertAmiiboStatement == nullptr) {
        rc = sqlite3_prepare_v2(dbh, SQL_INSERT_AMIIBO, strlen(SQL_INSERT_AMIIBO), &insertAmiiboStatement, &tail);
        if (rc != SQLITE_OK) {
            Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
            return false;
        }
    }

    sqlite3_bind_text(insertAmiiboStatement, 1, amiibo.name, strlen(amiibo.name), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertAmiiboStatement, 2, amiibo.amiiboName, strlen(amiibo.ownerName), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertAmiiboStatement, 3, amiibo.ownerName, strlen(amiibo.ownerName), SQLITE_TRANSIENT);
    sqlite3_bind_blob(insertAmiiboStatement, 4, &amiibo.variation, 1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(insertAmiiboStatement, 5, &amiibo.form, 1, SQLITE_TRANSIENT);
    sqlite3_bind_int(insertAmiiboStatement, 6, amiibo.number);
    sqlite3_bind_blob(insertAmiiboStatement, 7, &amiibo.set, 1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(insertAmiiboStatement, 8, &amiibo.head, 4, SQLITE_TRANSIENT);
    sqlite3_bind_blob(insertAmiiboStatement, 9, &amiibo.tail, 4, SQLITE_TRANSIENT);
    sqlite3_bind_text(insertAmiiboStatement, 10, amiibo.file, strlen(amiibo.file), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertAmiiboStatement, 11, amiibo.hash, 32, SQLITE_TRANSIENT);
    sqlite3_bind_int(insertAmiiboStatement, 12, amiibo.last_updated);
    sqlite3_bind_int(insertAmiiboStatement, 13, amiibo.directory_id);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(insertAmiiboStatement);
    PRINTV("INSERT:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    PRINTV("Attempting to insert amiibo hash:", amiibo.hash);
    if (sqlite3_step(insertAmiiboStatement) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        sqlite3_clear_bindings(insertAmiiboStatement);
        sqlite3_reset(insertAmiiboStatement);

        return false;
    }

//    PRINTV("Inserting hash:", amiibo.hash);

    sqlite3_clear_bindings(insertAmiiboStatement);
    sqlite3_reset(insertAmiiboStatement);
    amiibo.id = sqlite3_last_insert_rowid(dbh);
//    sqlite3_finalize(insertAmiiboStatement);
    PRINTV("Inserted amiibo id:", amiibo.id);

    maybeFlush();
    return true;
}

bool AmiiboDBAO::insertSave(SaveRecord& save) {
    if (insertSaveStatement == nullptr) {
        rc = sqlite3_prepare_v2(dbh, SQL_INSERT_SAVE, strlen(SQL_INSERT_SAVE), &insertSaveStatement, &tail);
        if (rc != SQLITE_OK) {
            Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
            return false;
        }
    }

    sqlite3_bind_int(insertSaveStatement, 1, save.amiibo_id);
    sqlite3_bind_text(insertSaveStatement, 2, save.hash, 32, SQLITE_TRANSIENT);
    sqlite3_bind_text(insertSaveStatement, 3, save.name, strlen(save.name), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertSaveStatement, 4, save.file, strlen(save.file), SQLITE_TRANSIENT);
    sqlite3_bind_int(insertSaveStatement, 5, save.last_update);
    sqlite3_bind_int(insertSaveStatement, 6, save.is_custom ? 1 : 0);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(insertSaveStatement);
    PRINTV("INSERT:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    PRINTV("Attempting to insert save hash:", save.hash);
    if (sqlite3_step(insertSaveStatement) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        sqlite3_clear_bindings(insertSaveStatement);
        sqlite3_reset(insertSaveStatement);

        return false;
    }
    sqlite3_clear_bindings(insertSaveStatement);
    sqlite3_reset(insertSaveStatement);
    save.id = sqlite3_last_insert_rowid(dbh);
//    sqlite3_finalize(res);
    PRINTV("Inserted save id:", save.id);

    maybeFlush();
    return true;
}

bool AmiiboDBAO::insertDirectory(DirectoryRecord &directory) {
    if (insertDirectoryStatement == nullptr) {
        rc = sqlite3_prepare_v2(dbh, SQL_INSERT_DIRECTORY, strlen(SQL_INSERT_DIRECTORY), &insertDirectoryStatement, &tail);
        if (rc != SQLITE_OK) {
            Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
            return false;
        }
    }

    sqlite3_bind_int(insertDirectoryStatement, 1, directory.parent_id);
    sqlite3_bind_text(insertDirectoryStatement, 2, directory.name, strlen(directory.name), SQLITE_TRANSIENT);
    sqlite3_bind_text(insertDirectoryStatement, 3, directory.path, strlen(directory.path), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(insertDirectoryStatement);
    PRINTV("INSERT:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    PRINTV("Attempting to insert directory:", directory.path);
    if (sqlite3_step(insertDirectoryStatement) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        sqlite3_clear_bindings(insertDirectoryStatement);
        sqlite3_reset(insertDirectoryStatement);

        return false;
    }
    sqlite3_clear_bindings(insertDirectoryStatement);
    sqlite3_reset(insertDirectoryStatement);
    directory.id = sqlite3_last_insert_rowid(dbh);
//    sqlite3_finalize(res);
    PRINTV("Inserted directory id:", directory.id);

    maybeFlush();
    return true;
}

bool AmiiboDBAO::updateSave(SaveRecord& save) {
    rc = sqlite3_prepare_v2(dbh, SQL_INSERT_SAVE, strlen(SQL_INSERT_SAVE), &res, &tail);
    if (rc != SQLITE_OK) {
        Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

    sqlite3_bind_text(res, 1, save.hash, 32, SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 2, save.name, strlen(save.file), SQLITE_TRANSIENT);
    sqlite3_bind_text(res, 3, save.file, strlen(save.file), SQLITE_TRANSIENT);
    sqlite3_bind_int(res, 4, save.last_update);
    sqlite3_bind_int(res, 5, save.id);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("UPDATE:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    if (sqlite3_step(res) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        return false;
    }
    sqlite3_clear_bindings(res);
    sqlite3_reset(res);
    sqlite3_finalize(res);

    maybeFlush();
    return true;
}

bool AmiiboDBAO::truncate() {
    sqlite3_close(dbh);
    SD.remove(FS_DB_PATH);
    openDb(SQLITE_DB_PATH, &dbh);

    rc = db_exec(dbh, directories_sql_start);
    if (rc != SQLITE_OK) {
        return false;
    }

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

void AmiiboDBAO::calculateSaveHash(const amiiboInfoStruct& amiiboInfo, char* checksum) {
    md5Builder.begin();
    // ami nb counter
    md5Builder.add(&atool.modified[0x29], 2);
    // flags
    md5Builder.add(&atool.modified[0x2C], 1);
    // program id
    md5Builder.add(&atool.modified[0xAC], 8);
    // cfg nb counter
    md5Builder.add(&atool.modified[0xB4], 2);
    // app id
    md5Builder.add(&atool.modified[0xB6], 4);
    // data
    md5Builder.add(&atool.modified[0xDC], 216);
    md5Builder.calculate();
    md5Builder.getChars(checksum);
}

void AmiiboDBAO::calculateHashes(uint8_t* tagData, HashInfo& hashes) {
    atool.loadKey(KEY_FILE);
    atool.loadFileFromData(tagData, NTAG215_SIZE, false);
    calculateHashes(atool.amiiboInfo, hashes);
}

void AmiiboDBAO::calculateHashes(const amiiboInfoStruct& amiiboInfo, HashInfo& hashes) {
    calculateSaveHash(amiiboInfo, hashes.saveHash);
    calculateAmiiboHash(amiiboInfo, hashes.amiiboHash);
}

void AmiiboDBAO::calculateAmiiboHash(const amiiboInfoStruct& amiiboInfo, char* hash) {
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
    amiibo.last_updated = sqlite3_column_int(stmt, 12);
    amiibo.directory_id = sqlite3_column_int(stmt, 13);
}

void readSaveFromResult(sqlite3_stmt* stmt, SaveRecord& save) {
//    PRINTLN("Reading Save Result from query");
    save.id = sqlite3_column_int(stmt, 0);
    save.amiibo_id = sqlite3_column_int(stmt, 1);
    strcpy(save.hash, (const char*)sqlite3_column_text(stmt, 2));
    strcpy(save.name, (const char*)sqlite3_column_text(stmt, 3));
    strcpy(save.file, (const char*)sqlite3_column_text(stmt, 4));
    save.last_update = sqlite3_column_int(stmt, 5);
    save.is_custom = sqlite3_column_int(stmt, 6) == 1;
}

void readDirectoryFromResult(sqlite3_stmt* stmt, DirectoryRecord& directory) {
    directory.id = sqlite3_column_int(stmt, 0);
    directory.parent_id = sqlite3_column_int(stmt, 1);
    strcpy(directory.name, (const char*)sqlite3_column_text(stmt, 2));
    strcpy(directory.path, (const char*)sqlite3_column_text(stmt, 3));
}

bool prepareStatement(const char* sql, sqlite3_stmt** stmt, const char* stmtTail) {
    rc = sqlite3_prepare_v2(dbh, sql, strlen(sql), stmt, &stmtTail);
    if (rc != SQLITE_OK) {
        Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

    return true;
}

bool readAmiibosFromQuery(sqlite3_stmt* stmt, AmiiboRecord& amiibo, const std::function<void(AmiiboRecord& record)>& callback = nullptr) {
    countBuff = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        countBuff++;
        readAmiiboFromResult(stmt, amiibo);
        if (callback != nullptr) {
            callback(amiibo);
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return countBuff > 0;
}

bool readSavesFromQuery(sqlite3_stmt* stmt, int limit, SaveRecord& save, const std::function<void(SaveRecord& record)>& callback = nullptr) {
//    PRINTLN("Reading save results from query");
    countBuff = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && (limit == 0 || countBuff < limit)) {
        countBuff++;
        readSaveFromResult(stmt, save);
        if (callback != nullptr) {
            callback(save);
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return countBuff > 0;
}

bool readDirectoriesFromQuery(bool finalize, sqlite3_stmt* stmt, DirectoryRecord& directory, const std::function<void(DirectoryRecord& record)>& callback = nullptr) {
    countBuff = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        countBuff++;
        readDirectoryFromResult(stmt, directory);
        if (callback != nullptr) {
            callback(directory);
        }
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    if (finalize) {
        sqlite3_finalize(stmt);
    }

    return countBuff > 0;
}

bool AmiiboDBAO::getDirectoryByPath(const char* path, DirectoryRecord &directory) {
//    PRINTV("Query Template:", SQL_SELECT_DIRECTORY_BY_PATH);
    if (selectDirectoryByPathStmt == nullptr) {
        rc = sqlite3_prepare_v2(dbh, SQL_SELECT_DIRECTORY_BY_PATH, strlen(SQL_SELECT_DIRECTORY_BY_PATH), &selectDirectoryByPathStmt, &tail);
        if (rc != SQLITE_OK) {
            Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
            return false;
        }
    }

    directory.id = 0;
    directory.parent_id = 0;
    strcpy(directory.name, "");
    strcpy(directory.path, "");
    PRINTV("Path:", path);

    sqlite3_bind_text(selectDirectoryByPathStmt, 1, path, strlen(path), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(selectDirectoryByPathStmt);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readDirectoriesFromQuery(false, selectDirectoryByPathStmt, directory);
}

bool AmiiboDBAO::findDirectoriesByParentId(const int parentId, const std::function<void(DirectoryRecord &)> &callback) {
//    PRINTV("Query Template:", SQL_SELECT_DIRECTORY_BY_PATH);
    if (findDirectoryByParentIdStmt == nullptr) {
        rc = sqlite3_prepare_v2(dbh, SQL_SELECT_DIRECTORIES_BY_PARENT_ID, strlen(SQL_SELECT_DIRECTORIES_BY_PARENT_ID), &findDirectoryByParentIdStmt, &tail);
        if (rc != SQLITE_OK) {
            Serial.printf("ERROR preparing sql: %s\n", sqlite3_errmsg(dbh));
            return false;
        }
    }

    PRINTV("Parent Id:", parentId);

    sqlite3_bind_int(selectDirectoryByPathStmt, 1, parentId);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(selectDirectoryByPathStmt);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readDirectoriesFromQuery(false, selectDirectoryByPathStmt, directoryRecordBuff);
}

int AmiiboDBAO::getAmiiboIdByHash(const char* hash) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_ID_BY_HASH, &res, tail)) {
        return -1;
    }

    PRINTV("Searching for hash:", hash);

    sqlite3_bind_text(res, 1, hash, strlen(hash), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
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

bool AmiiboDBAO::getAmiiboById(int id, AmiiboRecord& amiibo) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_BY_ID, &res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, id);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readAmiibosFromQuery(res, amiibo);
}

bool AmiiboDBAO::getSaveById(int id, SaveRecord& save) {
    if (! prepareStatement(SQL_SELECT_SAVE_BY_ID, &res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, id);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readSavesFromQuery(res, 1, save);
}

bool AmiiboDBAO::getAmiiboByHash(const char* hash, AmiiboRecord& amiibo) {
    PRINTV("Query Template:", SQL_SELECT_AMIIBO_BY_HASH);
    if (! prepareStatement(SQL_SELECT_AMIIBO_BY_HASH, &res, tail)) {
        return false;
    }

    PRINTV("Hash:", hash);

    sqlite3_bind_text(res, 1, hash, strlen(hash), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readAmiibosFromQuery(res, amiibo);
}

bool AmiiboDBAO::getAmiiboByFileName(const char *filename, AmiiboRecord &amiibo) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_BY_FILE_NAME, &res, tail)) {
        return false;
    }

    sqlite3_bind_text(res, 1, filename, strlen(filename), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readAmiibosFromQuery(res, amiibo);
}

bool AmiiboDBAO::findAmiibosByNameMatch(const char* name, const std::function<void(AmiiboRecord& record)>& callback, int limit) {
    if (! prepareStatement(SQL_SELECT_AMIIBO_BY_NAME_LIKE, &res, tail)) {
        return false;
    }

    String partial("%");
    partial = partial + name + "%";
    sqlite3_bind_text(res, 1, partial.c_str(), partial.length(), SQLITE_TRANSIENT);
    sqlite3_bind_int(res, 2, limit);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    return readAmiibosFromQuery(res, amiiboRecordBuff, callback);
}

bool AmiiboDBAO::findSavesByAmiiboHash(const char* hash, const std::function<void(SaveRecord& record)>& callback, int limit) {
    if (! prepareStatement(SQL_SELECT_SAVE_BY_AMIIBO_HASH, &res, tail)) {
        return false;
    }

    sqlite3_bind_text(res, 1, hash, strlen(hash), SQLITE_TRANSIENT);

    return readSavesFromQuery(res, limit, saveRecordBuff, callback);
}

bool AmiiboDBAO::findSavesByAmiiboFileName(const char* filename, const std::function<void(SaveRecord& record)>& callback, int limit) {
    PRINTLN("Preparing statement");
    if (! prepareStatement(SQL_SELECT_SAVE_BY_AMIIBO_FILE, &res, tail)) {
        return false;
    }

    PRINTLN("Binding parameter");
    sqlite3_bind_text(res, 1, filename, strlen(filename), SQLITE_TRANSIENT);

#ifdef DEBUG_SQL
    sqlBuff = sqlite3_expanded_sql(res);
    PRINTV("Query:", sqlBuff);
    sqlite3_free(sqlBuff);
#endif

    PRINTLN("Initializing save record");
    return readSavesFromQuery(res, limit, saveRecordBuff, callback);
}

bool executeStatement(sqlite3_stmt* stmt) {
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbh));
        return false;
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    maybeFlush();
    return true;
}

int getNextSaveUpdateNumber() {
    if (! prepareStatement(SQL_SELECT_NEXT_SAVE_UPDATE_INCREMENT, &res, tail)) {
        return false;
    }

    int ret = 0;

    if (sqlite3_step(res) == SQLITE_ROW) {
        ret = sqlite3_column_int(res, 0);
    }

    sqlite3_reset(res);
    sqlite3_finalize(res);

    return ret;
}

int getNextAmiiboUpdateNumber() {
    if (! prepareStatement(SQL_SELECT_NEXT_AMIIBO_UPDATE_INCREMENT, &res, tail)) {
        return false;
    }

    int ret = 0;

    if (sqlite3_step(res) == SQLITE_ROW) {
        ret = sqlite3_column_int(res, 0);
    }

    sqlite3_reset(res);
    sqlite3_finalize(res);

    return ret;
}

bool AmiiboDBAO::updateAmiiboTimestamp(const int id, int timestamp) {
    if (timestamp == 0) {
        timestamp = getNextAmiiboUpdateNumber();
    }

    if (! prepareStatement(SQL_UPDATE_AMIIBO_TIMESTAMP_BY_ID, &res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, timestamp);
    sqlite3_bind_int(res, 2, id);

    return executeStatement(res);
}

bool AmiiboDBAO::updateSaveTimestamp(const int id, int timestamp) {
    if (timestamp == 0) {
        timestamp = getNextAmiiboUpdateNumber();
    }
    if (! prepareStatement(SQL_UPDATE_SAVE_TIMESTAMP_BY_ID, &res, tail)) {
        return false;
    }

    sqlite3_bind_int(res, 1, timestamp);
    sqlite3_bind_int(res, 2, id);

    if (! executeStatement(res)) {
        return false;
    }

    AmiiboDBAO::getSaveById(id, saveRecordBuff);
    return AmiiboDBAO::updateAmiiboTimestamp(saveRecordBuff.amiibo_id);
}