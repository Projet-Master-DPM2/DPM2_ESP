#include <Arduino.h>
#include "services/uart_service.h"
#include "config.h"
#include "orchestrator.h"
#include "security_config.h"
// Utiliser un port UART dédié (Serial1) pour la liaison NUCLEO afin de laisser Serial2 au scanner QR
#include <HardwareSerial.h>
#include "services/wifi_service.h"
#include "uart_parser.h"

static TaskHandle_t uartTaskHandle = nullptr;
static QueueHandle_t orchestratorQueueHandle = nullptr;
static HardwareSerial SerialNucleo(1);

static void uartTask(void* pvParameters);

void StartTaskUartService(QueueHandle_t orchestratorQueue) {
  orchestratorQueueHandle = orchestratorQueue;

  // Config pins si nécessaire (selon board) + begin
  SerialNucleo.begin(UART_BAUDRATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  SECURE_LOG_INFO("UART1", "Started @ %lu bps (RX=%d, TX=%d)", (unsigned long)UART_BAUDRATE, UART_RX_PIN, UART_TX_PIN);
#if UART0_FALLBACK_ENABLED
  SECURE_LOG_INFO("UART0", "USB monitor active @ 115200 (RX0/TX0)");
#endif

  if (!uartTaskHandle) {
    xTaskCreate(
      uartTask,
      "uart_service",
      4096,
      nullptr,
      1,
      &uartTaskHandle
    );
  }
}

void UartService_SendLine(const char* line) {
  if (!line) return;
  SerialNucleo.println(line);
}

static void publishEvent(OrchestratorEventType type, const char* payload) {
  if (!orchestratorQueueHandle) return;
  OrchestratorEvent evt{};
  evt.type = type;
  if (payload) {
    strncpy(evt.payload, payload, sizeof(evt.payload) - 1);
    evt.payload[sizeof(evt.payload) - 1] = '\0';
  } else {
    evt.payload[0] = '\0';
  }
  xQueueSend(orchestratorQueueHandle, &evt, 0);
}

static void handleIncomingLine(const String& line) {
  // Rate limiting UART
  if (!rateLimitCheck("UART", UART_COMMAND_COOLDOWN_MS)) {
    SECURE_LOG_ERROR("UART", "Command rate limited");
    UartService_SendLine("ERR:RATE_LIMIT");
    return;
  }
  
  // Validation de la ligne
  if (line.length() > MAX_UART_LINE_LENGTH) {
    SECURE_LOG_ERROR("UART", "Line too long: %d chars", line.length());
    UartService_SendLine("ERR:LINE_TOO_LONG");
    return;
  }
  
  // Log sécurisé (masquer données sensibles)
  String maskedLine = line;
  if (line.startsWith("STATE:PAYING") || line.startsWith("NFC_")) {
    maskedLine = maskSensitiveData(line, 12);
  }
  SECURE_LOG_INFO("UART", "Processing: %s", maskedLine.c_str());
  
  // Traitement spécial pour les réponses de livraison
  if (line.startsWith("DELIVERY_COMPLETED")) {
    SECURE_LOG_INFO("UART", "Delivery completed successfully");
    publishEvent(ORCH_EVT_DELIVERY_COMPLETED, line.c_str());
    return;
  }
  
  if (line.startsWith("DELIVERY_FAILED")) {
    SECURE_LOG_ERROR("UART", "Delivery failed: %s", line.c_str());
    publishEvent(ORCH_EVT_DELIVERY_FAILED, line.c_str());
    return;
  }
  
  // Traitement des confirmations de commande
  if (line.startsWith("ORDER_ACK")) {
    SECURE_LOG_INFO("UART", "Order acknowledged by NUCLEO");
    return;
  }
  
  if (line.startsWith("ORDER_NAK")) {
    SECURE_LOG_ERROR("UART", "Order rejected by NUCLEO: %s", line.c_str());
    publishEvent(ORCH_EVT_DELIVERY_FAILED, line.c_str());
    return;
  }
  
  // Traitement des statuts de livraison par item
  if (line.startsWith("VEND_COMPLETED")) {
    SECURE_LOG_INFO("UART", "Vending completed for item: %s", line.c_str());
    return;
  }
  
  if (line.startsWith("VEND_FAILED")) {
    SECURE_LOG_ERROR("UART", "Vending failed for item: %s", line.c_str());
    return;
  }
  
  UartResult r = UartParser_HandleLine(line.c_str(), WifiService_IsReady());
  switch (r) {
    case UART_ACK:
      publishEvent(ORCH_EVT_STATE_PAYING, nullptr);
      UartService_SendLine("ACK:STATE:PAYING");
      SECURE_LOG_INFO("UART", "ACK sent for payment state");
      break;
    case UART_NAK:
      UartService_SendLine("NAK:STATE:PAYING:NO_NET");
      SECURE_LOG_INFO("UART", "NAK sent - no network");
      break;
    case UART_ERR_TOO_LONG:
      UartService_SendLine("ERR:LINE_TOO_LONG");
      SECURE_LOG_ERROR("UART", "Line length error");
      break;
    case UART_ERR_BAD_CHAR:
      UartService_SendLine("ERR:BAD_CHAR");
      SECURE_LOG_ERROR("UART", "Invalid character detected");
      break;
    case UART_UNKNOWN:
    default:
      UartService_SendLine("ERR:UNKNOWN_CMD");
      SECURE_LOG_ERROR("UART", "Unknown command");
      break;
  }
}

static void uartTask(void* pvParameters) {
  String buffer;
  buffer.reserve(128);

  for (;;) {
    while (SerialNucleo.available() > 0) {
      char c = (char)SerialNucleo.read();
      if (c == '\n' || c == '\r') {
        if (buffer.length() > 0) {
          // Log sécurisé sans révéler les données sensibles
          String maskedBuffer = maskSensitiveData(buffer, 10);
          SECURE_LOG_INFO("UART1", "Received: %s", maskedBuffer.c_str());
          handleIncomingLine(buffer);
          buffer = "";
        }
      } else {
        buffer += c;
        if (buffer.length() > 120) {
          buffer = ""; // éviter débordement
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}


