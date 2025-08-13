#pragma once

// Brochage RC522 (VSPI) sur ESP32-WROOM-32D
#define RC522_SCK   18
#define RC522_MISO  19
#define RC522_MOSI  23
#define RC522_SS    5
#define RC522_RST   22

// Anti-rebond NFC (en ms)
#define NFC_DEBOUNCE_MS 1000

// Fenêtre de scan NFC lorsqu'une notification est reçue (en ms)
#define NFC_SCAN_TIMEOUT_MS 15000

// Configuration tâches FreeRTOS
#define NFC_TASK_STACK_SIZE         4096
#define NFC_TASK_PRIORITY           1
#define ORCHESTRATOR_TASK_STACK_SIZE 4096
#define ORCHESTRATOR_TASK_PRIORITY   1

// File d'événements orchestrateur
#define ORCHESTRATOR_QUEUE_LENGTH   10

// UART vers NUCLEO (adapter si besoin)
#define UART_BAUDRATE 115200
#define UART_TX_PIN   25
#define UART_RX_PIN   26

// Activer un fallback de lecture sur UART0 (Serial) si le câblage utilise RX0/TX0
#define UART0_FALLBACK_ENABLED 1

// UART pour scanner QR (MP...): câblé sur D16/D17 -> utiliser UART2
#define QR_UART_BAUDRATE 115200
#define QR_UART_TX_PIN   17
#define QR_UART_RX_PIN   16


