#include <Arduino.h>
#include "services/http_service.h"
#include "services/wifi_service.h"
#include "security_config.h"
#include "env_config.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static TaskHandle_t httpTaskHandle = nullptr;
static QueueHandle_t httpRequestQueue = nullptr;

// Configuration TLS sécurisée
static WiFiClientSecure* createSecureClient(const char* url) {
  WiFiClientSecure* sclient = new WiFiClientSecure();
  
  if (TLS_CERT_VALIDATION_ENABLED) {
    // Utiliser le certificat CA pour validation
    sclient->setCACert(CA_CERT);
    
    if (TLS_HOSTNAME_VERIFICATION) {
      // Extraire le hostname de l'URL pour vérification
      String hostname = "";
      if (strncmp(url, "https://", 8) == 0) {
        hostname = String(url + 8);
        int slashPos = hostname.indexOf('/');
        if (slashPos > 0) {
          hostname = hostname.substring(0, slashPos);
        }
      }
      
      if (hostname.length() > 0) {
        SECURE_LOG_INFO("HTTP", "TLS hostname verification: %s", hostname.c_str());
      }
    }
    
    SECURE_LOG_INFO("HTTP", "TLS certificate validation enabled");
  } else {
    // Mode non-sécurisé (dev uniquement)
    sclient->setInsecure();
    SECURE_LOG_WARN("HTTP", "TLS validation disabled - DEV MODE ONLY");
  }
  
  return sclient;
}

