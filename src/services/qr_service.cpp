#include "services/qr_service.h"
#include "config.h"

static TaskHandle_t qrTaskHandle = nullptr;
static volatile bool hexDumpEnabled = false;

static void qrTask(void* pvParameters) {
  String line;
  line.reserve(256);
  unsigned long lastByteMs = 0;
  const unsigned long interCharFlushMs = 40; // si pas de fin de ligne, flush après un court silence

  for (;;) {
    while (Serial2.available() > 0) {
      uint8_t b = (uint8_t)Serial2.read();
      lastByteMs = millis();
      if (hexDumpEnabled) {
        Serial.printf("%02X ", b);
        // Fin de ligne: si pause, on passe à la ligne
        continue;
      }
      char c = (char)b;
      if (c == '\n' || c == '\r') {
        if (line.length() > 0) {
          Serial.print("[QR] ");
          Serial.println(line);
          line = "";
        }
      } else {
        line += c;
        if (line.length() > 250) {
          // éviter dépassement
          Serial.print("[QR] (tronc) ");
          Serial.println(line);
          line = "";
        }
      }
    }
    // Flush par timeout inter-caractère si pas de fin de ligne
    if (!hexDumpEnabled && line.length() > 0 && (millis() - lastByteMs) > interCharFlushMs) {
      Serial.print("[QR] ");
      Serial.println(line);
      line = "";
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void StartTaskQrService() {
  // Initialiser l'UART2 pour le scanner (D16/D17)
  const uint32_t fixedBaud = 115200;
  Serial2.begin(fixedBaud, SERIAL_8N1, QR_UART_RX_PIN, QR_UART_TX_PIN);
  Serial.printf("[QR] UART2 start @ %lu bps (RX=%d, TX=%d)\n", (unsigned long)fixedBaud, QR_UART_RX_PIN, QR_UART_TX_PIN);
  if (!qrTaskHandle) {
    xTaskCreate(qrTask, "qr_service", 4096, nullptr, 1, &qrTaskHandle);
  }
}

void StopTaskQrService() {
  if (qrTaskHandle) {
    vTaskDelete(qrTaskHandle);
    qrTaskHandle = nullptr;
    Serial.println("[QR] service stopped");
  }
}

bool QrService_IsRunning() {
  return qrTaskHandle != nullptr;
}

void QrService_SetHexDump(bool enable) { hexDumpEnabled = enable; }
bool QrService_GetHexDump() { return hexDumpEnabled; }


