#include "services/wifi_service.h"
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

static void handleSave() {
  String ssid = server.hasArg("ssid") ? server.arg("ssid") : "";
  String pass = server.hasArg("pass") ? server.arg("pass") : "";
  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID manquant");
    return;
  }
  server.send(200, "text/plain", "Connexion en cours... Redemarrer la page.");
  // Sauver et tenter connexion
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  credsUpdated = true;
}

static bool testConnectSaved(uint32_t timeoutMs) {
  // Ouvrir RW pour créer l'espace au besoin et éviter le spam NOT_FOUND
  prefs.begin("wifi", false);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();
  if (ssid.isEmpty()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t start = millis();
  Serial.printf("[WIFI] Connexion a '%s'...\n", ssid.c_str());
  while (millis() - start < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Connecte, IP=%s\n", WiFi.localIP().toString().c_str());
      return true;
    }
    delay(200);
  }
  Serial.println("[WIFI] Echec connexion");
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


