# DPM2 ESP32 - Module de Communication

## 📋 Description

Module ESP32 du projet **DPM (Distributeur de Produits Modulaire)** - un distributeur automatique intelligent. L'ESP32 gère la connectivité réseau, la lecture NFC/QR et la communication avec la carte NUCLEO STM32.

## 🏗️ Architecture

### Rôle de l'ESP32
- **Connectivité Wi-Fi** avec provisioning SoftAP
- **Lecture NFC** (RC522) et QR code
- **Communication UART** avec la NUCLEO STM32
- **Interface REST API** avec le backend
- **Orchestration** des événements entre services

### Framework & OS
- **Arduino-ESP32** (PlatformIO)
- **FreeRTOS** natif
- **Architecture événementielle** avec queues FreeRTOS

## 🔧 Composants Matériels

| Composant | Interface | Pins | Description |
|-----------|-----------|------|-------------|
| RC522 NFC | SPI (VSPI) | SCK=18, MISO=19, MOSI=23, SS=5, RST=22 | Lecteur RFID/NFC |
| QR Scanner | UART2 | RX=16, TX=17 | Lecteur QR code série |
| NUCLEO STM32 | UART1 | RX=26, TX=25 | Communication inter-cartes |
| USB Debug | UART0 | RX=3, TX=1 | Console de débogage |

## 📁 Structure du Projet

```
DPM2_ESP/
├── src/
│   ├── main.cpp              # Point d'entrée + CLI
│   ├── orchestrator.cpp      # Orchestrateur principal
│   ├── cli.cpp              # Parsing des commandes CLI
│   ├── uart_parser.cpp      # Parsing UART NUCLEO
│   ├── http_utils.cpp       # Utilitaires HTTP
│   ├── nfc_ndef.cpp         # Parsing NDEF NFC
│   └── services/
│       ├── wifi_service.cpp      # Gestion Wi-Fi + SoftAP
│       ├── nfc_service.cpp       # Service NFC RC522
│       ├── qr_service.cpp        # Service QR code
│       ├── uart_service.cpp      # Communication NUCLEO
│       └── http_service.cpp      # Client REST API
├── include/
│   ├── config.h             # Configuration globale
│   ├── orchestrator.h       # Types d'événements
│   ├── cli.h               # Commandes CLI
│   ├── uart_parser.h       # Parser UART
│   ├── http_utils.h        # Utilitaires HTTP
│   ├── nfc_ndef.h          # Parser NDEF
│   └── services/           # Headers des services
├── test/                   # Tests unitaires natifs
├── lib/dmp_core/          # Bibliothèque pour tests
└── platformio.ini         # Configuration PlatformIO
```

## 🚀 Démarrage Rapide

### Prérequis
- **PlatformIO** installé
- **ESP32-WROOM-32D** 
- **Lecteur RC522** câblé selon le schéma
- **Scanner QR** optionnel sur UART2

### Installation

```bash
# Cloner le projet
git clone <repo-url>
cd DPM2_ESP

# Compiler et flasher
pio run -e esp32dev -t upload

# Moniteur série
pio device monitor -b 115200
```

### Premier Démarrage

1. **Connexion Wi-Fi** : L'ESP32 démarre en mode SoftAP
   - SSID : `ESP32-Config-XXXX`
   - Mot de passe : `12345678`
   - Page web : `http://192.168.4.1`

2. **Configuration** : Entrez vos identifiants Wi-Fi via le portail web

3. **Test CLI** : Utilisez les commandes série pour tester

## 🎮 Commandes CLI

| Commande | Description | Exemple |
|----------|-------------|---------|
| `HELP` | Affiche l'aide | `HELP` |
| `INFO` | Informations système | `INFO` |
| `SCAN` | Déclenche un scan NFC | `SCAN` |
| `WIFI?` | Statut Wi-Fi | `WIFI?` |
| `WIFI OFF` | Déconnexion Wi-Fi | `WIFI OFF` |
| `HTTPGET <url>` | Requête GET | `HTTPGET https://httpbin.org/get` |
| `HTTPPOST <url>` | Requête POST | `HTTPPOST https://httpbin.org/post` |
| `TX1 <msg>` | Envoi UART1 (NUCLEO) | `TX1 HELLO` |
| `TX2 <msg>` | Envoi UART2 (QR) | `TX2 TEST` |
| `HEX ON/OFF` | Mode hexadécimal QR | `HEX ON` |

## 🔄 Flux de Communication

### 1. Flux NFC
```
[Utilisateur] → [Badge NFC] → [RC522] → [NFC Service] 
    ↓
[Orchestrator] → [UART Service] → [NUCLEO] → [Backend]
    ↓
[Réponse Backend] → [NUCLEO] → [ESP32] → [Affichage LCD]
```

### 2. Protocole UART avec NUCLEO

**ESP32 → NUCLEO :**
- `NFC_UID:<uid_hex>` : UID lu
- `NFC_DATA:<text>` : Données NDEF
- `NFC_ERR:TIMEOUT` : Erreur de lecture

**NUCLEO → ESP32 :**
- `STATE:PAYING` : Demande de scan NFC
- `STATE:IDLE` : Retour à l'état repos

**Réponses ESP32 :**
- `ACK:STATE:PAYING` : Wi-Fi OK, scan autorisé
- `NAK:STATE:PAYING:NO_NET` : Pas de Wi-Fi
- `NAK:PAYMENT:DENIED` : Paiement refusé

