#pragma once

#include <Arduino.h>

// Démarre la tâche Wi-Fi: tentative creds NVS, sinon SoftAP + portail HTTP pour configurer
void StartTaskWifiService();

// Indique si la connexion Internet est prête (Wi-Fi connecté + test backend optionnel)
bool WifiService_IsReady();

// Déconnexion du Wi‑Fi (et relance du SoftAP de provisioning)
void WifiService_Disconnect();

// Affiche l'état Wi‑Fi (STA/AP) sur le port série
void WifiService_DebugStatus();


