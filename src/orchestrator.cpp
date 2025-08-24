#include "orchestrator.h"
#include "config.h"
#include "services/nfc_service.h"
#include "services/uart_service.h"
#include "services/wifi_service.h"
#include "services/http_service.h"
#include "order_manager.h"

static QueueHandle_t orchestratorQueueHandle = nullptr;
static TaskHandle_t orchestratorTaskHandle = nullptr;
static QueueHandle_t httpResponseQueue = nullptr;

// États du workflow de commande
enum OrderWorkflowState {
  WORKFLOW_IDLE = 0,
  WORKFLOW_VALIDATING_TOKEN = 1,
  WORKFLOW_DELIVERING = 2,
  WORKFLOW_UPDATING_STOCK = 3,
  WORKFLOW_UPDATING_STATUS = 4,
  WORKFLOW_COMPLETED = 5
};

static OrderWorkflowState currentWorkflowState = WORKFLOW_IDLE;

static void orchestratorTask(void* pvParameters);

QueueHandle_t Orchestrator_GetQueue() {
  return orchestratorQueueHandle;
}

void StartTaskOrchestrator() {
  if (!orchestratorQueueHandle) {
    orchestratorQueueHandle = xQueueCreate(ORCHESTRATOR_QUEUE_LENGTH, sizeof(OrchestratorEvent));
  }
  
  if (!httpResponseQueue) {
    httpResponseQueue = xQueueCreate(5, sizeof(HttpResponse));
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
  HttpResponse httpResp{};
  
  for (;;) {
    // Vérifier les réponses HTTP en attente (non bloquant)
    if (xQueueReceive(httpResponseQueue, &httpResp, 0) == pdTRUE) {
      Serial.printf("[ORCH] HTTP Response: Status=%d, Content=%s\n", httpResp.statusCode, httpResp.payload);
      
      switch (currentWorkflowState) {
        case WORKFLOW_VALIDATING_TOKEN:
          if (httpResp.statusCode == 200) {
            // Parser la réponse JSON et stocker les données de commande
            OrderData order;
            if (OrderManager::ParseOrderFromJSON(httpResp.payload, &order)) {
              OrderManager::SetCurrentOrder(&order);
              
              // Générer et envoyer les commandes de livraison à NUCLEO
              String deliveryCommands = OrderManager::GenerateDeliveryCommands();
              if (deliveryCommands.length() > 0) {
                UartService_SendLine(("ORDER_START:" + deliveryCommands).c_str());
                currentWorkflowState = WORKFLOW_DELIVERING;
                Serial.println("[ORCH] Order validated, delivery commands sent to NUCLEO");
              } else {
                Serial.println("[ORCH] Error: Could not generate delivery commands");
                UartService_SendLine("QR_TOKEN_ERROR");
                currentWorkflowState = WORKFLOW_IDLE;
                OrderManager::ClearCurrentOrder();
              }
            } else {
              Serial.println("[ORCH] Error: Could not parse order data from response");
              UartService_SendLine("QR_TOKEN_INVALID");
              currentWorkflowState = WORKFLOW_IDLE;
            }
          } else {
            Serial.printf("[ORCH] QR Token invalide ou erreur: %d\n", httpResp.statusCode);
            UartService_SendLine("QR_TOKEN_INVALID");
            currentWorkflowState = WORKFLOW_IDLE;
          }
          break;
          
        case WORKFLOW_UPDATING_STOCK:
          if (httpResp.statusCode == 200) {
            Serial.println("[ORCH] Stock updated successfully, updating order status");
            OrderData* order = OrderManager::GetCurrentOrder();
            if (order) {
              HttpService_UpdateOrderStatus(order->order_id, "DELIVERED", httpResponseQueue, 10000);
              currentWorkflowState = WORKFLOW_UPDATING_STATUS;
            } else {
              Serial.println("[ORCH] Error: No active order for status update");
              currentWorkflowState = WORKFLOW_IDLE;
            }
          } else {
            Serial.printf("[ORCH] Stock update failed: %d\n", httpResp.statusCode);
            currentWorkflowState = WORKFLOW_IDLE;
            OrderManager::ClearCurrentOrder();
          }
          break;
          
        case WORKFLOW_UPDATING_STATUS:
          if (httpResp.statusCode == 200) {
            Serial.println("[ORCH] Order status updated to DELIVERED - Workflow completed!");
            currentWorkflowState = WORKFLOW_COMPLETED;
          } else {
            Serial.printf("[ORCH] Order status update failed: %d\n", httpResp.statusCode);
          }
          // Dans tous les cas, nettoyer la commande courante
          OrderManager::ClearCurrentOrder();
          currentWorkflowState = WORKFLOW_IDLE;
          break;
          
        default:
          Serial.printf("[ORCH] Unexpected HTTP response in state %d\n", currentWorkflowState);
          break;
      }
    }
    
    if (xQueueReceive(orchestratorQueueHandle, &evt, pdMS_TO_TICKS(100)) == pdTRUE) {
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
        case ORCH_EVT_QR_TOKEN_READ:
          Serial.printf("[ORCH] QR Token reçu: %s\n", evt.payload);
          if (currentWorkflowState != WORKFLOW_IDLE) {
            Serial.printf("[ORCH] QR Token ignoré: workflow en cours (état %d)\n", currentWorkflowState);
            UartService_SendLine("QR_TOKEN_BUSY");
            break;
          }
          if (!WifiService_IsReady()) {
            Serial.println("[ORCH] QR Token ignoré: pas de réseau");
            UartService_SendLine("QR_TOKEN_NO_NETWORK");
            break;
          }
          Serial.println("[ORCH] Validation du QR Token...");
          if (HttpService_ValidateQRToken(evt.payload, httpResponseQueue, 10000)) {
            currentWorkflowState = WORKFLOW_VALIDATING_TOKEN;
          } else {
            Serial.println("[ORCH] Erreur envoi requête validation QR");
            UartService_SendLine("QR_TOKEN_ERROR");
          }
          break;
          
        case ORCH_EVT_DELIVERY_COMPLETED:
          Serial.printf("[ORCH] Delivery completed: %s\n", evt.payload);
          if (currentWorkflowState == WORKFLOW_DELIVERING) {
            // Générer et envoyer la mise à jour du stock
            String stockData = OrderManager::GenerateStockUpdateData();
            if (stockData.length() > 0) {
              HttpService_UpdateStock(stockData.c_str(), httpResponseQueue, 10000);
              currentWorkflowState = WORKFLOW_UPDATING_STOCK;
              Serial.println("[ORCH] Stock update request sent");
            } else {
              Serial.println("[ORCH] Error: Could not generate stock update data");
              currentWorkflowState = WORKFLOW_IDLE;
              OrderManager::ClearCurrentOrder();
            }
          } else {
            Serial.printf("[ORCH] Unexpected delivery completion in state %d\n", currentWorkflowState);
          }
          break;
          
        case ORCH_EVT_DELIVERY_FAILED:
          Serial.printf("[ORCH] Delivery failed: %s\n", evt.payload);
          if (currentWorkflowState == WORKFLOW_DELIVERING) {
            Serial.println("[ORCH] Cleaning up failed order");
            currentWorkflowState = WORKFLOW_IDLE;
            OrderManager::ClearCurrentOrder();
            UartService_SendLine("ORDER_FAILED");
          }
          break;
        default:
          Serial.println("[ORCH] Événement inconnu");
          break;
      }
    }
  }
}


