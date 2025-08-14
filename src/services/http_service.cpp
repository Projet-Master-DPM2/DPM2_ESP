#include "services/http_service.h"
#include "services/wifi_service.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static TaskHandle_t httpTaskHandle = nullptr;
static QueueHandle_t httpRequestQueue = nullptr;

static void httpTask(void* pv) {
  HTTPClient client;
  HttpRequest req;
  for (;;) {
    if (xQueueReceive(httpRequestQueue, &req, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    if (!WifiService_IsReady()) {
      Serial.println("[HTTP] Ignored: WiFi not ready");
      continue;
    }
    HttpResponse resp = {};
    resp.statusCode = 0;
    resp.contentLength = 0;
    resp.payload[0] = '\0';
    bool ok = false;
    client.setTimeout(req.timeoutMs > 0 ? req.timeoutMs : 8000);
    if (req.method == HTTP_METHOD_GET) {
      Serial.printf("[HTTP] GET %s\n", req.url);
      if (req.https) {
        WiFiClientSecure sclient;
        sclient.setInsecure();
        client.begin(sclient, req.url);
      } else {
        client.begin(req.url);
      }
      resp.statusCode = client.GET();
      if (resp.statusCode > 0) {
        String payload = client.getString();
        resp.contentLength = payload.length();
        payload.toCharArray(resp.payload, sizeof(resp.payload));
        ok = true;
      }
      client.end();
    } else if (req.method == HTTP_METHOD_POST) {
      Serial.printf("[HTTP] POST %s\n", req.url);
      if (req.https) {
        WiFiClientSecure sclient;
        sclient.setInsecure();
        client.begin(sclient, req.url);
      } else {
        client.begin(req.url);
      }
      client.addHeader("Content-Type", req.contentType);
      resp.statusCode = client.POST((uint8_t*)req.body, strlen(req.body));
      if (resp.statusCode > 0) {
        String payload = client.getString();
        resp.contentLength = payload.length();
        payload.toCharArray(resp.payload, sizeof(resp.payload));
        ok = true;
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


