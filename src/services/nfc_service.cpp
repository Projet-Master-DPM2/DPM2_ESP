#include <Arduino.h>
#include "services/nfc_service.h"
#include "config.h"
#include "security_config.h"
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
#include "orchestrator.h"

static TaskHandle_t nfcTaskHandle = nullptr;
static QueueHandle_t orchestratorQueueHandle = nullptr;
static MFRC522* mfrc522 = nullptr;
static volatile bool nfcBusy = false;
static uint8_t rc522Version = 0;

static void nfcTask(void* pvParameters);

static void sendEvent(OrchestratorEventType type, const char* message) {
  if (!orchestratorQueueHandle) return;
  OrchestratorEvent evt{};
  evt.type = type;
  strncpy(evt.payload, message ? message : "", sizeof(evt.payload) - 1);
  evt.payload[sizeof(evt.payload) - 1] = '\0';
  xQueueSend(orchestratorQueueHandle, &evt, 0);
}

void StartTaskNfcService(QueueHandle_t orchestratorQueue) {
  orchestratorQueueHandle = orchestratorQueue;

  if (!mfrc522) {
    SPI.begin(RC522_SCK, RC522_MISO, RC522_MOSI, RC522_SS);
    mfrc522 = new MFRC522(RC522_SS, RC522_RST);
    mfrc522->PCD_Init();
    Serial.println("[NFC] RC522 init done");
    rc522Version = mfrc522->PCD_ReadRegister(MFRC522::VersionReg);
    Serial.printf("[NFC] Version: 0x%02X\n", rc522Version);
  }

  if (!nfcTaskHandle) {
    xTaskCreate(
      nfcTask,
      "nfc_service",
      NFC_TASK_STACK_SIZE,
      nullptr,
      NFC_TASK_PRIORITY,
      &nfcTaskHandle
    );
  }
}

void NfcService_DebugInfo() {
  Serial.printf("[NFC] Busy=%s, Timeout=%d ms, RC522 Version=0x%02X\n",
                nfcBusy ? "YES" : "NO",
                (int)NFC_SCAN_TIMEOUT_MS,
                rc522Version);
}

void StopTaskNfcService() {
  if (nfcTaskHandle) {
    vTaskDelete(nfcTaskHandle);
    nfcTaskHandle = nullptr;
    nfcBusy = false;
    Serial.println("[NFC] service stopped");
  }
}

bool NfcService_IsRunning() {
  return nfcTaskHandle != nullptr;
}

TaskHandle_t NfcService_GetTaskHandle() {
  return nfcTaskHandle;
}

bool NfcService_TriggerScan() {
  // Rate limiting pour éviter le spam NFC
  if (!rateLimitCheck("NFC", NFC_SCAN_COOLDOWN_MS)) {
    SECURE_LOG_ERROR("NFC", "Scan rate limited");
    return false;
  }
  
  if (nfcTaskHandle == nullptr) {
    SECURE_LOG_ERROR("NFC", "Service not initialized");
    return false;
  }
  
  if (nfcBusy) {
    SECURE_LOG_ERROR("NFC", "Scanner busy");
    return false;
  }
  
  SECURE_LOG_INFO("NFC", "Scan triggered");
  logSecurityEvent("NFC_SCAN_TRIGGERED", "User initiated");
  xTaskNotifyGive(nfcTaskHandle);
  return true;
}

static String uidToHex(const MFRC522::Uid& uid) {
  char buffer[2 * 10 + 1]; // UID jusqu'à 10 octets
  size_t index = 0;
  for (byte i = 0; i < uid.size && i < 10; i++) {
    sprintf(&buffer[index], "%02X", uid.uidByte[i]);
    index += 2;
  }
  buffer[index] = '\0';
  return String(buffer);
}

