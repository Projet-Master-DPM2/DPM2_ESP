#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void StartTaskUartService(QueueHandle_t orchestratorQueue);

// API d'envoi vers NUCLEO
void UartService_SendLine(const char* line);


