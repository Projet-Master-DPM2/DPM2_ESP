#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Démarre la tâche du service NFC (initialisation RC522).
void StartTaskNfcService(QueueHandle_t orchestratorQueue);

// Retourne le handle de la tâche NFC
TaskHandle_t NfcService_GetTaskHandle();

// Déclenche un scan NFC (notification directe de tâche).
// Retourne true si la demande a été prise en compte, false si déjà occupé.
bool NfcService_TriggerScan();

// Infos debug (version RC522, busy, timeout)
void NfcService_DebugInfo();

// Arrête la tâche NFC si en cours d'exécution
void StopTaskNfcService();

// Indique si la tâche NFC tourne
bool NfcService_IsRunning();