static void httpTask(void* pv) {
  HTTPClient client;
  HttpRequest req;
  
  for (;;) {
    if (xQueueReceive(httpRequestQueue, &req, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    
    // Vérification WiFi
    if (!WifiService_IsReady()) {
      SECURE_LOG_ERROR("HTTP", "Request ignored: WiFi not ready");
      continue;
    }
    
    // Rate limiting
    if (!rateLimitCheck("HTTP", HTTP_REQUEST_COOLDOWN_MS)) {
      SECURE_LOG_ERROR("HTTP", "Request rate limited");
      continue;
    }
    
    // Validation de l'URL
    if (!isValidUrl(req.url)) {
      SECURE_LOG_ERROR("HTTP", "Invalid URL rejected: %s", maskSensitiveData(String(req.url), 20).c_str());
      continue;
    }
    
    HttpResponse resp = {};
    resp.statusCode = 0;
    resp.contentLength = 0;
    resp.payload[0] = '\0';
    bool ok = false;
    
    // Timeout sécurisé
    uint32_t timeout = req.timeoutMs > 0 ? req.timeoutMs : 
                      (req.https ? HTTPS_TIMEOUT_MS : HTTP_TIMEOUT_MS);
    client.setTimeout(timeout);
    
    // Monitoring sécurité avant la requête
    checkSystemSecurity();
    
    if (req.method == HTTP_METHOD_GET) {
      SECURE_LOG_INFO("HTTP", "GET request to %s", maskSensitiveData(String(req.url), 30).c_str());
      
      if (req.https) {
        WiFiClientSecure* sclient = createSecureClient(req.url);
        client.begin(*sclient, req.url);
      } else {
        client.begin(req.url);
      }
      
      resp.statusCode = client.GET();
      if (resp.statusCode > 0) {
        String payload = client.getString();
        resp.contentLength = min((size_t)payload.length(), sizeof(resp.payload) - 1);
        payload.toCharArray(resp.payload, resp.contentLength + 1);
        resp.payload[resp.contentLength] = '\0';
        ok = true;
        
        SECURE_LOG_INFO("HTTP", "GET response: %d (%d bytes)", resp.statusCode, resp.contentLength);
      } else {
        SECURE_LOG_ERROR("HTTP", "GET failed: %d", resp.statusCode);
        logSecurityEvent("HTTP_GET_FAILED", String("Status: " + String(resp.statusCode)).c_str());
      }
      
      client.end();
      
    } else if (req.method == HTTP_METHOD_POST) {
      SECURE_LOG_INFO("HTTP", "POST request to %s", maskSensitiveData(String(req.url), 30).c_str());
      
      if (req.https) {
        WiFiClientSecure* sclient = createSecureClient(req.url);
        client.begin(*sclient, req.url);
      } else {
        client.begin(req.url);
      }
      
      client.addHeader("Content-Type", req.contentType);
      client.addHeader("User-Agent", "DPM2-ESP32/1.0");
      
      // Validation du body
      size_t bodyLen = strlen(req.body);
      if (bodyLen > sizeof(req.body) - 1) {
        SECURE_LOG_ERROR("HTTP", "POST body too large: %zu bytes", bodyLen);
        client.end();
        continue;
      }
      
      resp.statusCode = client.POST((uint8_t*)req.body, bodyLen);
      if (resp.statusCode > 0) {
        String payload = client.getString();
        resp.contentLength = min((size_t)payload.length(), sizeof(resp.payload) - 1);
        payload.toCharArray(resp.payload, resp.contentLength + 1);
        resp.payload[resp.contentLength] = '\0';
        ok = true;
        
        SECURE_LOG_INFO("HTTP", "POST response: %d (%d bytes)", resp.statusCode, resp.contentLength);
      } else {
        SECURE_LOG_ERROR("HTTP", "POST failed: %d", resp.statusCode);
        logSecurityEvent("HTTP_POST_FAILED", String("Status: " + String(resp.statusCode)).c_str());
      }
      
      client.end();
    }
    if (req.responseQueue) {
      xQueueSend(req.responseQueue, &resp, 0);
    } else {
      Serial.printf("[HTTP] Status=%d Len=%d\n", resp.statusCode, resp.contentLength);
      if (resp.contentLength > 0) Serial.println(resp.payload);
    }
    (void)ok;
  }
}

void StartTaskHttpService() {
  if (!httpRequestQueue) httpRequestQueue = xQueueCreate(6, sizeof(HttpRequest));
  if (!httpTaskHandle) xTaskCreate(httpTask, "http_service", 6144, nullptr, 1, &httpTaskHandle);
}

bool HttpService_Enqueue(const HttpRequest* req) {
  if (!httpRequestQueue || !req) return false;
  return xQueueSend(httpRequestQueue, req, 0) == pdTRUE;
}

bool HttpService_Get(const char* url, QueueHandle_t responseQueue, uint32_t timeoutMs) {
  if (!url) return false;
  HttpRequest r{};
  r.method = HTTP_METHOD_GET;
  strncpy(r.url, url, sizeof(r.url) - 1);
  r.timeoutMs = timeoutMs;
  r.https = (strncmp(url, "https://", 8) == 0);
  r.responseQueue = responseQueue;
  return HttpService_Enqueue(&r);
}

bool HttpService_Post(const char* url, const char* contentType, const char* body, QueueHandle_t responseQueue, uint32_t timeoutMs) {
  if (!url || !contentType || !body) return false;
  HttpRequest r{};
  r.method = HTTP_METHOD_POST;
  strncpy(r.url, url, sizeof(r.url) - 1);
  strncpy(r.contentType, contentType, sizeof(r.contentType) - 1);
  strncpy(r.body, body, sizeof(r.body) - 1);
  r.timeoutMs = timeoutMs;
  r.https = (strncmp(url, "https://", 8) == 0);
  r.responseQueue = responseQueue;
  return HttpService_Enqueue(&r);
}

bool HttpService_ValidateQRToken(const char* qrToken, QueueHandle_t responseQueue, uint32_t timeoutMs) {
  if (!qrToken) return false;
  
  // Construction du JSON body
  char jsonBody[256];
  snprintf(jsonBody, sizeof(jsonBody), "{\"qr_code_token\":\"%s\"}", qrToken);
  
  // URL de l'endpoint de validation depuis la configuration
  String validationUrl = EnvConfig::GetValidateTokenUrl();
  
  Serial.printf("[HTTP] Validation QR token: %s\n", qrToken);
  Serial.printf("[HTTP] Using endpoint: %s\n", validationUrl.c_str());
  
  return HttpService_Post(validationUrl.c_str(), "application/json", jsonBody, responseQueue, timeoutMs);
}

bool HttpService_UpdateStock(const char* stockData, QueueHandle_t responseQueue, uint32_t timeoutMs) {
  if (!stockData) return false;
  
  // URL de l'endpoint de mise à jour du stock depuis la configuration
  String stockUrl = EnvConfig::GetStockUpdateUrl();
  
  Serial.printf("[HTTP] Updating stock with data: %s\n", stockData);
  Serial.printf("[HTTP] Using endpoint: %s\n", stockUrl.c_str());
  
  return HttpService_Post(stockUrl.c_str(), "application/json", stockData, responseQueue, timeoutMs);
}

bool HttpService_UpdateOrderStatus(const char* orderId, const char* newStatus, QueueHandle_t responseQueue, uint32_t timeoutMs) {
  if (!orderId || !newStatus) return false;
  
  // Construction du JSON body
  char jsonBody[256];
  snprintf(jsonBody, sizeof(jsonBody), "{\"order_id\":\"%s\",\"status\":\"%s\"}", orderId, newStatus);
  
  // URL de l'endpoint de mise à jour du statut depuis la configuration
  String statusUrl = EnvConfig::GetOrderStatusUrl();
  
  Serial.printf("[HTTP] Updating order %s status to: %s\n", orderId, newStatus);
  Serial.printf("[HTTP] Using endpoint: %s\n", statusUrl.c_str());
  
  return HttpService_Post(statusUrl.c_str(), "application/json", jsonBody, responseQueue, timeoutMs);
}

bool HttpService_ConfirmDelivery(const char* orderId, const char* machineId, const char* timestamp, const char* itemsDeliveredJson, QueueHandle_t responseQueue, uint32_t timeoutMs) {
  if (!orderId || !machineId || !timestamp || !itemsDeliveredJson) return false;
  
  // Construction du JSON body pour la confirmation de livraison
  char jsonBody[1024];
  snprintf(jsonBody, sizeof(jsonBody), 
    "{\"order_id\":\"%s\",\"machine_id\":\"%s\",\"timestamp\":\"%s\",\"items_delivered\":%s}", 
    orderId, machineId, timestamp, itemsDeliveredJson);
  
  // URL de l'endpoint de confirmation de livraison depuis la configuration
  String deliveryUrl = EnvConfig::GetDeliveryConfirmUrl();
  
  Serial.printf("[HTTP] Confirming delivery for order: %s\n", orderId);
  Serial.printf("[HTTP] Machine: %s, Timestamp: %s\n", machineId, timestamp);
  Serial.printf("[HTTP] Items delivered: %s\n", itemsDeliveredJson);
  Serial.printf("[HTTP] Using endpoint: %s\n", deliveryUrl.c_str());
  
  return HttpService_Post(deliveryUrl.c_str(), "application/json", jsonBody, responseQueue, timeoutMs);
}

bool HttpService_UpdateQuantities(const char* machineId, const char* productId, int quantity, int slotNumber, QueueHandle_t responseQueue, uint32_t timeoutMs) {
  if (!machineId || !productId) return false;
  
  // Construction du JSON body pour la mise à jour des quantités
  char jsonBody[512];
  snprintf(jsonBody, sizeof(jsonBody), 
    "{\"machine_id\":\"%s\",\"product_id\":\"%s\",\"quantity\":%d,\"slot_number\":%d}", 
    machineId, productId, quantity, slotNumber);
  
  // URL de l'endpoint de mise à jour des quantités depuis la configuration
  String quantitiesUrl = EnvConfig::GetUpdateQuantitiesUrl();
  
  Serial.printf("[HTTP] Updating quantities for product: %s\n", productId);
  Serial.printf("[HTTP] Machine: %s, Slot: %d, Quantity: %d\n", machineId, slotNumber, quantity);
  Serial.printf("[HTTP] Using endpoint: %s\n", quantitiesUrl.c_str());
  
  return HttpService_Post(quantitiesUrl.c_str(), "application/json", jsonBody, responseQueue, timeoutMs);
}


