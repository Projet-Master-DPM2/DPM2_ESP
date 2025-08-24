#include "env_config.h"
#include "FS.h"
#include "SPIFFS.h"

// Variables statiques
ApiConfig EnvConfig::config = {};
bool EnvConfig::initialized = false;

bool EnvConfig::Initialize() {
  if (initialized) {
    return true;
  }
  
  Serial.println("[ENV] Initializing configuration...");
  
  // Initialiser SPIFFS si pas déjà fait
  if (!SPIFFS.begin(true)) {
    Serial.println("[ENV] SPIFFS Mount Failed, using defaults");
    LoadDefaults();
    initialized = true;
    return false;
  }
  
  // Charger les valeurs par défaut d'abord
  LoadDefaults();
  
  // Essayer de charger le fichier .env
  if (LoadEnvFile()) {
    Serial.println("[ENV] Configuration loaded from .env file");
    config.loaded_from_env = true;
  } else {
    Serial.println("[ENV] Using default configuration");
    config.loaded_from_env = false;
  }
  
  initialized = true;
  PrintConfig();
  return true;
}

String EnvConfig::GetValidateTokenUrl() {
  if (!initialized) Initialize();
  return String(config.api_base_url) + String(config.api_validate_endpoint);
}

String EnvConfig::GetStockUpdateUrl() {
  if (!initialized) Initialize();
  return String(config.api_base_url) + String(config.api_stock_endpoint);
}

String EnvConfig::GetOrderStatusUrl() {
  if (!initialized) Initialize();
  return String(config.api_base_url) + String(config.api_order_status_endpoint);
}

String EnvConfig::GetDeliveryConfirmUrl() {
  if (!initialized) Initialize();
  return String(config.api_base_url) + String(config.api_delivery_confirm_endpoint);
}

String EnvConfig::GetUpdateQuantitiesUrl() {
  if (!initialized) Initialize();
  return String(config.api_base_url) + String(config.api_update_quantities_endpoint);
}

const char* EnvConfig::GetApiBaseUrl() {
  if (!initialized) Initialize();
  return config.api_base_url;
}

const char* EnvConfig::GetValidateEndpoint() {
  if (!initialized) Initialize();
  return config.api_validate_endpoint;
}

const char* EnvConfig::GetStockEndpoint() {
  if (!initialized) Initialize();
  return config.api_stock_endpoint;
}

const char* EnvConfig::GetOrderStatusEndpoint() {
  if (!initialized) Initialize();
  return config.api_order_status_endpoint;
}

const char* EnvConfig::GetDeliveryConfirmEndpoint() {
  if (!initialized) Initialize();
  return config.api_delivery_confirm_endpoint;
}

const char* EnvConfig::GetUpdateQuantitiesEndpoint() {
  if (!initialized) Initialize();
  return config.api_update_quantities_endpoint;
}

String EnvConfig::GetSupervisionUrl() {
  if (!initialized) Initialize();
  return String(config.api_base_url) + String(config.api_supervision_endpoint);
}

const char* EnvConfig::GetSupervisionEndpoint() {
  if (!initialized) Initialize();
  return config.api_supervision_endpoint;
}

bool EnvConfig::IsLoadedFromEnv() {
  if (!initialized) Initialize();
  return config.loaded_from_env;
}

void EnvConfig::PrintConfig() {
  Serial.println("[ENV] === API Configuration ===");
  Serial.printf("Base URL: %s\n", config.api_base_url);
  Serial.printf("Validate: %s\n", config.api_validate_endpoint);
  Serial.printf("Stock: %s\n", config.api_stock_endpoint);
  Serial.printf("Status: %s\n", config.api_order_status_endpoint);
  Serial.printf("Delivery: %s\n", config.api_delivery_confirm_endpoint);
  Serial.printf("Quantities: %s\n", config.api_update_quantities_endpoint);
  Serial.printf("Supervision: %s\n", config.api_supervision_endpoint);
  Serial.printf("Source: %s\n", config.loaded_from_env ? ".env file" : "defaults");
  Serial.println("[ENV] === End Configuration ===");
}

bool EnvConfig::LoadEnvFile() {
  File file = SPIFFS.open("/.env", "r");
  if (!file) {
    Serial.println("[ENV] .env file not found");
    return false;
  }
  
  Serial.println("[ENV] Reading .env file...");
  int lineCount = 0;
  int validLines = 0;
  
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    lineCount++;
    
    // Ignorer les lignes vides et les commentaires
    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }
    
    if (ParseEnvLine(line)) {
      validLines++;
    } else {
      Serial.printf("[ENV] Invalid line %d: %s\n", lineCount, line.c_str());
    }
  }
  
  file.close();
  Serial.printf("[ENV] Processed %d lines, %d valid configurations\n", lineCount, validLines);
  return validLines > 0;
}

