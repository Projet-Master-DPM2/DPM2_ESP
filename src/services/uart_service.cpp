#include "services/uart_service.h"
#include "config.h"
#include "orchestrator.h"
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
  Serial.printf("[UART1] start @ %lu bps (RX=%d, TX=%d)\n", (unsigned long)UART_BAUDRATE, UART_RX_PIN, UART_TX_PIN);
#if UART0_FALLBACK_ENABLED
  Serial.printf("[UART0] USB monitor active @ 115200 (RX0/TX0). Si vous voyez [UART1<-], la liaison est OK.\n");
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
  UartResult r = UartParser_HandleLine(line.c_str(), WifiService_IsReady());
  switch (r) {
    case UART_ACK:
      publishEvent(ORCH_EVT_STATE_PAYING, nullptr);
      UartService_SendLine("ACK:STATE:PAYING");
      break;
    case UART_NAK:
      UartService_SendLine("NAK:STATE:PAYING:NO_NET");
      break;
    case UART_ERR_TOO_LONG:
      UartService_SendLine("ERR:LINE_TOO_LONG");
      break;
    case UART_ERR_BAD_CHAR:
      UartService_SendLine("ERR:BAD_CHAR");
      break;
    case UART_UNKNOWN:
    default:
      UartService_SendLine("ERR:UNKNOWN_CMD");
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
          Serial.printf("[UART1<-] %s\n", buffer.c_str());
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