## 🔒 Sécurité

### Mesures Implémentées
- ✅ **Validation d'entrée** stricte (UART, HTTP, NFC)
- ✅ **Protection buffer overflow** avec limites strictes
- ✅ **HTTPS/TLS** pour les communications backend
- ✅ **Masquage des secrets** dans les logs
- ✅ **Stockage sécurisé** des credentials Wi-Fi (NVS)
- ✅ **Gestion d'erreurs** sans fuite d'informations
- ✅ **Logging minimaliste** configurable par niveau
- ✅ **Optimisation mémoire** avec stack sizes réduits

### Configuration Sécurité

Dans `include/config.h` :
```cpp
#define LOG_LEVEL 2              // 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug
#define ALLOW_INSECURE_TLS 0     // 0=Production (TLS strict), 1=Dev
#define HTTP_MAX_PAYLOAD 1024    // Limite payload HTTP
```

## 🧪 Tests

### Tests Unitaires Natifs

```bash
# Exécuter tous les tests
pio test -e native

# Tests spécifiques
pio test -e native --filter "*cli*"
pio test -e native --filter "*uart*"
pio test -e native --filter "*http*"
pio test -e native --filter "*nfc*"
```

### Tests Disponibles
- **CLI Parsing** : Validation des commandes
- **UART Parser** : Protocole NUCLEO
- **HTTP Utils** : Validation URLs
- **NFC NDEF** : Parsing TLV/NDEF
- **Orchestrator Logic** : Gestion d'événements
- **Wi-Fi Validation** : SSID/password
- **NFC Utils** : Utilitaires cartes

## 🔧 Configuration

### Pins (config.h)
```cpp
// NFC RC522 (VSPI)
#define NFC_SCK_PIN    18
#define NFC_MISO_PIN   19  
#define NFC_MOSI_PIN   23
#define NFC_SS_PIN     5
#define NFC_RST_PIN    22

// UART
#define UART1_RX_PIN   26  // NUCLEO
#define UART1_TX_PIN   25
#define UART2_RX_PIN   16  // QR Scanner  
#define UART2_TX_PIN   17
```

### Timeouts & Tailles
```cpp
#define NFC_SCAN_TIMEOUT_MS     15000
#define NFC_DEBOUNCE_MS         1000
#define WIFI_CONNECT_TIMEOUT_MS 30000
#define HTTP_TIMEOUT_MS         10000
```

## 🚀 CI/CD

### Pipeline GitHub Actions
- ✅ **Tests unitaires natifs** avec rapport JUnit
- ✅ **Analyse statique** cppcheck avec rapport HTML  
- ✅ **Build firmware** avec `-Werror`
- ✅ **Release automatique** sur tags
- ✅ **Notifications email** de statut

### Commandes Locales
```bash
# Validation locale (miroir CI)
./scripts/ci-local.sh

# Tests natifs seulement
pio test -e native

# Build avec warnings=errors
CI_WERROR=-Werror pio run -e esp32dev

# Analyse statique
cppcheck --std=c11 --enable=all -I include -I src .
```

## 📊 Monitoring

### Logs Runtime
```
[DPM2] ESP32 boot
[BOARD] Free heap: 298234 bytes
[WIFI] Connecting to saved SSID: MyWiFi***
[NFC] RC522 init done (version: 0x92)
[UART1] start @ 115200 bps (RX=26, TX=25)
[HTTP] Service started, queue ready
```

### Métriques Système
- **Heap libre** : Surveillance continue
- **Stack usage** : Optimisé par service
- **Queue depths** : Monitoring des files d'attente
- **Wi-Fi RSSI** : Qualité signal

## 🤝 Intégration NUCLEO

### Messages Attendus
```
STATE:PAYING    # Demande scan NFC
STATE:IDLE      # Retour repos  
STATE:ORDERING  # Mode commande
STATE:DELIVERING # Distribution en cours
```

### Réponses ESP32
```
ACK:STATE:PAYING              # Scan autorisé
NAK:STATE:PAYING:NO_NET       # Pas de réseau
NAK:PAYMENT:DENIED            # Paiement refusé
NFC_UID:01020304              # UID carte
NFC_DATA:Hello World          # Données NDEF
NFC_ERR:TIMEOUT               # Erreur lecture
```

## 📝 Changelog

Voir les [releases GitHub](../../releases) pour l'historique détaillé des versions.

## 🐛 Dépannage

### Problèmes Courants

**Wi-Fi ne se connecte pas :**
- Vérifier SSID/password via SoftAP
- Redémarrer : `WIFI OFF` puis reconfigurer

**NFC ne lit pas :**
- Vérifier câblage RC522
- Tester avec `SCAN` en CLI
- Contrôler alimentation 3.3V

**UART silencieux :**
- Vérifier connexions RX/TX croisées
- Tester avec `TX1 TEST`
- Contrôler baudrate (115200)

### Debug Avancé

```cpp
// Dans config.h, passer en mode debug
#define LOG_LEVEL 4  // Debug complet

// Monitoring heap
#define HEAP_MONITOR_ENABLED 1
```

## 📄 Licence

Ce projet fait partie du cursus académique M2 - Sophia Ynov Campus
Réalisé par l'équipe DPM - Distributeur Projet Master

---

**Équipe DPM2** - Distributeur Automatique Intelligent  
*ESP32 Communication Module - v1.0*

