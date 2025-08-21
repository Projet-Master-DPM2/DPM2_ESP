#include "security_config.h"
#include <Arduino.h>
#include <mbedtls/md.h>
#include <mbedtls/aes.h>

// =============================================================================
// VARIABLES GLOBALES DE SÉCURITÉ
// =============================================================================

static LogLevel currentLogLevel = DEFAULT_LOG_LEVEL;
static unsigned long lastRateLimitCheck[10] = {0}; // Support 10 services différents
static const char* rateLimitServices[] = {"NFC", "UART", "HTTP", "WIFI", "QR", "", "", "", "", ""};

// =============================================================================
// GESTION DU NIVEAU DE LOG
// =============================================================================

LogLevel getLogLevel() {
    return currentLogLevel;
}

void setLogLevel(LogLevel level) {
    currentLogLevel = level;
    Serial.printf("[SEC] Log level set to %d\n", (int)level);
}

// =============================================================================
// MASQUAGE DES DONNÉES SENSIBLES
// =============================================================================

String maskSensitiveData(const String& data, int visibleChars) {
    if (!MASK_SENSITIVE_DATA || data.length() <= visibleChars) {
        return data;
    }
    
    String masked = data.substring(0, visibleChars);
    for (int i = visibleChars; i < data.length(); i++) {
        masked += "*";
    }
    return masked;
}

String maskUID(const String& uid) {
    return maskSensitiveData(uid, UID_MASK_LENGTH);
}

String maskSSID(const String& ssid) {
    return maskSensitiveData(ssid, WIFI_SSID_MASK_LENGTH);
}

String maskPassword(const String& password) {
    return String("***") + String(password.length()) + "chars***";
}

// =============================================================================
// VALIDATION D'ENTRÉES SÉCURISÉE
// =============================================================================

bool isValidChar(char c, const char* allowedChars) {
    if (!allowedChars) return false;
    return strchr(allowedChars, c) != nullptr;
}

bool isValidString(const char* str, const char* allowedChars, size_t maxLength) {
    if (!str || !allowedChars) return false;
    
    size_t len = strlen(str);
    if (len == 0 || len > maxLength) return false;
    
    for (size_t i = 0; i < len; i++) {
        if (!isValidChar(str[i], allowedChars)) {
            return false;
        }
    }
    return true;
}

bool isValidUrl(const char* url) {
    if (!url) return false;
    
    // Vérifier la longueur
    if (strlen(url) > MAX_URL_LENGTH) return false;
    
    // Vérifier le protocole
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        return false;
    }
    
    // Vérifier les caractères autorisés
    return isValidString(url, ALLOWED_URL_CHARS, MAX_URL_LENGTH);
}

bool isValidSSID(const char* ssid) {
    if (!ssid) return false;
    return isValidString(ssid, ALLOWED_SSID_CHARS, MAX_SSID_LENGTH);
}

bool isValidNFCData(const char* data) {
    if (!data) return false;
    return isValidString(data, ALLOWED_NFC_CHARS, MAX_NFC_DATA_LENGTH);
}

// =============================================================================
// RATE LIMITING
// =============================================================================

int findServiceIndex(const char* serviceName) {
    for (int i = 0; i < 10; i++) {
        if (strcmp(rateLimitServices[i], serviceName) == 0) {
            return i;
        }
        if (strlen(rateLimitServices[i]) == 0) {
            // Slot libre, l'assigner
            strncpy((char*)rateLimitServices[i], serviceName, 9);
            rateLimitServices[i][9] = '\0';
            return i;
        }
    }
    return -1; // Pas de slot libre
}

bool rateLimitCheck(const char* service, unsigned long cooldownMs) {
    if (!service) return false;
    
    int index = findServiceIndex(service);
    if (index < 0) return false; // Pas de slot disponible
    
    unsigned long now = millis();
    if (now - lastRateLimitCheck[index] < cooldownMs) {
        SECURE_LOG_ERROR("RATE", "Rate limit exceeded for %s", service);
        return false;
    }
    
    lastRateLimitCheck[index] = now;
    return true;
}

// =============================================================================
// CHIFFREMENT DES CREDENTIALS
// =============================================================================

// Génération d'une clé dérivée à partir du salt
bool deriveKey(const char* password, const char* salt, uint8_t* key, size_t keyLen) {
    if (!password || !salt || !key) return false;
    
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info) {
        mbedtls_md_free(&ctx);
        return false;
    }
    
    if (mbedtls_md_setup(&ctx, info, 1) != 0) {
        mbedtls_md_free(&ctx);
        return false;
    }
    
    // Utiliser PBKDF2 simplifié (itérations multiples)
    String combined = String(password) + String(salt);
    const uint8_t* input = (const uint8_t*)combined.c_str();
    size_t inputLen = combined.length();
    
    // Faire 1000 itérations pour renforcer la sécurité
    uint8_t hash[32];
    for (int i = 0; i < 1000; i++) {
        if (mbedtls_md_starts(&ctx) != 0 ||
            mbedtls_md_update(&ctx, input, inputLen) != 0 ||
            mbedtls_md_finish(&ctx, hash) != 0) {
            mbedtls_md_free(&ctx);
            return false;
        }
        input = hash;
        inputLen = sizeof(hash);
    }
    
    // Copier la clé de la taille demandée
    memcpy(key, hash, min(keyLen, sizeof(hash)));
    
    mbedtls_md_free(&ctx);
    return true;
}

