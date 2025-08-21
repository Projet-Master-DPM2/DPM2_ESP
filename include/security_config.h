#pragma once

// Configuration de sécurité OWASP pour ESP32
// Implémentation des recommandations de sécurité critiques

// =============================================================================
// CONFIGURATION TLS/HTTPS
// =============================================================================

// Certificate Authority Root pour validation TLS
// Certificat Let's Encrypt ISRG Root X1 (format PEM)
static const char* CA_CERT = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n" \
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n" \
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n" \
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n" \
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n" \
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n" \
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n" \
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n" \
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n" \
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n" \
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n" \
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n" \
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n" \
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n" \
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n" \
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n" \
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n" \
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n" \
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n" \
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n" \
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n" \
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n" \
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n" \
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n" \
"-----END CERTIFICATE-----\n";

// Configuration sécurité TLS
#define TLS_CERT_VALIDATION_ENABLED     true
#define TLS_HOSTNAME_VERIFICATION       true
#define TLS_MIN_VERSION                 1.2f  // Minimum TLS 1.2
#define HTTP_TIMEOUT_MS                 10000
#define HTTPS_TIMEOUT_MS                15000

// =============================================================================
// CONFIGURATION CHIFFREMENT NVS
// =============================================================================

// Clé de chiffrement pour NVS (doit être changée en production)
#define NVS_ENCRYPTION_KEY_SIZE         32
#define NVS_NAMESPACE_SECURE            "secure_dpm"
#define WIFI_CREDS_ENCRYPTION_ENABLED   true

// Salt pour le hachage des mots de passe
static const char* PASSWORD_SALT = "DPM2_ESP32_SALT_2024";

// =============================================================================
// CONFIGURATION LOGGING SÉCURISÉ
// =============================================================================

typedef enum {
    LOG_LEVEL_ERROR = 0,    // Erreurs uniquement
    LOG_LEVEL_WARN = 1,     // Avertissements + erreurs
    LOG_LEVEL_INFO = 2,     // Informations + warn + erreurs
    LOG_LEVEL_DEBUG = 3,    // Debug complet (dev uniquement)
    LOG_LEVEL_VERBOSE = 4   // Très verbeux (dev uniquement)
} LogLevel;

// Niveau de log par défaut (production = INFO, dev = DEBUG)
#ifdef DEBUG_MODE
#define DEFAULT_LOG_LEVEL LOG_LEVEL_DEBUG
#else
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO
#endif

// Masquage des données sensibles
#define MASK_SENSITIVE_DATA             true
#define UID_MASK_LENGTH                 4    // Afficher seulement 4 chars des UIDs
#define WIFI_SSID_MASK_LENGTH          8    // Afficher seulement 8 chars des SSIDs

// =============================================================================
// CONFIGURATION RATE LIMITING
// =============================================================================

// Rate limiting NFC
#define NFC_MAX_SCANS_PER_MINUTE        10
#define NFC_SCAN_COOLDOWN_MS            3000

// Rate limiting UART
#define UART_MAX_COMMANDS_PER_SECOND    5
#define UART_COMMAND_COOLDOWN_MS        200

// Rate limiting HTTP
#define HTTP_MAX_REQUESTS_PER_MINUTE    20
#define HTTP_REQUEST_COOLDOWN_MS        3000

// =============================================================================
// VALIDATION D'ENTRÉES
// =============================================================================

// Tailles maximales sécurisées
#define MAX_SSID_LENGTH                 32
#define MAX_PASSWORD_LENGTH             64
#define MAX_URL_LENGTH                  256
#define MAX_UART_LINE_LENGTH            128
#define MAX_NFC_DATA_LENGTH             128
#define MAX_QR_DATA_LENGTH              256

// Caractères autorisés
#define ALLOWED_SSID_CHARS              "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_. "
#define ALLOWED_URL_CHARS               "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_./:?&=+%"
#define ALLOWED_NFC_CHARS               "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_. :,;!?"

// =============================================================================
// MONITORING ET WATCHDOG
// =============================================================================

#define HEAP_MONITOR_INTERVAL_MS        30000  // Vérifier heap toutes les 30s
#define MIN_FREE_HEAP_BYTES             10240  // 10KB minimum
#define WATCHDOG_TIMEOUT_MS             30000  // 30s timeout watchdog
#define TASK_HEARTBEAT_INTERVAL_MS      5000   // Heartbeat toutes les 5s

// =============================================================================
// MACROS DE SÉCURITÉ
// =============================================================================

// Macro pour validation sécurisée des pointeurs
#define SAFE_CHECK_PTR(ptr) do { if (!(ptr)) { Serial.println("[SEC] Null pointer detected"); return false; } } while(0)

// Macro pour validation des tailles
#define SAFE_CHECK_SIZE(size, max) do { if ((size) > (max)) { Serial.println("[SEC] Size limit exceeded"); return false; } } while(0)

// Macro pour nettoyage sécurisé de la mémoire
#define SECURE_ZERO(ptr, size) do { if (ptr) memset((ptr), 0, (size)); } while(0)

// Macro pour logging sécurisé avec masquage
#define SECURE_LOG_INFO(tag, format, ...) do { if (getLogLevel() >= LOG_LEVEL_INFO) Serial.printf("[" tag "] " format "\n", ##__VA_ARGS__); } while(0)
#define SECURE_LOG_ERROR(tag, format, ...) do { if (getLogLevel() >= LOG_LEVEL_ERROR) Serial.printf("[ERR][" tag "] " format "\n", ##__VA_ARGS__); } while(0)

// Déclarations des fonctions utilitaires
LogLevel getLogLevel();
void setLogLevel(LogLevel level);
String maskSensitiveData(const String& data, int visibleChars);
bool isValidUrl(const char* url);
bool isValidSSID(const char* ssid);
bool rateLimitCheck(const char* service, unsigned long cooldownMs);

