#include <Arduino.h>
#include "config.h"
#include "orchestrator.h"
#include "services/nfc_service.h"
#include "services/uart_service.h"
#include "services/qr_service.h"
#include "services/wifi_service.h"

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  delay(200);
  Serial.println("\n[DPM2] ESP32 boot");
  Serial.printf("[BOARD] Free heap: %lu bytes\n", (unsigned long)ESP.getFreeHeap());

  StartTaskWifiService();
  StartTaskOrchestrator();
  StartTaskUartService(Orchestrator_GetQueue()); // UART1 actif aussi hors réseau pour ACK/NAK

  // Lancer NFC/QR apres que le Wi-Fi soit pret (tache differée)
  xTaskCreate([](void* arg){
    // Attendre que le Wi-Fi soit pret
    while (!WifiService_IsReady()) vTaskDelay(pdMS_TO_TICKS(200));
    StartTaskNfcService(Orchestrator_GetQueue());
    StartTaskQrService();
    vTaskDelete(nullptr);
  }, "defer_services", 4096, nullptr, 1, nullptr);
}

void loop() {
  static String cliBuffer;

  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      String cmd = cliBuffer;
      cmd.trim();
      cliBuffer = "";

      if (cmd.length() == 0) {
        continue;
      }

      cmd.toUpperCase();
      // Tokenisation simple: commande + argument(s)
      String token = cmd;
      String args;
      int sp = cmd.indexOf(' ');
      if (sp >= 0) {
        token = cmd.substring(0, sp);
        args = cmd.substring(sp + 1);
        args.trim();
      }

      if (token == "HELP") {
        Serial.println("CMD: SCAN -> demarrer une fenetre NFC");
        Serial.println("CMD: HELP -> aide");
        Serial.println("CMD: HEX ON|OFF -> activer/desactiver hexdump QR");
        Serial.println("CMD: TX2 <texte> -> envoyer sur UART2");
        Serial.println("CMD: TX2HEX <octets hex> -> envoyer binaire sur UART2");
        Serial.println("CMD: TX1 <texte> -> envoyer sur UART1 (vers NUCLEO)");
        Serial.println("CMD: INFO -> afficher etat UART1/NFC");
        Serial.println("CMD: WIFI? -> etat Wi-Fi");
        Serial.println("CMD: WIFI OFF -> deconnecter et relancer le portail SoftAP");
      } else if (token == "SCAN") {
        bool ok = NfcService_TriggerScan();
        Serial.println(ok ? "[CLI] NFC scan demarre" : "[CLI] NFC deja en cours");
      } else if (token == "TX1") {
        String msg = args;
        UartService_SendLine(msg.c_str());
        Serial.println("[CLI] TX1 sent");
      } else if (token == "INFO") {
        Serial.printf("[INFO] UART1 RX=%d, TX=%d, BAUD=%lu\n", UART_RX_PIN, UART_TX_PIN, (unsigned long)UART_BAUDRATE);
        NfcService_DebugInfo();
      } else if (token == "WIFI?") {
        WifiService_DebugStatus();
      } else if (token == "WIFI") {
        String arg = args;
        arg.trim();
          if (arg == "OFF") {
            // Stopper NFC/QR avant de couper le Wi‑Fi
            if (NfcService_IsRunning()) StopTaskNfcService();
            if (QrService_IsRunning()) StopTaskQrService();
            WifiService_Disconnect();
          } else {
            Serial.println("Usage: WIFI OFF");
          }
      } else if (token == "HEX") {
        String arg = args;
        arg.trim();
        if (arg == "ON") {
          QrService_SetHexDump(true);
          Serial.println("[CLI] HEX dump: ON");
        } else if (arg == "OFF") {
          QrService_SetHexDump(false);
          Serial.println("[CLI] HEX dump: OFF");
        } else {
          Serial.println("Usage: HEX ON|OFF");
        }
      } else if (token == "TX2") {
        String msg = args;
        Serial2.println(msg);
        Serial.println("[CLI] TX2 sent");
      } else if (token == "TX2HEX") {
        String hex = args;
        hex.trim();
        int i = 0;
        auto hexval = [](char ch) -> int {
          if (ch >= '0' && ch <= '9') return ch - '0';
          if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
          if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
          return -1;
        };
        while (i < (int)hex.length()) {
          while (i < (int)hex.length() && hex[i] == ' ') i++;
          if (i + 1 >= (int)hex.length()) break;
          int v1 = hexval(hex[i++]);
          int v2 = hexval(hex[i++]);
          if (v1 < 0 || v2 < 0) continue;
          uint8_t b = (uint8_t)((v1 << 4) | v2);
          Serial2.write(b);
        }
        Serial.println("[CLI] TX2HEX sent");
      } else {
        Serial.print("[CLI] Commande inconnue: ");
        Serial.println(cmd);
        Serial.println("Tapez HELP");
      }
    } else {
      // accumulate
      if (cliBuffer.length() < 64) {
        cliBuffer += c;
      } else {
        cliBuffer = ""; // reset en cas de dépassement
      }
    }
  }

  delay(10);
}