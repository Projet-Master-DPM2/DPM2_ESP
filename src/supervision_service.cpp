#include "supervision_service.h"
#include "services/http_service.h"
#include "env_config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include <esp_random.h>

// Variables statiques
String SupervisionService::machine_id = "";
bool SupervisionService::is_initialized = false;
unsigned long SupervisionService::last_notification_time = 0;

void SupervisionService::Initialize() {
  if (is_initialized) {
    return;
  }
  
  // Générer un ID de machine unique basé sur l'ESP32
  if (machine_id.length() == 0) {
    uint64_t chip_id = ESP.getEfuseMac();
    machine_id = "esp32_" + String((uint32_t)(chip_id >> 32), HEX) + String((uint32_t)chip_id, HEX);
  }
  
  is_initialized = true;
  Serial.println("[SUPERVISION] Service initialized with machine ID: " + machine_id);
}

void SupervisionService::SendErrorNotification(SupervisionErrorType error_type, const String& message) {
  if (!is_initialized) {
    Initialize();
  }
  
  if (!ShouldSendNotification()) {
    Serial.println("[SUPERVISION] Notification skipped due to cooldown period");
    return;
  }
  
  SupervisionEvent event;
  event.error_id = GenerateErrorId();
  event.machine_id = machine_id;
  event.error_type = error_type;
  event.message = message;
  event.timestamp = millis();
  
  SendErrorNotification(event);
}

void SupervisionService::SendErrorNotification(const SupervisionEvent& event) {
  if (!is_initialized) {
    Initialize();
  }
  
  if (!ShouldSendNotification()) {
    Serial.println("[SUPERVISION] Notification skipped due to cooldown period");
    return;
  }
  
  // Vérifier la connectivité réseau
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SUPERVISION] Cannot send notification: WiFi not connected");
    return;
  }
  
  // Construire le payload JSON
  String json_payload = "{";
  json_payload += "\"error_id\":\"" + event.error_id + "\",";
  json_payload += "\"machine_id\":\"" + event.machine_id + "\",";
  json_payload += "\"error_type\":\"" + ErrorTypeToString(event.error_type) + "\",";
  json_payload += "\"message\":\"" + event.message + "\"";
  json_payload += "}";
  
  Serial.println("[SUPERVISION] Sending error notification:");
  Serial.println("  Error ID: " + event.error_id);
  Serial.println("  Machine ID: " + event.machine_id);
  Serial.println("  Error Type: " + ErrorTypeToString(event.error_type));
  Serial.println("  Message: " + event.message);
  Serial.println("  Payload: " + json_payload);
  
  // Envoyer la notification via HTTP
  String url = EnvConfig::GetSupervisionUrl();
  if (url.length() > 0) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "DPM2-ESP32-Supervision/1.0");
    
    int http_response_code = http.POST(json_payload);
    
    if (http_response_code == 200 || http_response_code == 201) {
      Serial.println("[SUPERVISION] Error notification sent successfully");
      last_notification_time = millis();
    } else {
      Serial.printf("[SUPERVISION] Failed to send error notification. HTTP Code: %d\n", http_response_code);
      String response = http.getString();
      Serial.println("[SUPERVISION] Response: " + response);
    }
    
    http.end();
  } else {
    Serial.println("[SUPERVISION] Error: Supervision URL not configured");
  }
}

String SupervisionService::GenerateErrorId() {
  unsigned long timestamp = millis();
  uint32_t random_val = esp_random();
  String error_id = "err_" + String(timestamp, HEX) + "_" + String(random_val, HEX);
  return error_id;
}

String SupervisionService::GetMachineId() {
  if (!is_initialized) {
    Initialize();
  }
  return machine_id;
}

void SupervisionService::SetMachineId(const String& new_machine_id) {
  machine_id = new_machine_id;
  Serial.println("[SUPERVISION] Machine ID set to: " + machine_id);
}

String SupervisionService::ErrorTypeToString(SupervisionErrorType error_type) {
  switch (error_type) {
    case SUPERVISION_ERROR_WATCHDOG_TIMEOUT:
      return "WATCHDOG_TIMEOUT";
    case SUPERVISION_ERROR_WATCHDOG_RESET:
      return "WATCHDOG_RESET";
    case SUPERVISION_ERROR_TASK_HANG:
      return "TASK_HANG";
    case SUPERVISION_ERROR_MEMORY_LOW:
      return "MEMORY_LOW";
    case SUPERVISION_ERROR_NETWORK_DISCONNECTED:
      return "NETWORK_DISCONNECTED";
    case SUPERVISION_ERROR_CRITICAL_SERVICE_FAILURE:
      return "CRITICAL_SERVICE_FAILURE";
    case SUPERVISION_ERROR_HARDWARE_FAULT:
      return "HARDWARE_FAULT";
    case SUPERVISION_ERROR_SYSTEM_CRASH:
      return "SYSTEM_CRASH";
    default:
      return "UNKNOWN_ERROR";
  }
}

bool SupervisionService::ShouldSendNotification() {
  unsigned long current_time = millis();
  if (current_time - last_notification_time < NOTIFICATION_COOLDOWN_MS) {
    return false;
  }
  return true;
}
