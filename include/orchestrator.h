#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

enum OrchestratorEventType {
  ORCH_EVT_NFC_UID_READ = 1,
  ORCH_EVT_NFC_ERROR = 2,
  ORCH_EVT_STATE_PAYING = 3,
  ORCH_EVT_NFC_DATA = 4,
};

struct OrchestratorEvent {
  OrchestratorEventType type;
  char payload[128];
};

QueueHandle_t Orchestrator_GetQueue();
void StartTaskOrchestrator();


