//
// Created by hotdropper on 7/18/20.
//

#ifndef WIFIIBO_CLION_ARDUINODEBUG_H
#define WIFIIBO_CLION_ARDUINODEBUG_H

#include <WString.h>

#ifndef PRINT_DEBUG
#define PRINT_DEBUG 0
#endif

#if PRINT_DEBUG && !defined(AMIIBUDDY_RELEASE)
#define PRINT_PREAMBLE() { Serial.print(__FILE__); Serial.print(F(":")); Serial.print(__LINE__); Serial.print(F(" ")); Serial.print(__FUNCTION__); Serial.print(F("(): ")); }
#define PRINT(...)   { Serial.print(__VA_ARGS__); }
#define PRINTLN(...)   { PRINT_PREAMBLE(); Serial.println(__VA_ARGS__); }
#define PRINTS(s)   { Serial.print(F(s)); }
#define PRINTLNS(s)   { PRINT_PREAMBLE(); Serial.println(F(s)); }
#define PRINTV(s,v)  { PRINT_PREAMBLE(); Serial.print(F(s " ")); Serial.println(v); }
#define PRINTX(s,x) { PRINT_PREAMBLE(); Serial.print(F(s " ")); Serial.println(x, HEX); }
#define PRINTHEX(d,l) { PRINT_PREAMBLE(); Serial.println(); printHexDump(d, l); }
#define PRINTHEXV(s,d,l) { PRINT_PREAMBLE(); Serial.println(); Serial.print(F(s " ")); printHexDump(d, l); }
#else
#define PRINT(...)
#define PRINTLN(...)
#define PRINTS(s)
#define PRINTLNS(s)
#define PRINTV(s,v)
#define PRINTX(s,x)
#define PRINTHEX(d,l)
#define PRINTHEXV(s,d,l)
#endif

void printHexDump(const uint8_t *data, int dataLength, int bytesPerLine = 16);

#endif //WIFIIBO_CLION_ARDUINODEBUG_H
