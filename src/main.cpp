#include <Arduino.h>
#include "config.h"
#include "orchestrator.h"
#include "services/nfc_service.h"
#include "services/uart_service.h"
#include "services/qr_service.h"
#include "services/wifi_service.h"
#include "services/http_service.h"

// --- CLI command mapping in cli.h ---
#include "cli.h"

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
    StartTaskHttpService();
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

      switch (parseCommand(token.c_str())) {
        case CMD_HELP: {
          Serial.println("CMD: SCAN -> demarrer une fenetre NFC");
          Serial.println("CMD: HELP -> aide");
          Serial.println("CMD: HEX ON|OFF -> activer/desactiver hexdump QR");
          Serial.println("CMD: TX2 <texte> -> envoyer sur UART2");
          Serial.println("CMD: TX2HEX <octets hex> -> envoyer binaire sur UART2");
          Serial.println("CMD: TX1 <texte> -> envoyer sur UART1 (vers NUCLEO)");
          Serial.println("CMD: INFO -> afficher etat UART1/NFC");
          Serial.println("CMD: WIFI? -> etat Wi-Fi");
          Serial.println("CMD: WIFI OFF -> deconnecter et relancer le portail SoftAP");
          Serial.println("CMD: HTTPGET <url> -> requete GET");
          Serial.println("CMD: HTTPPOST <url>|<ctype>|<body> -> requete POST");
          break;
        }
        case CMD_SCAN: {
          bool ok = NfcService_TriggerScan();
          Serial.println(ok ? "[CLI] NFC scan demarre" : "[CLI] NFC deja en cours");
          break;
        }
        case CMD_TX1: {
          String msg = args;
          UartService_SendLine(msg.c_str());
          Serial.println("[CLI] TX1 sent");
          break;
        }
        case CMD_INFO: {
          Serial.printf("[INFO] UART1 RX=%d, TX=%d, BAUD=%lu\n", UART_RX_PIN, UART_TX_PIN, (unsigned long)UART_BAUDRATE);
          NfcService_DebugInfo();
          break;
        }
        case CMD_WIFI_Q: {
          WifiService_DebugStatus();
          break;
        }
        case CMD_WIFI: {
          String arg = args;
          arg.trim();
          if (arg == "OFF") {
            if (NfcService_IsRunning()) StopTaskNfcService();
            if (QrService_IsRunning()) StopTaskQrService();
            WifiService_Disconnect();
          } else {
            Serial.println("Usage: WIFI OFF");
          }
          break;
        }
        case CMD_HTTPGET: {
          if (args.length() == 0) {
            Serial.println("Usage: HTTPGET <url>");
          } else {
            HttpService_Get(args.c_str(), nullptr, 8000);
          }
          break;
        }
        case CMD_HTTPPOST: {
          if (args.length() == 0) {
            Serial.println("Usage: HTTPPOST <url>|<content-type>|<body>");
          } else {
            int p1 = args.indexOf('|');
            int p2 = p1 >= 0 ? args.indexOf('|', p1 + 1) : -1;
            if (p1 < 0 || p2 < 0) {
              Serial.println("Usage: HTTPPOST <url>|<content-type>|<body>");
            } else {
              String url = args.substring(0, p1);
              String ctype = args.substring(p1 + 1, p2);
              String body = args.substring(p2 + 1);
              url.trim(); ctype.trim(); body.trim();
              HttpService_Post(url.c_str(), ctype.c_str(), body.c_str(), nullptr, 10000);
            }
          }
          break;
        }
        case CMD_HEX: {
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
          break;
        }
        case CMD_TX2: {
          String msg = args;
          Serial2.println(msg);
          Serial.println("[CLI] TX2 sent");
          break;
        }
        case CMD_TX2HEX: {
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
          break;
        }
        case CMD_UNKNOWN:
        default: {
          Serial.print("[CLI] Commande inconnue: ");
          Serial.println(cmd);
          Serial.println("Tapez HELP");
          break;
        }
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
      // if (token == "HTTPPOST") {
      //   int p1 = args.indexOf('|');
      //   int p2 = p1 >= 0 ? args.indexOf('|', p1 + 1) : -1;
      //   if (p1 < 0 || p2 < 0) {
      //     Serial.println("Usage: HTTPPOST <url>|<content-type>|<body>");
      //   } else {
      //     String url = args.substring(0, p1);
      //     String ctype = args.substring(p1 + 1, p2);
      //     String body = args.substring(p2 + 1);
      //     url.trim(); ctype.trim(); body.trim();
      //     HttpService_Post(url.c_str(), ctype.c_str(), body.c_str(), nullptr, 10000);
      //   }
      // }
