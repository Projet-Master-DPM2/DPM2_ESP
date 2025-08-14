#include <unity.h>
#include "../../include/orchestrator_logic.h"

void setUp(void) {
    // Configuration avant chaque test
}

void tearDown(void) {
    // Nettoyage après chaque test
}

// Tests de traitement des événements
void test_state_paying_with_wifi() {
    OrchestratorResult result = Orchestrator_ProcessEvent_C(ORCH_EVT_STATE_PAYING, nullptr, true);
    TEST_ASSERT_EQUAL(ORCH_RESULT_NFC_TRIGGERED, result);
}

void test_state_paying_without_wifi() {
    OrchestratorResult result = Orchestrator_ProcessEvent_C(ORCH_EVT_STATE_PAYING, nullptr, false);
    TEST_ASSERT_EQUAL(ORCH_RESULT_ERROR_NO_WIFI, result);
}

void test_nfc_uid_valid() {
    OrchestratorResult result = Orchestrator_ProcessEvent_C(ORCH_EVT_NFC_UID_READ, "ABCD1234", true);
    TEST_ASSERT_EQUAL(ORCH_RESULT_PAYMENT_PROCESSED, result);
}

void test_nfc_uid_invalid() {
    OrchestratorResult result = Orchestrator_ProcessEvent_C(ORCH_EVT_NFC_UID_READ, "INVALID_UID!", true);
    TEST_ASSERT_EQUAL(ORCH_RESULT_IGNORED, result);
}

void test_nfc_data_valid() {
    OrchestratorResult result = Orchestrator_ProcessEvent_C(ORCH_EVT_NFC_DATA, "Hello World", true);
    TEST_ASSERT_EQUAL(ORCH_RESULT_PAYMENT_PROCESSED, result);
}

void test_nfc_data_invalid() {
    OrchestratorResult result = Orchestrator_ProcessEvent_C(ORCH_EVT_NFC_DATA, "AB", true);
    TEST_ASSERT_EQUAL(ORCH_RESULT_IGNORED, result);
}

void test_nfc_error_ignored() {
    OrchestratorResult result = Orchestrator_ProcessEvent_C(ORCH_EVT_NFC_ERROR, "TIMEOUT", true);
    TEST_ASSERT_EQUAL(ORCH_RESULT_IGNORED, result);
}

// Tests de validation UID
void test_uid_validation_valid_cases() {
    TEST_ASSERT_TRUE(Orchestrator_IsValidNfcUid_C("ABCD1234"));
    TEST_ASSERT_TRUE(Orchestrator_IsValidNfcUid_C("12345678"));
    TEST_ASSERT_TRUE(Orchestrator_IsValidNfcUid_C("aAbBcCdD"));
    TEST_ASSERT_TRUE(Orchestrator_IsValidNfcUid_C("0123456789ABCDEF"));
}

void test_uid_validation_invalid_cases() {
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcUid_C(nullptr));
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcUid_C(""));
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcUid_C("123"));      // Trop court
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcUid_C("123456789012345678901")); // Trop long
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcUid_C("ABCD123G"));  // Caractère invalide
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcUid_C("ABCD-123"));  // Caractère invalide
}

// Tests de validation données NFC
void test_nfc_data_validation_valid() {
    TEST_ASSERT_TRUE(Orchestrator_IsValidNfcData_C("Hello"));
    TEST_ASSERT_TRUE(Orchestrator_IsValidNfcData_C("User123"));
    TEST_ASSERT_TRUE(Orchestrator_IsValidNfcData_C("Product ABC"));
}

void test_nfc_data_validation_invalid() {
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcData_C(nullptr));
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcData_C(""));
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcData_C("AB"));       // Trop court
    TEST_ASSERT_FALSE(Orchestrator_IsValidNfcData_C("This is a very long string that exceeds the maximum allowed length for NFC data validation testing purposes and should fail")); // Trop long
}

int main() {
    UNITY_BEGIN();
    
    // Tests de traitement des événements
    RUN_TEST(test_state_paying_with_wifi);
    RUN_TEST(test_state_paying_without_wifi);
    RUN_TEST(test_nfc_uid_valid);
    RUN_TEST(test_nfc_uid_invalid);
    RUN_TEST(test_nfc_data_valid);
    RUN_TEST(test_nfc_data_invalid);
    RUN_TEST(test_nfc_error_ignored);
    
    // Tests de validation UID
    RUN_TEST(test_uid_validation_valid_cases);
    RUN_TEST(test_uid_validation_invalid_cases);
    
    // Tests de validation données NFC
    RUN_TEST(test_nfc_data_validation_valid);
    RUN_TEST(test_nfc_data_validation_invalid);
    
    return UNITY_END();
}
