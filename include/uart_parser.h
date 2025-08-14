#pragma once

#include <stddef.h>

enum UartResult {
  UART_ACK,
  UART_NAK,
  UART_ERR_TOO_LONG,
  UART_ERR_BAD_CHAR,
  UART_UNKNOWN
};

// wifiReady: passer l'état simulé en test natif; en build ESP, wrappera WifiService_IsReady()
// line: buffer C, nul-terminé
UartResult UartParser_HandleLine(const char* line, bool wifiReady);


