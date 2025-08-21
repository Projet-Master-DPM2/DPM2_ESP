#include "services/wifi_service.h"
#include "security_config.h"
#include "../security_utils.cpp"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

static TaskHandle_t wifiTaskHandle = nullptr;
static volatile bool networkReady = false;
static WebServer server(80);
static Preferences prefs;
static volatile bool credsUpdated = false;

static void handleRoot() {
  String html =
    "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>"
    "<h3>Configurer le Wi-Fi</h3>"
    "<form method='POST' action='/save'>"
    "SSID: <input name='ssid'/><br/>"
    "Mot de passe: <input name='pass' type='password'/><br/>"
    "<button type='submit'>Enregistrer</button>"
    "</form>"
    "</body></html>";
  server.send(200, "text/html", html);
}

// Sauvegarde sécurisée des credentials WiFi
static bool saveWifiCredentials(const String& ssid, const String& password) {
  // Validation des entrées
  if (!isValidSSID(ssid.c_str())) {
    SECURE_LOG_ERROR("WIFI", "Invalid SSID rejected");
    return false;
  }
  
  if (password.length() > MAX_PASSWORD_LENGTH) {
    SECURE_LOG_ERROR("WIFI", "Password too long");
    return false;
  }
  
  // Générer la clé de chiffrement
  uint8_t encKey[NVS_ENCRYPTION_KEY_SIZE];
  if (!deriveKey(PASSWORD_SALT, ssid.c_str(), encKey, sizeof(encKey))) {
    SECURE_LOG_ERROR("WIFI", "Failed to derive encryption key");
    return false;
  }
  
  // Chiffrer le mot de passe
  char encryptedPassword[256];
  if (WIFI_CREDS_ENCRYPTION_ENABLED && password.length() > 0) {
    if (!encryptData(password.c_str(), encKey, encryptedPassword, sizeof(encryptedPassword))) {
      SECURE_LOG_ERROR("WIFI", "Failed to encrypt password");
      return false;
    }
  } else {
    strncpy(encryptedPassword, password.c_str(), sizeof(encryptedPassword) - 1);
    encryptedPassword[sizeof(encryptedPassword) - 1] = '\0';
  }
  
  // Sauvegarder dans NVS sécurisé
  prefs.begin(NVS_NAMESPACE_SECURE, false);
  bool success = prefs.putString("ssid", ssid) && 
                 prefs.putString("pass_enc", encryptedPassword) &&
                 prefs.putBool("encrypted", WIFI_CREDS_ENCRYPTION_ENABLED);
  prefs.end();
  
  if (success) {
    SECURE_LOG_INFO("WIFI", "Credentials saved securely for SSID: %s", maskSSID(ssid).c_str());
    logSecurityEvent("WIFI_CREDS_SAVED", ("SSID: " + maskSSID(ssid)).c_str());
  } else {
    SECURE_LOG_ERROR("WIFI", "Failed to save credentials");
  }
  
  // Nettoyer la mémoire
  SECURE_ZERO(encKey, sizeof(encKey));
  SECURE_ZERO(encryptedPassword, sizeof(encryptedPassword));
  
  return success;
}

// Chargement sécurisé des credentials WiFi
static bool loadWifiCredentials(String& ssid, String& password) {
  prefs.begin(NVS_NAMESPACE_SECURE, true);
  
  ssid = prefs.getString("ssid", "");
  String encryptedPassword = prefs.getString("pass_enc", "");
  bool isEncrypted = prefs.getBool("encrypted", false);
  
  prefs.end();
  
  if (ssid.isEmpty()) {
    return false;
  }
  
  // Déchiffrer le mot de passe si nécessaire
  if (isEncrypted && encryptedPassword.length() > 0) {
    uint8_t encKey[NVS_ENCRYPTION_KEY_SIZE];
    if (!deriveKey(PASSWORD_SALT, ssid.c_str(), encKey, sizeof(encKey))) {
      SECURE_LOG_ERROR("WIFI", "Failed to derive decryption key");
      return false;
    }
    
    char decryptedPassword[128];
    if (!decryptData(encryptedPassword.c_str(), encKey, decryptedPassword, sizeof(decryptedPassword))) {
      SECURE_LOG_ERROR("WIFI", "Failed to decrypt password");
      SECURE_ZERO(encKey, sizeof(encKey));
      return false;
    }
    
    password = String(decryptedPassword);
    
    // Nettoyer la mémoire
    SECURE_ZERO(encKey, sizeof(encKey));
    SECURE_ZERO(decryptedPassword, sizeof(decryptedPassword));
  } else {
    password = encryptedPassword;
  }
  
  SECURE_LOG_INFO("WIFI", "Credentials loaded for SSID: %s", maskSSID(ssid).c_str());
  return true;
}

