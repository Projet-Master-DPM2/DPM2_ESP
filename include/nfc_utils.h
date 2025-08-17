#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Types de cartes NFC
typedef enum {
    NFC_CARD_UNKNOWN = 0,
    NFC_CARD_MIFARE_ULTRALIGHT,
    NFC_CARD_MIFARE_CLASSIC_1K,
    NFC_CARD_MIFARE_CLASSIC_4K,
    NFC_CARD_NTAG213,
    NFC_CARD_NTAG215,
    NFC_CARD_NTAG216
} NfcCardType;

// Utilitaires de conversion et validation
bool Nfc_IsValidUidLength_C(size_t length);
NfcCardType Nfc_DetectCardType_C(const uint8_t* uid, size_t uidLen, uint8_t sak);
bool Nfc_IsDataSectorValid_C(const uint8_t* sector, size_t sectorSize);

// Extraction de données ASCII (fallback)
bool Nfc_ExtractAsciiText_C(const uint8_t* data, size_t dataLen, char* output, size_t outputSize);

// Validation de format NDEF simplifié
bool Nfc_IsValidNdefHeader_C(const uint8_t* header);

#ifdef __cplusplus
}
#endif
