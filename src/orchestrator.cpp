#include "orchestrator.h"
#include "config.h"
#include "services/nfc_service.h"
#include "services/uart_service.h"
#include "services/wifi_service.h"
#include "services/http_service.h"
#include "order_manager.h"
#include "supervision_service.h"

static QueueHandle_t orchestratorQueueHandle = nullptr;
static TaskHandle_t orchestratorTaskHandle = nullptr;
static QueueHandle_t httpResponseQueue = nullptr;

// États du workflow de commande
enum OrderWorkflowState {
  WORKFLOW_IDLE = 0,
  WORKFLOW_VALIDATING_TOKEN = 1,
  WORKFLOW_DELIVERING = 2,
  WORKFLOW_UPDATING_QUANTITIES = 3,
  WORKFLOW_CONFIRMING_DELIVERY = 4,
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

  // Initialiser le service de supervision
  SupervisionService::Initialize();

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
                SupervisionService::SendErrorNotification(
                  SUPERVISION_ERROR_CRITICAL_SERVICE_FAILURE,
                  "Failed to generate delivery commands for validated order"
                );
                currentWorkflowState = WORKFLOW_IDLE;
                OrderManager::ClearCurrentOrder();
              }
            } else {
              Serial.println("[ORCH] Error: Could not parse order data from response");
              UartService_SendLine("QR_TOKEN_INVALID");
              SupervisionService::SendErrorNotification(
                SUPERVISION_ERROR_CRITICAL_SERVICE_FAILURE,
                "Failed to parse order data from QR token validation response"
              );
              currentWorkflowState = WORKFLOW_IDLE;
            }
          } else {
            Serial.printf("[ORCH] QR Token invalide ou erreur: %d\n", httpResp.statusCode);
            UartService_SendLine("QR_TOKEN_INVALID");
            currentWorkflowState = WORKFLOW_IDLE;
          }
          break;
          
        case WORKFLOW_UPDATING_QUANTITIES:
          if (httpResp.statusCode == 200) {
            Serial.println("[ORCH] Quantities updated successfully, confirming delivery");
            // Confirmer la livraison au backend
            OrderData* order = OrderManager::GetCurrentOrder();
            if (order) {
              String deliveryData = OrderManager::GenerateDeliveryConfirmationData();
              if (deliveryData.length() > 0) {
                HttpService_ConfirmDelivery(order->order_id, order->machine_id, order->timestamp, deliveryData.c_str(), httpResponseQueue, 10000);
                currentWorkflowState = WORKFLOW_CONFIRMING_DELIVERY;
                Serial.println("[ORCH] Delivery confirmation request sent");
              } else {
                Serial.println("[ORCH] Error: Could not generate delivery confirmation data");
                currentWorkflowState = WORKFLOW_IDLE;
                OrderManager::ClearCurrentOrder();
              }
            } else {
              Serial.println("[ORCH] Error: No active order for delivery confirmation");
              currentWorkflowState = WORKFLOW_IDLE;
            }
          } else {
            Serial.printf("[ORCH] Quantity update failed: %d\n", httpResp.statusCode);
            currentWorkflowState = WORKFLOW_IDLE;
            OrderManager::ClearCurrentOrder();
          }
          break;
          
        case WORKFLOW_CONFIRMING_DELIVERY:
          if (httpResp.statusCode == 200) {
            Serial.println("[ORCH] Delivery confirmed successfully - Workflow completed!");
            // Le backend gère automatiquement la mise à jour du stock et du statut
            currentWorkflowState = WORKFLOW_COMPLETED;
          } else {
            Serial.printf("[ORCH] Delivery confirmation failed: %d\n", httpResp.statusCode);
            currentWorkflowState = WORKFLOW_IDLE;
            OrderManager::ClearCurrentOrder();
          }
          break;
          
        case WORKFLOW_COMPLETED:
          Serial.println("[ORCH] Workflow completed, cleaning up and returning to idle");
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
            // Mettre à jour les quantités de stock
            if (OrderManager::UpdateAllQuantities(httpResponseQueue, 10000)) {
              currentWorkflowState = WORKFLOW_UPDATING_QUANTITIES;
              Serial.println("[ORCH] Quantity update request sent");
            } else {
              Serial.println("[ORCH] Error: Could not send quantity update request");
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
            SupervisionService::SendErrorNotification(
              SUPERVISION_ERROR_CRITICAL_SERVICE_FAILURE,
              "Physical delivery failed - NUCLEO reported delivery failure: " + String(evt.payload)
            );
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


