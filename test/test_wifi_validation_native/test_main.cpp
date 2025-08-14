#include <unity.h>
#include "../../include/wifi_validation.h"

void setUp(void) {}
void tearDown(void) {}

// Tests de validation SSID
void test_ssid_valid_cases() {
    TEST_ASSERT_TRUE(Wifi_IsValidSsid_C("MyNetwork"));
    TEST_ASSERT_TRUE(Wifi_IsValidSsid_C("WiFi-2024"));
    TEST_ASSERT_TRUE(Wifi_IsValidSsid_C("Network_123"));
    TEST_ASSERT_TRUE(Wifi_IsValidSsid_C("A"));
    TEST_ASSERT_TRUE(Wifi_IsValidSsid_C("12345678901234567890123456789012")); // 32 chars
}

void test_ssid_invalid_cases() {
    TEST_ASSERT_FALSE(Wifi_IsValidSsid_C(nullptr));
    TEST_ASSERT_FALSE(Wifi_IsValidSsid_C(""));
    TEST_ASSERT_FALSE(Wifi_IsValidSsid_C("123456789012345678901234567890123")); // 33 chars
    TEST_ASSERT_FALSE(Wifi_IsValidSsid_C("Network\x01")); // Caractère de contrôle
    TEST_ASSERT_FALSE(Wifi_IsValidSsid_C("Network\x7F")); // DEL
}

// Tests de validation mot de passe
void test_password_valid_cases() {
    TEST_ASSERT_TRUE(Wifi_IsValidPassword_C(nullptr));        // Réseau ouvert
    TEST_ASSERT_TRUE(Wifi_IsValidPassword_C(""));             // Réseau ouvert
    TEST_ASSERT_TRUE(Wifi_IsValidPassword_C("password"));     // 8 chars min
    TEST_ASSERT_TRUE(Wifi_IsValidPassword_C("MySecurePass123"));
    TEST_ASSERT_TRUE(Wifi_IsValidPassword_C("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef")); // 64 chars hex
    TEST_ASSERT_TRUE(Wifi_IsValidPassword_C("ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890")); // 64 chars hex
}

void test_password_invalid_cases() {
    TEST_ASSERT_FALSE(Wifi_IsValidPassword_C("1234567"));     // Trop court
    TEST_ASSERT_FALSE(Wifi_IsValidPassword_C("This password is way too long and exceeds the maximum allowed length")); // Trop long
    TEST_ASSERT_FALSE(Wifi_IsValidPassword_C("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdeg")); // 64 chars non-hex
    TEST_ASSERT_FALSE(Wifi_IsValidPassword_C("password\x01")); // Caractère de contrôle
}

// Tests de validation canal
void test_channel_valid_cases() {
    TEST_ASSERT_TRUE(Wifi_IsValidChannel_C(1));
    TEST_ASSERT_TRUE(Wifi_IsValidChannel_C(6));
    TEST_ASSERT_TRUE(Wifi_IsValidChannel_C(11));
    TEST_ASSERT_TRUE(Wifi_IsValidChannel_C(14));
}

void test_channel_invalid_cases() {
    TEST_ASSERT_FALSE(Wifi_IsValidChannel_C(0));
    TEST_ASSERT_FALSE(Wifi_IsValidChannel_C(15));
    TEST_ASSERT_FALSE(Wifi_IsValidChannel_C(-1));
    TEST_ASSERT_FALSE(Wifi_IsValidChannel_C(100));
}

// Tests de détection de sécurité
void test_secure_connection_detection() {
    TEST_ASSERT_TRUE(Wifi_ShouldUseSecureConnection_C("MyHomeNetwork"));
    TEST_ASSERT_TRUE(Wifi_ShouldUseSecureConnection_C("Office-WiFi"));
    
    TEST_ASSERT_FALSE(Wifi_ShouldUseSecureConnection_C("FREE-WiFi"));
    TEST_ASSERT_FALSE(Wifi_ShouldUseSecureConnection_C("GUEST-Network"));
    TEST_ASSERT_FALSE(Wifi_ShouldUseSecureConnection_C("PUBLIC-Access"));
    TEST_ASSERT_FALSE(Wifi_ShouldUseSecureConnection_C("OPEN-Hotspot"));
    TEST_ASSERT_FALSE(Wifi_ShouldUseSecureConnection_C("HOTSPOT-Cafe"));
}

// Tests d'estimation de temps de connexion
void test_connection_time_estimation() {
    int time1 = Wifi_EstimateConnectionTime_C("ShortSSID", false);
    int time2 = Wifi_EstimateConnectionTime_C("ShortSSID", true);
    int time3 = Wifi_EstimateConnectionTime_C("VeryLongSSIDNameThatExceeds20Characters", true);
    
    TEST_ASSERT_EQUAL(5000, time1);  // Pas de mot de passe
    TEST_ASSERT_EQUAL(8000, time2);  // Avec mot de passe (+3s)
    TEST_ASSERT_EQUAL(10000, time3); // Long SSID + mot de passe (+2s supplémentaires)
    
    // Cas par défaut
    TEST_ASSERT_EQUAL(15000, Wifi_EstimateConnectionTime_C(nullptr, false));
}

int main() {
    UNITY_BEGIN();
    
    // Tests SSID
    RUN_TEST(test_ssid_valid_cases);
    RUN_TEST(test_ssid_invalid_cases);
    
    // Tests mot de passe
    RUN_TEST(test_password_valid_cases);
    RUN_TEST(test_password_invalid_cases);
    
    // Tests canal
    RUN_TEST(test_channel_valid_cases);
    RUN_TEST(test_channel_invalid_cases);
    
    // Tests sécurité
    RUN_TEST(test_secure_connection_detection);
    
    // Tests temps de connexion
    RUN_TEST(test_connection_time_estimation);
    
    return UNITY_END();
}
