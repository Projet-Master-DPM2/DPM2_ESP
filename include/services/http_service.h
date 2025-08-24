#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
  HTTP_METHOD_GET = 1,
  HTTP_METHOD_POST = 2,
} HttpMethod;

typedef struct {
  HttpMethod method;
  char url[256];
  char contentType[64];
  uint32_t timeoutMs;
  bool https;
  // Corps pour POST (facultatif)
  char body[768];
  // File de réponse optionnelle (si NULL, la réponse est juste loggée)
  QueueHandle_t responseQueue;
} HttpRequest;

typedef struct {
  int statusCode;
  int contentLength;
  char payload[1024];
} HttpResponse;

void StartTaskHttpService();

// Envoi non bloquant: pousse la requête dans la file du service
bool HttpService_Enqueue(const HttpRequest* req);

// Helpers simples
bool HttpService_Get(const char* url, QueueHandle_t responseQueue, uint32_t timeoutMs);
bool HttpService_Post(const char* url, const char* contentType, const char* body, QueueHandle_t responseQueue, uint32_t timeoutMs);

// Validation de token QR
bool HttpService_ValidateQRToken(const char* qrToken, QueueHandle_t responseQueue, uint32_t timeoutMs);

// Mise à jour du stock après livraison
bool HttpService_UpdateStock(const char* stockData, QueueHandle_t responseQueue, uint32_t timeoutMs);

// Mise à jour du statut de commande
bool HttpService_UpdateOrderStatus(const char* orderId, const char* newStatus, QueueHandle_t responseQueue, uint32_t timeoutMs);


