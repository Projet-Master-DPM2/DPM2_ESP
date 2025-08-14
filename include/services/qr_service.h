#pragma once

#include <Arduino.h>

// Démarre la tâche de lecture du scanner QR (UART2 sur D16/D17)
void StartTaskQrService();

// Arrête la tâche QR si en cours d'exécution
void StopTaskQrService();

// Indique si la tâche QR tourne
bool QrService_IsRunning();

// Active/Désactive le mode hex dump (octets bruts)
void QrService_SetHexDump(bool enable);
bool QrService_GetHexDump();


