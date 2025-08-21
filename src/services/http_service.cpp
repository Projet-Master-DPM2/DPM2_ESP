#include "services/http_service.h"
#include "services/wifi_service.h"
#include "security_config.h"
#include "../security_utils.cpp"
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


