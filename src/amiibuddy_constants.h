//
// Created by Jacob Mather on 7/29/20.
//

#ifndef AMIIBUDDY_AMIIBUDDY_CONSTANTS_H
#define AMIIBUDDY_AMIIBUDDY_CONSTANTS_H

#include <amiitool.h>


const prog_char PROGMEM FIELD_NAME[] = "name";
const prog_char PROGMEM FIELD_OWNER_NAME[] = "owner_name";
const prog_char PROGMEM FIELD_CHARACTER_NUMBER[] = "character_number";
const prog_char PROGMEM FIELD_VARIATION[] = "variation";
const prog_char PROGMEM FIELD_FORM[] = "form";
const prog_char PROGMEM FIELD_NUMBER[] = "number";
const prog_char PROGMEM FIELD_SET[] = "set";
const prog_char PROGMEM FIELD_HEAD[] = "head";
const prog_char PROGMEM FIELD_TAIL[] = "tail";
const prog_char PROGMEM FIELD_HASH[] = "hash";
const prog_char PROGMEM FIELD_SAVE_HASH[] = "save_hash";
const prog_char PROGMEM FIELD_FILE_PATH[] = "file_path";
const prog_int16_t PROGMEM FIELD_NAME_SIZE = sizeof(amiiboInfoStruct::amiiboName);
const prog_int16_t PROGMEM FIELD_OWNER_NAME_SIZE = sizeof(amiiboInfoStruct::amiiboOwnerMiiName);
const prog_int16_t PROGMEM FIELD_CHARACTER_NUMBER_SIZE = sizeof(char[5]);
const prog_int16_t PROGMEM FIELD_VARIATION_SIZE = sizeof(char[3]);
const prog_int16_t PROGMEM FIELD_FORM_SIZE = sizeof(char[3]);
const prog_int16_t PROGMEM FIELD_NUMBER_SIZE = sizeof(char[5]);
const prog_int16_t PROGMEM FIELD_SET_SIZE = sizeof(char[3]);
const prog_int16_t PROGMEM FIELD_HEAD_SIZE = sizeof(amiiboInfoStruct::amiiboHeadChar);
const prog_int16_t PROGMEM FIELD_TAIL_SIZE = sizeof(amiiboInfoStruct::amiiboTailChar);
const prog_int16_t PROGMEM FIELD_HASH_SIZE = 33;
const prog_int16_t PROGMEM FIELD_FILE_PATH_SIZE = 200;

const prog_char PROGMEM ERROR_LOAD_FILE[] = "Could not load file: ";
const prog_char PROGMEM ERROR_WRITE_FILE[] = "Failed to write to file: ";
const prog_char PROGMEM ERROR_INVALID_AMIIBO[] = "Encountered invalid amiibo file: ";

const prog_char PROGMEM MSG_WAITING_FOR_DB_RELOAD[] = "Waiting for the db to reload...";

const prog_char PROGMEM TEXT_WAITING[] = "Waiting";
const prog_char PROGMEM TEXT_WARNING[] = "Warning";
const prog_char PROGMEM TEXT_ERROR[] = "Error";
const prog_char PROGMEM TEXT_DISMISS[] = "Dismiss";
const prog_char PROGMEM TEXT_OK[] = "Ok";
const prog_char PROGMEM TEXT_SUCCESS[] = "Success";
const prog_char PROGMEM TEXT_COMPLETE[] = "Complete";
const prog_char PROGMEM TEXT_FULL_AMIIBO_SET[] = "Full Amiibo Set";
const prog_char PROGMEM TEXT_TEST_AMIIBO_SET[] = "Test Amiibo Set";

const prog_char PROGMEM TMPL_SWITCHED_AMIIBO_SET[] = "We have switched the amiibo data set to the ";
const prog_char PROGMEM TMPL_HEX_4BYTES[] = "%04hX";
const prog_char PROGMEM TMPL_HEX_2BYTES[] = "%02hhX";
const prog_char PROGMEM TMPL_DOUBLE_STR[] = "%s%s";
const prog_char PROGMEM TMPL_QUAD_STR[] = "%s%s%s%s";

typedef uint8_t tag_data_t[540];
typedef uint8_t tag_key_data[160];
typedef char md5_hash_t[40];

#endif //AMIIBUDDY_AMIIBUDDY_CONSTANTS_H
