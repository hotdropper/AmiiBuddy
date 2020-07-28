#ifndef __DEBUG_H__
#define __DEBUG_H__

//#define PN532_DEBUG

#include "Arduino.h"

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define PN532_SERIAL SerialUSB
#else
    #define PN532_SERIAL Serial
#endif

#ifdef PN532_DEBUG
#define DMSG(args...)       PN532_SERIAL.print(args)
#define DMSG_STR(str)       PN532_SERIAL.println(str)
#define DMSG_HEX(num)       PN532_SERIAL.print(' '); PN532_SERIAL.print((num>>4)&0x0F, HEX); PN532_SERIAL.print(num&0x0F, HEX)
#define DMSG_INT(num)       PN532_SERIAL.print(' '); PN532_SERIAL.print(num)
#else
#define DMSG(args...)
#define DMSG_STR(str)
#define DMSG_HEX(num)
#define DMSG_INT(num)
#endif

#endif
