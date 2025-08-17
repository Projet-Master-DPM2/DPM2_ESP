#pragma once

#include <stddef.h>
#include <stdint.h>

// API C (compatible tests natifs)
bool Nfc_ParseTlvAndExtractNdefText_C(const uint8_t* mem, size_t totalLen, char* outText, size_t outSize);
bool Nfc_UidToHex_C(const uint8_t* uidBytes, size_t uidLen, char* outHex, size_t outSize);

#ifdef ARDUINO
#include <Arduino.h>
// API Arduino pratique
bool Nfc_ParseTlvAndExtractNdefText(const uint8_t* mem, size_t totalLen, String& outText);
String Nfc_UidToHex(const uint8_t* uidBytes, size_t uidLen);
#endif


