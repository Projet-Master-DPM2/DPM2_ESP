#include "../../include/wifi_validation.h"
#include <string.h>

bool Wifi_IsValidSsid_C(const char* ssid) {
    if (!ssid) return false;
    size_t len = strlen(ssid);
    if (len == 0 || len > 32) return false; // SSID max 32 bytes selon IEEE 802.11
    
    // Vérifier les caractères autorisés (UTF-8 basique)
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)ssid[i];
        // Rejeter les caractères de contrôle
        if (c < 0x20 && c != 0x09) return false; // Sauf TAB
        // Rejeter DEL
        if (c == 0x7F) return false;
    }
    
    return true;
}

bool Wifi_IsValidPassword_C(const char* password) {
    if (!password) return true; // Réseau ouvert autorisé
    size_t len = strlen(password);
    if (len == 0) return true; // Mot de passe vide = réseau ouvert
    
    // WPA/WPA2: 8-63 caractères ASCII ou 64 caractères hex
    if (len < 8) return false;
    if (len > 64) return false;
    
    // Si exactement 64 caractères, doit être hexadécimal (PSK)
    if (len == 64) {
        for (size_t i = 0; i < len; i++) {
            char c = password[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
                return false;
            }
        }
    } else {
        // Sinon, vérifier les caractères ASCII imprimables
        for (size_t i = 0; i < len; i++) {
            unsigned char c = (unsigned char)password[i];
            if (c < 0x20 || c > 0x7E) return false;
        }
    }
    
    return true;
}

bool Wifi_IsValidChannel_C(int channel) {
    // Canaux Wi-Fi 2.4GHz standards
    return (channel >= 1 && channel <= 14);
}

bool Wifi_ShouldUseSecureConnection_C(const char* ssid) {
    if (!ssid) return true;
    
    // Heuristiques pour détecter les réseaux publics non sécurisés
    const char* publicPrefixes[] = {"FREE", "GUEST", "PUBLIC", "OPEN", "HOTSPOT"};
    const size_t numPrefixes = sizeof(publicPrefixes) / sizeof(publicPrefixes[0]);
    
    for (size_t i = 0; i < numPrefixes; i++) {
        if (strstr(ssid, publicPrefixes[i]) != NULL) {
            return false; // Réseau potentiellement public
        }
    }
    
    return true; // Par défaut, utiliser une connexion sécurisée
}

int Wifi_EstimateConnectionTime_C(const char* ssid, bool hasPassword) {
    if (!ssid) return 15000; // 15s par défaut
    
    int baseTime = 5000; // 5s de base
    
    // Ajouter du temps pour l'authentification
    if (hasPassword) {
        baseTime += 3000; // +3s pour WPA/WPA2
    }
    
    // Ajouter du temps selon la longueur du SSID (heuristique)
    size_t len = strlen(ssid);
    if (len > 20) {
        baseTime += 2000; // +2s pour les SSID longs
    }
    
    return baseTime;
}
