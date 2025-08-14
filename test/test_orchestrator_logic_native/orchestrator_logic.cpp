#include "../../include/orchestrator_logic.h"
#include <string.h>

OrchestratorResult Orchestrator_ProcessEvent_C(OrchestratorEventType_C eventType, 
                                                const char* payload, 
                                                bool wifiReady) {
    switch (eventType) {
        case ORCH_EVT_STATE_PAYING:
            if (!wifiReady) {
                return ORCH_RESULT_ERROR_NO_WIFI;
            }
            return ORCH_RESULT_NFC_TRIGGERED;
            
        case ORCH_EVT_NFC_UID_READ:
            if (payload && Orchestrator_IsValidNfcUid_C(payload)) {
                return ORCH_RESULT_PAYMENT_PROCESSED;
            }
            return ORCH_RESULT_IGNORED;
            
        case ORCH_EVT_NFC_DATA:
            if (payload && Orchestrator_IsValidNfcData_C(payload)) {
                return ORCH_RESULT_PAYMENT_PROCESSED;
            }
            return ORCH_RESULT_IGNORED;
            
        case ORCH_EVT_NFC_ERROR:
            return ORCH_RESULT_IGNORED;
            
        default:
            return ORCH_RESULT_IGNORED;
    }
}

bool Orchestrator_IsValidNfcUid_C(const char* uid) {
    if (!uid) return false;
    size_t len = strlen(uid);
    if (len < 8 || len > 20) return false; // UID entre 4-10 bytes = 8-20 chars hex
    
    // Vérifier que tous les caractères sont hexadécimaux
    for (size_t i = 0; i < len; i++) {
        char c = uid[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }
    return true;
}

bool Orchestrator_IsValidNfcData_C(const char* data) {
    if (!data) return false;
    size_t len = strlen(data);
    if (len < 3 || len > 100) return false; // Données entre 3-100 caractères
    
    // Vérifier que les données contiennent des caractères imprimables
    int printableCount = 0;
    for (size_t i = 0; i < len; i++) {
        char c = data[i];
        if (c >= 0x20 && c <= 0x7E) {
            printableCount++;
        }
    }
    
    // Au moins 80% de caractères imprimables
    return (printableCount * 100 / (int)len) >= 80;
}