// Chiffrement AES simple
bool encryptData(const char* plaintext, const uint8_t* key, char* ciphertext, size_t maxLen) {
    if (!plaintext || !key || !ciphertext) return false;
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    
    if (mbedtls_aes_setkey_enc(&aes, key, 256) != 0) {
        mbedtls_aes_free(&aes);
        return false;
    }
    
    size_t len = strlen(plaintext);
    if (len + 16 > maxLen) { // AES block size
        mbedtls_aes_free(&aes);
        return false;
    }
    
    // Padding simple (PKCS7)
    uint8_t padded[256];
    memcpy(padded, plaintext, len);
    uint8_t padValue = 16 - (len % 16);
    for (int i = 0; i < padValue; i++) {
        padded[len + i] = padValue;
    }
    len += padValue;
    
    // Chiffrer par blocs de 16 bytes
    for (size_t i = 0; i < len; i += 16) {
        if (mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, 
                                  padded + i, (uint8_t*)ciphertext + i) != 0) {
            mbedtls_aes_free(&aes);
            return false;
        }
    }
    
    mbedtls_aes_free(&aes);
    
    // Convertir en hex pour stockage
    char* hex = ciphertext;
    for (size_t i = 0; i < len && (i * 2 + 1) < maxLen; i++) {
        sprintf(hex + i * 2, "%02x", (uint8_t)ciphertext[i]);
    }
    hex[len * 2] = '\0';
    
    return true;
}

// Déchiffrement AES simple
bool decryptData(const char* ciphertext, const uint8_t* key, char* plaintext, size_t maxLen) {
    if (!ciphertext || !key || !plaintext) return false;
    
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    
    if (mbedtls_aes_setkey_dec(&aes, key, 256) != 0) {
        mbedtls_aes_free(&aes);
        return false;
    }
    
    // Convertir hex vers bytes
    size_t hexLen = strlen(ciphertext);
    if (hexLen % 2 != 0 || hexLen > 512) {
        mbedtls_aes_free(&aes);
        return false;
    }
    
    uint8_t encrypted[256];
    size_t encLen = hexLen / 2;
    
    for (size_t i = 0; i < encLen; i++) {
        sscanf(ciphertext + i * 2, "%2hhx", &encrypted[i]);
    }
    
    // Déchiffrer par blocs
    uint8_t decrypted[256];
    for (size_t i = 0; i < encLen; i += 16) {
        if (mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT,
                                  encrypted + i, decrypted + i) != 0) {
            mbedtls_aes_free(&aes);
            return false;
        }
    }
    
    mbedtls_aes_free(&aes);
    
    // Retirer le padding
    if (encLen > 0) {
        uint8_t padValue = decrypted[encLen - 1];
        if (padValue > 0 && padValue <= 16) {
            encLen -= padValue;
        }
    }
    
    if (encLen >= maxLen) {
        return false;
    }
    
    memcpy(plaintext, decrypted, encLen);
    plaintext[encLen] = '\0';
    
    return true;
}

// =============================================================================
// MONITORING ET DIAGNOSTICS
// =============================================================================

void logSecurityEvent(const char* event, const char* details) {
    SECURE_LOG_INFO("SEC", "Security event: %s - %s", event ? event : "unknown", details ? details : "no details");
}

void checkSystemSecurity() {
    // Vérifier la mémoire disponible
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < MIN_FREE_HEAP_BYTES) {
        SECURE_LOG_ERROR("SEC", "Low heap memory: %zu bytes", freeHeap);
        logSecurityEvent("LOW_MEMORY", String("Heap: " + String(freeHeap) + " bytes").c_str());
    }
    
    // Vérifier les stack overflows (basique)
    if (uxTaskGetStackHighWaterMark(NULL) < 512) {
        SECURE_LOG_ERROR("SEC", "Stack overflow risk detected");
        logSecurityEvent("STACK_RISK", "Low stack space");
    }
}

// =============================================================================
// FONCTIONS UTILITAIRES SUPPLÉMENTAIRES
// =============================================================================

// Génération d'un ID de session sécurisé
String generateSessionId() {
    String sessionId = "";
    uint32_t random1 = esp_random();
    uint32_t random2 = esp_random();
    uint32_t timestamp = millis();
    
    sessionId = String(random1, HEX) + String(random2, HEX) + String(timestamp, HEX);
    return sessionId.substring(0, 16); // Limiter à 16 caractères
}

// Validation de l'intégrité d'une chaîne
bool validateStringIntegrity(const char* str, size_t expectedMaxLen) {
    if (!str) return false;
    
    // Vérifier que la chaîne est bien terminée dans la limite
    for (size_t i = 0; i <= expectedMaxLen; i++) {
        if (str[i] == '\0') return true;
    }
    return false; // Pas de terminaison trouvée
}