void EnvConfig::LoadDefaults() {
  strncpy(config.api_base_url, DEFAULT_API_BASE_URL, MAX_URL_LENGTH - 1);
  strncpy(config.api_validate_endpoint, DEFAULT_API_VALIDATE_ENDPOINT, MAX_ENDPOINT_LENGTH - 1);
  strncpy(config.api_stock_endpoint, DEFAULT_API_STOCK_ENDPOINT, MAX_ENDPOINT_LENGTH - 1);
  strncpy(config.api_order_status_endpoint, DEFAULT_API_ORDER_STATUS_ENDPOINT, MAX_ENDPOINT_LENGTH - 1);
  strncpy(config.api_delivery_confirm_endpoint, DEFAULT_API_DELIVERY_CONFIRM_ENDPOINT, MAX_ENDPOINT_LENGTH - 1);
  strncpy(config.api_update_quantities_endpoint, DEFAULT_API_UPDATE_QUANTITIES_ENDPOINT, MAX_ENDPOINT_LENGTH - 1);
  strncpy(config.api_supervision_endpoint, DEFAULT_API_SUPERVISION_ENDPOINT, MAX_ENDPOINT_LENGTH - 1);
  
  config.api_base_url[MAX_URL_LENGTH - 1] = '\0';
  config.api_validate_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  config.api_stock_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  config.api_order_status_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  config.api_delivery_confirm_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  config.api_update_quantities_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  config.api_supervision_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  
  config.loaded_from_env = false;
}

bool EnvConfig::ParseEnvLine(const String& line) {
  int equalPos = line.indexOf('=');
  if (equalPos == -1) {
    return false;
  }
  
  String key = line.substring(0, equalPos);
  String value = line.substring(equalPos + 1);
  
  key.trim();
  value.trim();
  
  // Supprimer les guillemets si présents
  if ((value.startsWith("\"") && value.endsWith("\"")) ||
      (value.startsWith("'") && value.endsWith("'"))) {
    value = value.substring(1, value.length() - 1);
  }
  
  if (key.length() == 0 || value.length() == 0) {
    return false;
  }
  
  SetConfigValue(key, value);
  Serial.printf("[ENV] Set %s = %s\n", key.c_str(), value.c_str());
  return true;
}

void EnvConfig::SetConfigValue(const String& key, const String& value) {
  if (key == "API_BASE_URL") {
    strncpy(config.api_base_url, value.c_str(), MAX_URL_LENGTH - 1);
    config.api_base_url[MAX_URL_LENGTH - 1] = '\0';
  }
  else if (key == "API_VALIDATE_ENDPOINT") {
    strncpy(config.api_validate_endpoint, value.c_str(), MAX_ENDPOINT_LENGTH - 1);
    config.api_validate_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  }
  else if (key == "API_STOCK_ENDPOINT") {
    strncpy(config.api_stock_endpoint, value.c_str(), MAX_ENDPOINT_LENGTH - 1);
    config.api_stock_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  }
  else if (key == "API_ORDER_STATUS_ENDPOINT") {
    strncpy(config.api_order_status_endpoint, value.c_str(), MAX_ENDPOINT_LENGTH - 1);
    config.api_order_status_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  }
  else if (key == "API_DELIVERY_CONFIRM_ENDPOINT") {
    strncpy(config.api_delivery_confirm_endpoint, value.c_str(), MAX_ENDPOINT_LENGTH - 1);
    config.api_delivery_confirm_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  }
  else if (key == "API_UPDATE_QUANTITIES_ENDPOINT") {
    strncpy(config.api_update_quantities_endpoint, value.c_str(), MAX_ENDPOINT_LENGTH - 1);
    config.api_update_quantities_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  }
  else if (key == "API_SUPERVISION_ENDPOINT") {
    strncpy(config.api_supervision_endpoint, value.c_str(), MAX_ENDPOINT_LENGTH - 1);
    config.api_supervision_endpoint[MAX_ENDPOINT_LENGTH - 1] = '\0';
  }
  else {
    Serial.printf("[ENV] Unknown configuration key: %s\n", key.c_str());
  }
}