// ---- Helpers NDEF ----
static bool parseTlvAndExtractNdefText(const uint8_t* mem, size_t totalLen, String& outText) {
  size_t i = 0;
  while (i < totalLen) {
    uint8_t t = mem[i++];
    if (t == 0x00) { // NULL TLV
      continue;
    }
    if (t == 0xFE) { // Terminator TLV
      break;
    }
    if (i >= totalLen) break;
    uint16_t L = mem[i++];
    if (L == 0xFF) {
      if (i + 1 >= totalLen) break;
      L = ((uint16_t)mem[i] << 8) | mem[i + 1];
      i += 2;
    }
    if (i + L > totalLen) break;

    if (t == 0x03) { // NDEF Message TLV
      // Parse first NDEF record as Text (TNF=0x01, type 'T')
      const uint8_t* p = mem + i;
      const uint8_t* end = p + L;
      if (p >= end) return false;
      uint8_t hdr = *p++;
      bool sr = (hdr & 0x10) != 0;
      bool il = (hdr & 0x08) != 0;
      uint8_t tnf = (hdr & 0x07);
      if (p >= end) return false;
      uint8_t typeLen = *p++;
      uint32_t payloadLen = 0;
      if (sr) {
        if (p >= end) return false;
        payloadLen = *p++;
      } else {
        if (p + 4 > end) return false;
        payloadLen = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
        p += 4;
      }
      uint8_t idLen = 0;
      if (il) {
        if (p >= end) return false;
        idLen = *p++;
      }
      if (p + typeLen > end) return false;
      const uint8_t* typePtr = p;
      p += typeLen;
      if (il) {
        if (p + idLen > end) return false;
        p += idLen;
      }
      if (p + payloadLen > end) return false;

      if (tnf == 0x01 && typeLen == 1 && typePtr[0] == 'T' && payloadLen >= 1) {
        uint8_t status = p[0];
        uint8_t langLen = status & 0x3F;
        bool utf16 = (status & 0x80) != 0;
        if (utf16) {
          // Non supporté: afficher brut en hex si besoin
          return false;
        }
        if (1 + langLen > payloadLen) return false;
        const uint8_t* txt = p + 1 + langLen;
        uint32_t txtLen = payloadLen - 1 - langLen;
        // Limiter la taille pour notre payload
        char buf[120];
        uint32_t n = (txtLen < sizeof(buf) - 1) ? txtLen : (sizeof(buf) - 1);
        memcpy(buf, txt, n);
        buf[n] = '\0';
        outText = String(buf);
        return true;
      }
      // Si pas un Text record, ignorer (on pourrait parser d'autres records au besoin)
    }

    i += L; // avancer au TLV suivant
  }
  return false;
}

static bool readUltralightText(MFRC522& reader, String& outText) {
  auto ulRead16 = [&](byte startPage, uint8_t* out16) -> bool {
    uint8_t cmd[2] = { 0x30, startPage }; // READ command for Ultralight (reads 4 pages = 16 bytes)
    uint8_t resBuf[18];
    byte resLen = sizeof(resBuf);
    MFRC522::StatusCode sc = reader.PCD_TransceiveData(cmd, 2, resBuf, &resLen, nullptr, 0, true);
    if (sc != MFRC522::STATUS_OK || resLen < 16) return false;
    memcpy(out16, resBuf, 16);
    return true;
  };

  uint8_t mem[192];
  size_t total = 0;
  for (byte page = 4; page < 40 && (total + 16) <= sizeof(mem); page += 4) {
    uint8_t tmp[16];
    if (!ulRead16(page, tmp)) {
      break;
    }
    memcpy(&mem[total], tmp, 16);
    total += 16;
  }
  if (total == 0) return false;
  return parseTlvAndExtractNdefText(mem, total, outText);
}

