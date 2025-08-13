#include "orchestrator.h"
#include "config.h"
#include "services/nfc_service.h"
#include "services/uart_service.h"
#include "services/wifi_service.h"

static QueueHandle_t orchestratorQueueHandle = nullptr;
static TaskHandle_t orchestratorTaskHandle = nullptr;

static void orchestratorTask(void* pvParameters);

QueueHandle_t Orchestrator_GetQueue() {
  return orchestratorQueueHandle;
}

void StartTaskOrchestrator() {
  if (!orchestratorQueueHandle) {
    orchestratorQueueHandle = xQueueCreate(ORCHESTRATOR_QUEUE_LENGTH, sizeof(OrchestratorEvent));
  }

  if (!orchestratorTaskHandle) {
    xTaskCreate(
      orchestratorTask,
      "orchestrator",
      ORCHESTRATOR_TASK_STACK_SIZE,
      nullptr,
      ORCHESTRATOR_TASK_PRIORITY,
      &orchestratorTaskHandle
    );
  }
}

static void orchestratorTask(void* pvParameters) {
  OrchestratorEvent evt{};
  for (;;) {
    if (xQueueReceive(orchestratorQueueHandle, &evt, portMAX_DELAY) == pdTRUE) {
      switch (evt.type) {
        case ORCH_EVT_NFC_UID_READ:
          Serial.print("[ORCH] NFC UID: ");
          Serial.println(evt.payload);
          UartService_SendLine((String("NFC_UID:") + evt.payload).c_str());
          // TODO: envoyer au backend (WS/HTTP), puis transmettre à la NUCLEO via UART si autorisé
          break;
        case ORCH_EVT_NFC_DATA:
          Serial.print("[ORCH] NFC TEXT: ");
          Serial.println(evt.payload);
          UartService_SendLine((String("NFC_TEXT:") + evt.payload).c_str());
          break;
        case ORCH_EVT_NFC_ERROR:
          Serial.print("[ORCH] NFC Error: ");
          Serial.println(evt.payload);
          UartService_SendLine((String("NFC_ERR:") + evt.payload).c_str());
          break;
        case ORCH_EVT_STATE_PAYING:
          if (!WifiService_IsReady()) {
            Serial.println("[ORCH] PAYING ignored: no network");
            break;
          }
          Serial.println("[ORCH] State=PAYING -> Trigger NFC scan");
          if (!NfcService_TriggerScan()) {
            Serial.println("[ORCH] NFC busy");
          }
          break;
        default:
          Serial.println("[ORCH] Événement inconnu");
          break;
      }
    }
  }
}


