#pragma once

#include <Arduino.h>

// Configuration par défaut (fallback si .env n'est pas trouvé)
#define DEFAULT_API_BASE_URL "https://iot-vending-machine.osc-fr1.scalingo.io"
#define DEFAULT_API_VALIDATE_ENDPOINT "/api/order-validation/validate-token"
#define DEFAULT_API_STOCK_ENDPOINT "/api/stock/update"
#define DEFAULT_API_ORDER_STATUS_ENDPOINT "/api/order/update-status"

// Tailles maximales pour les URLs
#define MAX_URL_LENGTH 256
#define MAX_ENDPOINT_LENGTH 128

// Structure pour stocker la configuration
typedef struct {
  char api_base_url[MAX_URL_LENGTH];
  char api_validate_endpoint[MAX_ENDPOINT_LENGTH];
  char api_stock_endpoint[MAX_ENDPOINT_LENGTH];
  char api_order_status_endpoint[MAX_ENDPOINT_LENGTH];
  bool loaded_from_env;
} ApiConfig;

class EnvConfig {
private:
  static ApiConfig config;
  static bool initialized;
  
public:
  // Initialisation de la configuration
  static bool Initialize();
  
  // Getters pour les URLs complètes
  static String GetValidateTokenUrl();
  static String GetStockUpdateUrl();
  static String GetOrderStatusUrl();
  
  // Getters pour les composants
  static const char* GetApiBaseUrl();
  static const char* GetValidateEndpoint();
  static const char* GetStockEndpoint();
  static const char* GetOrderStatusEndpoint();
  
  // Utilitaires
  static bool IsLoadedFromEnv();
  static void PrintConfig();
  
private:
  // Chargement du fichier .env
  static bool LoadEnvFile();
  static void LoadDefaults();
  static bool ParseEnvLine(const String& line);
  static void SetConfigValue(const String& key, const String& value);
};
