#ifndef AMIITOOL_H
#define AMIITOOL_H
//#define USE_SDFAT

#include <string.h>
#include <Arduino.h>
#include <amiibo.h>

#define AMIIBO_KEY_FILE_SIZE 160
#define NTAG215_SIZE 540
#define NTAG215_PAGESIZE 4
#define AMIIBO_NAME_LEN 20 // Actually 10 UTF-16 chars
#define AMIIBO_PAGES 135
#define AMIIBO_DYNAMIC_LOCK_PAGE 130
#define AMIIBO_STATIC_LOCK_PAGE 2

// These offsets are in the decrypted data
#define AMIIBO_NAME_OFFSET 				0x003C		// Offset in decrypted data
#define AMIIBO_DEC_CHARDATA_OFFSET 		0x01DC		// Offset in decrypted data
#define AMIIBO_ENC_CHARDATA_OFFSET		0x0054		// Offset in encrypted data
#define AMIIBO_DEC_OWNERMII_NAME_OFFSET	0x0066
#define AMIIBO_DEC_DATA_OFFSET			0x002C
#define AMIIBO_DEC_FLAGS_BYTE_OFFSET	(AMIIBO_DEC_DATA_OFFSET + 0x0000)
#define AMIIBO_CHARNUM_MASK			0xFF
#define AMIIBO_HEAD_LEN				4
#define AMIIBO_TAIL_LEN				4

typedef struct
{
	char amiiboName[AMIIBO_NAME_LEN*6+1];
	char amiiboOwnerMiiName[AMIIBO_NAME_LEN*6+1];
	unsigned short amiiboCharacterNumber;
	byte amiiboVariation;
	byte amiiboForm;
	unsigned short amiiboNumber;
	byte amiiboSet;
	uint8_t amiiboHead[AMIIBO_HEAD_LEN];
	uint8_t amiiboTail[AMIIBO_TAIL_LEN];
	char amiiboHeadChar[AMIIBO_HEAD_LEN*2+1];
	char amiiboTailChar[AMIIBO_TAIL_LEN*2+1];

} amiiboInfoStruct;

const uint8_t DynamicLockBytes[] = {0x01, 0x00, 0x0F, 0xBD};
const uint8_t StaticLockBytes[] =  {0x00, 0x00, 0x0F, 0xE0};

class amiitool
{
	public:
		nfc3d_amiibo_keys amiiboKeys;

		uint8_t original[NTAG215_SIZE];
		uint8_t modified[NFC3D_AMIIBO_SIZE];
		
		amiiboInfoStruct amiiboInfo;

        amiitool();
        bool setAmiiboKeys(nfc3d_amiibo_keys loadedKeys);
		bool isKeyLoaded();
		bool loadFileFromData(const uint8_t * filedata, int size, bool lenient);
		void unloadFile();
		int encryptLoadedFile(uint8_t * uid);
		int decryptLoadedFile(bool lenient);
		bool isEncrypted(const uint8_t * filedata);
		bool decryptedFileValid(uint8_t * filedata);
		void readIDFields(uint8_t *data, unsigned short offset, amiiboInfoStruct *info);
		void generateBlankAmiibo(uint8_t * amiiboID);
		void generateRandomUID(uint8_t * uid, uint8_t * sizeLen);
		bool verifyPage(int pagenum, uint8_t * pagebytes);
		
		static void printData(uint8_t *data, int len, int valsPerCol, bool headers, bool formatForC);
		static void readUTF16BEStr(uint8_t *startByte, int stringLen, char *outStr, bool byteSwap);
		
	private:
		bool keyloaded;
		bool fileloaded;
		char *keyfilename;

		void readDecryptedFields();
};

extern amiitool atool;

#endif