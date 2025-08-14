#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

// Types d'événements (simplifié pour les tests)
typedef enum {
    ORCH_EVT_UNKNOWN = 0,
    ORCH_EVT_NFC_UID_READ,
    ORCH_EVT_NFC_DATA,
    ORCH_EVT_NFC_ERROR,
    ORCH_EVT_STATE_PAYING
} OrchestratorEventType_C;

typedef enum {
    ORCH_RESULT_IGNORED,
    ORCH_RESULT_NFC_TRIGGERED,
    ORCH_RESULT_PAYMENT_PROCESSED,
    ORCH_RESULT_ERROR_NO_WIFI
} OrchestratorResult;

// Logique pure de traitement des événements
OrchestratorResult Orchestrator_ProcessEvent_C(OrchestratorEventType_C eventType, 
                                                const char* payload, 
                                                bool wifiReady);

// Validation des payloads
bool Orchestrator_IsValidNfcUid_C(const char* uid);
bool Orchestrator_IsValidNfcData_C(const char* data);

#ifdef __cplusplus
}
#endif