static void handleSave() {
  String ssid = server.hasArg("ssid") ? server.arg("ssid") : "";
  String pass = server.hasArg("pass") ? server.arg("pass") : "";
  
  // Rate limiting pour éviter le spam
  if (!rateLimitCheck("WIFI", 5000)) { // 5 secondes minimum entre tentatives
    server.send(429, "text/plain", "Trop de tentatives, veuillez patienter");
    return;
  }
  
  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID manquant");
    return;
  }
  
  // Validation et sauvegarde sécurisée
  if (!saveWifiCredentials(ssid, pass)) {
    server.send(500, "text/plain", "Erreur lors de la sauvegarde");
    return;
  }
  
  server.send(200, "text/plain", "Connexion en cours... Redemarrer la page.");
  credsUpdated = true;
}

static bool testConnectSaved(uint32_t timeoutMs) {
  String ssid, password;
  
  // Charger les credentials de manière sécurisée
  if (!loadWifiCredentials(ssid, password)) {
    SECURE_LOG_INFO("WIFI", "No saved credentials found");
    return false;
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  uint32_t start = millis();
  SECURE_LOG_INFO("WIFI", "Connecting to '%s'...", maskSSID(ssid).c_str());
  
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      SECURE_LOG_INFO("WIFI", "Connected successfully, IP=%s", WiFi.localIP().toString().c_str());
      logSecurityEvent("WIFI_CONNECTED", ("SSID: " + maskSSID(ssid)).c_str());
      
      // Nettoyer les credentials de la mémoire
      SECURE_ZERO((void*)password.c_str(), password.length());
      
      return true;
    }
    delay(200);
  }
  
  SECURE_LOG_ERROR("WIFI", "Connection failed to %s", maskSSID(ssid).c_str());
  logSecurityEvent("WIFI_CONNECT_FAILED", ("SSID: " + maskSSID(ssid)).c_str());
  
  // Nettoyer les credentials de la mémoire
  SECURE_ZERO((void*)password.c_str(), password.length());
  
  return false;
}

static void startSoftAP() {
  WiFi.mode(WIFI_AP_STA);
  String apSsid = String("DPM-Setup-") + String((uint32_t)ESP.getEfuseMac(), HEX);
  const char* apPass = "dpmsetup";
  WiFi.softAP(apSsid.c_str(), apPass);
  Serial.printf("[WIFI] SoftAP '%s' pass '%s' IP=%s\n", apSsid.c_str(), apPass, WiFi.softAPIP().toString().c_str());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

static void wifiTask(void* pv) {
  // 1) Essayer creds NVS
  if (!testConnectSaved(10000)) {
    // 2) Lancer SoftAP portail
    startSoftAP();
  }
  // Boucle principale: si STA connecte, networkReady=true; sinon, process portail
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      networkReady = true;
    } else {
      networkReady = false;
    }
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(20));
    // Si de nouvelles creds ont ete enregistrees, retenter la connexion une fois
    if (!networkReady && credsUpdated) {
      credsUpdated = false;
      if (testConnectSaved(8000)) {
        Serial.println("[WIFI] Connecte apres provisioning");
        WiFi.softAPdisconnect(true);
      } else {
        Serial.println("[WIFI] Echec connexion apres provisioning");
      }
    }
  }
}

void StartTaskWifiService() {
  if (!wifiTaskHandle) {
    xTaskCreate(wifiTask, "wifi_service", 6144, nullptr, 2, &wifiTaskHandle);
  }
}

bool WifiService_IsReady() { return networkReady; }

void WifiService_Disconnect() {
  Serial.println("[WIFI] Disconnect requested");
  WiFi.disconnect(true, true); // clear config & disconnect
  networkReady = false;
  startSoftAP();
}

void WifiService_DebugStatus() {
  wl_status_t st = WiFi.status();
  Serial.printf("[WIFI] STA status=%d (%s), IP=%s, AP=%s\n",
                (int)st,
                st == WL_CONNECTED ? "CONNECTED" : "NOT_CONNECTED",
                WiFi.localIP().toString().c_str(),
                WiFi.softAPgetStationNum() > 0 ? "ON" : "OFF");
}