static bool readClassicText(MFRC522& reader, String& outText) {
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF; // clé A par défaut

  // Lire blocs 4..6 (secteur 1 data)
  uint8_t mem[48];
  size_t total = 0;
  for (byte block = 4; block <= 6; block++) {
    MFRC522::StatusCode res = reader.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &reader.uid);
    if (res != MFRC522::STATUS_OK) {
      reader.PCD_StopCrypto1();
      return false;
    }
    byte buffer[18];
    byte size = sizeof(buffer);
    res = reader.MIFARE_Read(block, buffer, &size);
    if (res != MFRC522::STATUS_OK || size < 16) {
      reader.PCD_StopCrypto1();
      return false;
    }
    memcpy(&mem[total], buffer, 16);
    total += 16;
  }
  reader.PCD_StopCrypto1();
  if (total == 0) return false;

  // 1) Essayer NDEF TLV (si la carte est encodée NDEF)
  if (parseTlvAndExtractNdefText(mem, total, outText)) {
    return true;
  }

  // 2) Fallback: si contenu ASCII brut (comme dans ton dump), l'extraire
  String ascii;
  ascii.reserve(total);
  for (size_t i = 0; i < total; i++) {
    uint8_t b = mem[i];
    if (b == 0x00) {
      // fin de chaîne probable
      break;
    }
    if (b >= 0x20 && b <= 0x7E) {
      ascii += (char)b;
    } else {
      // remplacer par espace pour rester lisible
      ascii += ' ';
    }
  }
  ascii.trim();
  if (ascii.length() >= 3) {
    outText = ascii;
    return true;
  }

  return false;
}

static void nfcTask(void* pvParameters) {
  for (;;) {
    // Attente passive d'une notification pour démarrer un scan
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    nfcBusy = true;
    Serial.println("[NFC] Scan window opened");

    // Fenêtre de scan limitée dans le temps
    const unsigned long startMs = millis();
    bool found = false;
    String lastUidHex;
    unsigned long lastReadMs = 0;

    while ((millis() - startMs) < NFC_SCAN_TIMEOUT_MS) {
      if (mfrc522 && mfrc522->PICC_IsNewCardPresent() && mfrc522->PICC_ReadCardSerial()) {
        String uidHex = uidToHex(mfrc522->uid);
        unsigned long now = millis();
        bool isBounce = (uidHex == lastUidHex) && ((now - lastReadMs) < NFC_DEBOUNCE_MS);

        if (!isBounce) {
          lastUidHex = uidHex;
          lastReadMs = now;
          
          // Validation et masquage de l'UID
          if (!isValidNFCData(uidHex.c_str())) {
            SECURE_LOG_ERROR("NFC", "Invalid UID format detected");
            continue;
          }
          
          String maskedUID = maskUID(uidHex);
          SECURE_LOG_INFO("NFC", "Valid card detected: %s", maskedUID.c_str());
          logSecurityEvent("NFC_CARD_READ", ("UID: " + maskedUID).c_str());
          
          sendEvent(ORCH_EVT_NFC_UID_READ, uidHex.c_str());

          // Essayer de lire un enregistrement texte NDEF
          String text;
          bool ok = false;
          // Déterminer type de PICC
          byte sak = mfrc522->PICC_GetType(mfrc522->uid.sak);
          MFRC522::PICC_Type piccType = mfrc522->PICC_GetType(mfrc522->uid.sak);
          (void)sak;
          if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
            ok = readUltralightText(*mfrc522, text);
          } else if (piccType == MFRC522::PICC_TYPE_MIFARE_1K || piccType == MFRC522::PICC_TYPE_MIFARE_4K) {
            ok = readClassicText(*mfrc522, text);
          }
          if (ok) {
            sendEvent(ORCH_EVT_NFC_DATA, text.c_str());
          }

          found = true;
          mfrc522->PICC_HaltA();
          mfrc522->PCD_StopCrypto1();
          break;
        }

        mfrc522->PICC_HaltA();
        mfrc522->PCD_StopCrypto1();
      }

      vTaskDelay(pdMS_TO_TICKS(30));
    }

    if (!found) {
      sendEvent(ORCH_EVT_NFC_ERROR, "TIMEOUT");
      Serial.println("[NFC] TIMEOUT");
    }

    nfcBusy = false;
    Serial.println("[NFC] Scan window closed");
    // Boucle: retour à l'attente d'une nouvelle notification
  }
}


