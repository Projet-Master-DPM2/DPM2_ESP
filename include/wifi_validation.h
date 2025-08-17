#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// Validation des paramètres Wi-Fi
bool Wifi_IsValidSsid_C(const char* ssid);
bool Wifi_IsValidPassword_C(const char* password);
bool Wifi_IsValidChannel_C(int channel);

// Utilitaires de configuration Wi-Fi
bool Wifi_ShouldUseSecureConnection_C(const char* ssid);
int Wifi_EstimateConnectionTime_C(const char* ssid, bool hasPassword);

#ifdef __cplusplus
}
#endif
