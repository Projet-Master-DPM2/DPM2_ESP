#include "uart_parser.h"
#include <string.h>

static bool isPrintableOrAllowed(char ch) {
  return (ch == ':' || ch == '_' || ch == '-' || ch == ' ' || (ch >= 0x20 && ch <= 0x7E));
}

UartResult UartParser_HandleLine(const char* line, bool wifiReady) {
  if (!line) return UART_UNKNOWN;
  size_t len = strlen(line);
  if (len > 64) return UART_ERR_TOO_LONG;
  for (size_t i = 0; i < len; ++i) {
    if (!isPrintableOrAllowed(line[i])) return UART_ERR_BAD_CHAR;
  }
  if (strncmp(line, "STATE:", 6) == 0) {
    const char* state = line + 6;
    while (*state == ' ') state++;
    if (strcmp(state, "PAYING") == 0) {
      return wifiReady ? UART_ACK : UART_NAK;
    }
  }
  return UART_UNKNOWN;
}
