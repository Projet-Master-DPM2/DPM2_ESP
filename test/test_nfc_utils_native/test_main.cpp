#include <unity.h>
#include "../../include/nfc_utils.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

// Tests de validation longueur UID
void test_uid_length_validation() {
    TEST_ASSERT_TRUE(Nfc_IsValidUidLength_C(4));   // MIFARE Classic/Ultralight
    TEST_ASSERT_TRUE(Nfc_IsValidUidLength_C(7));   // NTAG, double size
    TEST_ASSERT_TRUE(Nfc_IsValidUidLength_C(10));  // Triple size
    
    TEST_ASSERT_FALSE(Nfc_IsValidUidLength_C(0));
    TEST_ASSERT_FALSE(Nfc_IsValidUidLength_C(3));
    TEST_ASSERT_FALSE(Nfc_IsValidUidLength_C(5));
    TEST_ASSERT_FALSE(Nfc_IsValidUidLength_C(8));
    TEST_ASSERT_FALSE(Nfc_IsValidUidLength_C(15));
}

// Tests de détection de type de carte
void test_card_type_detection() {
    uint8_t uid4[] = {0x12, 0x34, 0x56, 0x78};
    uint8_t uid7[] = {0x04, 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB};
    
    TEST_ASSERT_EQUAL(NFC_CARD_MIFARE_ULTRALIGHT, Nfc_DetectCardType_C(uid7, 7, 0x00));
    TEST_ASSERT_EQUAL(NFC_CARD_MIFARE_CLASSIC_1K, Nfc_DetectCardType_C(uid4, 4, 0x08));
    TEST_ASSERT_EQUAL(NFC_CARD_MIFARE_CLASSIC_4K, Nfc_DetectCardType_C(uid4, 4, 0x18));
    TEST_ASSERT_EQUAL(NFC_CARD_NTAG213, Nfc_DetectCardType_C(uid7, 7, 0x44));
    TEST_ASSERT_EQUAL(NFC_CARD_UNKNOWN, Nfc_DetectCardType_C(uid4, 4, 0xFF));
    TEST_ASSERT_EQUAL(NFC_CARD_UNKNOWN, Nfc_DetectCardType_C(nullptr, 0, 0x08));
}

// Tests de validation de secteur
void test_sector_validation() {
    // Secteur valide avec données
    uint8_t validSector[] = {
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
    };
    TEST_ASSERT_TRUE(Nfc_IsDataSectorValid_C(validSector, sizeof(validSector)));
    
    // Secteur vide
    uint8_t emptySector[16] = {0};
    TEST_ASSERT_FALSE(Nfc_IsDataSectorValid_C(emptySector, sizeof(emptySector)));
    
    // Secteur uniforme (corrompu)
    uint8_t uniformSector[16];
    memset(uniformSector, 0xFF, sizeof(uniformSector));
    TEST_ASSERT_FALSE(Nfc_IsDataSectorValid_C(uniformSector, sizeof(uniformSector)));
    
    // Secteur trop petit
    uint8_t smallSector[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    TEST_ASSERT_FALSE(Nfc_IsDataSectorValid_C(smallSector, sizeof(smallSector)));
    
    // Paramètres invalides
    TEST_ASSERT_FALSE(Nfc_IsDataSectorValid_C(nullptr, 16));
    TEST_ASSERT_FALSE(Nfc_IsDataSectorValid_C(validSector, 0));
}

// Tests d'extraction de texte ASCII
void test_ascii_text_extraction() {
    char output[64];
    
    // Données avec texte ASCII valide
    uint8_t textData[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', 0x00};
    TEST_ASSERT_TRUE(Nfc_ExtractAsciiText_C(textData, sizeof(textData), output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("Hello World", output);
    
    // Données avec caractères de contrôle
    uint8_t mixedData[] = {'T', 'e', 's', 't', 0x09, 'D', 'a', 't', 'a', 0x0A, 0x00};
    TEST_ASSERT_TRUE(Nfc_ExtractAsciiText_C(mixedData, sizeof(mixedData), output, sizeof(output)));
    TEST_ASSERT_EQUAL_STRING("Test Data ", output);
    
    // Données avec trop de caractères non-ASCII
    uint8_t binaryData[] = {0x01, 0x02, 0x03, 'A', 'B', 0x04, 0x05, 0x06};
    TEST_ASSERT_FALSE(Nfc_ExtractAsciiText_C(binaryData, sizeof(binaryData), output, sizeof(output)));
    
    // Données trop courtes
    uint8_t shortData[] = {'A', 'B'};
    TEST_ASSERT_FALSE(Nfc_ExtractAsciiText_C(shortData, sizeof(shortData), output, sizeof(output)));
    
    // Paramètres invalides
    TEST_ASSERT_FALSE(Nfc_ExtractAsciiText_C(nullptr, 10, output, sizeof(output)));
    TEST_ASSERT_FALSE(Nfc_ExtractAsciiText_C(textData, sizeof(textData), nullptr, sizeof(output)));
    TEST_ASSERT_FALSE(Nfc_ExtractAsciiText_C(textData, 0, output, sizeof(output)));
    TEST_ASSERT_FALSE(Nfc_ExtractAsciiText_C(textData, sizeof(textData), output, 1));
}

// Tests de validation header NDEF
void test_ndef_header_validation() {
    // Header NDEF valide: MB=1, ME=1, SR=1, TNF=1 (Well Known)
    uint8_t validHeader1[] = {0xD1}; // 11010001
    TEST_ASSERT_TRUE(Nfc_IsValidNdefHeader_C(validHeader1));
    
    // Header NDEF valide: MB=1, ME=0, SR=1, TNF=2 (Media type)
    uint8_t validHeader2[] = {0x92}; // 10010010
    TEST_ASSERT_TRUE(Nfc_IsValidNdefHeader_C(validHeader2));
    
    // Header invalide: TNF > 6
    uint8_t invalidTnf[] = {0x87}; // TNF = 7
    TEST_ASSERT_FALSE(Nfc_IsValidNdefHeader_C(invalidTnf));
    
    // Header invalide: CF=1 avec MB=1
    uint8_t invalidCfMb[] = {0xA1}; // CF=1, MB=1
    TEST_ASSERT_FALSE(Nfc_IsValidNdefHeader_C(invalidCfMb));
    
    // Header invalide: CF=1 avec ME=1
    uint8_t invalidCfMe[] = {0x61}; // CF=1, ME=1
    TEST_ASSERT_FALSE(Nfc_IsValidNdefHeader_C(invalidCfMe));
    
    // Paramètre invalide
    TEST_ASSERT_FALSE(Nfc_IsValidNdefHeader_C(nullptr));
}

int main() {
    UNITY_BEGIN();
    
    // Tests longueur UID
    RUN_TEST(test_uid_length_validation);
    
    // Tests détection type carte
    RUN_TEST(test_card_type_detection);
    
    // Tests validation secteur
    RUN_TEST(test_sector_validation);
    
    // Tests extraction ASCII
    RUN_TEST(test_ascii_text_extraction);
    
    // Tests header NDEF
    RUN_TEST(test_ndef_header_validation);
    
    return UNITY_END();
}
