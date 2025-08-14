#include "../../include/nfc_utils.h"
#include <string.h>

bool Nfc_IsValidUidLength_C(size_t length) {
    // Longueurs UID valides selon ISO14443
    return (length == 4 || length == 7 || length == 10);
}

NfcCardType Nfc_DetectCardType_C(const uint8_t* uid, size_t uidLen, uint8_t sak) {
    if (!uid || uidLen == 0) return NFC_CARD_UNKNOWN;
    
    // Détection basée sur SAK (Select Acknowledge) et longueur UID
    switch (sak) {
        case 0x00:
            if (uidLen == 7) return NFC_CARD_MIFARE_ULTRALIGHT;
            break;
        case 0x08:
            return NFC_CARD_MIFARE_CLASSIC_1K;
        case 0x18:
            return NFC_CARD_MIFARE_CLASSIC_4K;
        case 0x44:
            // NTAG - différencier par UID
            if (uidLen == 7) {
                return NFC_CARD_NTAG213; // Simplification
            }
            break;
    }
    
    return NFC_CARD_UNKNOWN;
}

bool Nfc_IsDataSectorValid_C(const uint8_t* sector, size_t sectorSize) {
    if (!sector || sectorSize == 0) return false;
    
    // Vérifications basiques d'intégrité
    if (sectorSize < 16) return false; // Secteur MIFARE minimum
    
    // Vérifier que le secteur n'est pas entièrement vide ou corrompu
    bool hasData = false;
    bool allSame = true;
    uint8_t firstByte = sector[0];
    
    for (size_t i = 0; i < sectorSize; i++) {
        if (sector[i] != 0x00) hasData = true;
        if (sector[i] != firstByte) allSame = false;
    }
    
    // Secteur valide s'il a des données et n'est pas uniforme
    return hasData && !allSame;
}

bool Nfc_ExtractAsciiText_C(const uint8_t* data, size_t dataLen, char* output, size_t outputSize) {
    if (!data || !output || dataLen == 0 || outputSize < 2) return false;
    
    size_t outputIndex = 0;
    size_t validChars = 0;
    
    for (size_t i = 0; i < dataLen && outputIndex < (outputSize - 1); i++) {
        uint8_t byte = data[i];
        
        if (byte == 0x00) break; // Fin de chaîne
        
        if (byte >= 0x20 && byte <= 0x7E) {
            // Caractère ASCII imprimable
            output[outputIndex++] = (char)byte;
            validChars++;
        } else if (byte == 0x09 || byte == 0x0A || byte == 0x0D) {
            // Caractères de contrôle acceptables (TAB, LF, CR)
            output[outputIndex++] = ' '; // Remplacer par espace
            validChars++;
        }
    }
    
    output[outputIndex] = '\0';
    
    // Considérer valide si au moins 3 caractères et 70% de caractères valides
    return (validChars >= 3) && (validChars * 100 / outputIndex >= 70);
}

bool Nfc_IsValidNdefHeader_C(const uint8_t* header) {
    if (!header) return false;
    
    // Vérification simplifiée d'un header NDEF
    // Format: [Flags][Type Length][Payload Length][ID Length][Type][ID][Payload]
    
    uint8_t flags = header[0];
    
    // Vérifier les flags NDEF de base
    bool mb = (flags & 0x80) != 0; // Message Begin
    bool me = (flags & 0x40) != 0; // Message End
    bool cf = (flags & 0x20) != 0; // Chunk Flag
    // bool sr = (flags & 0x10) != 0; // Short Record (unused)
    // bool il = (flags & 0x08) != 0; // ID Length present (unused)
    uint8_t tnf = flags & 0x07;   // Type Name Format
    
    // TNF doit être valide (0-6)
    if (tnf > 6) return false;
    
    // CF (Chunk Flag) ne doit pas être utilisé avec MB ou ME
    if (cf && (mb || me)) return false;
    
    return true;
}
