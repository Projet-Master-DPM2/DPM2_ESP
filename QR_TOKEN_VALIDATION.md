# Validation des Tokens QR - Feature Implementation

## Vue d'ensemble

Cette fonctionnalité permet au système ESP32 de détecter automatiquement les tokens QR scannés et de les valider auprès du backend pour autoriser ou refuser une commande.

## Format des Tokens QR

Les tokens QR suivent le format suivant :
```
qr_<UUID>_<timestamp>
```

**Exemple** : `qr_6565917e-a288-4d69-8693-b55e19ea8ba8_1755260515417`

- **Préfixe** : `qr_` (obligatoire)
- **UUID** : Identifiant unique de la commande (format UUID v4)  
- **Timestamp** : Horodatage de création du token

## Architecture

### 1. Détection QR (qr_service.cpp)
- Le service QR lit en continu les données du scanner via UART2 (GPIO 16/17)
- Détection automatique des tokens basée sur le pattern `qr_*_*`
- Envoi d'un événement `ORCH_EVT_QR_TOKEN_READ` à l'orchestrateur

### 2. Validation HTTP (http_service.cpp)
- Nouvelle fonction `HttpService_ValidateQRToken()`
- Requête POST sécurisée vers l'endpoint de validation
- Construction automatique du payload JSON

### 3. Orchestration (orchestrator.cpp)
- Traitement de l'événement `ORCH_EVT_QR_TOKEN_READ`
- Vérification de la connectivité WiFi avant validation
- Gestion des réponses HTTP et communication avec NUCLEO

## Endpoint API

**URL** : `https://iot-vending-machine.osc-fr1.scalingo.io/api/order-validation/validate-token`

**Méthode** : POST

**Headers** :
```
Content-Type: application/json
User-Agent: DPM2-ESP32/1.0
```

**Body** :
```json
{
  "qr_code_token": "qr_6565917e-a288-4d69-8693-b55e19ea8ba8_1755260515417"
}
```

**Réponses** :
- **200** : Token valide, commande autorisée
- **400/404/401** : Token invalide ou expiré
- **500** : Erreur serveur

## Flux de Traitement

1. **Scanner QR** → Données UART2
2. **QR Service** → Détection pattern + Événement orchestrateur
3. **Orchestrateur** → Vérification WiFi + Validation HTTP
4. **HTTP Service** → Requête POST sécurisée
5. **Orchestrateur** → Traitement réponse + Commande NUCLEO
6. **UART Service** → Transmission résultat vers NUCLEO

## Messages UART vers NUCLEO

- `QR_TOKEN_VALID` : Token validé avec succès
- `QR_TOKEN_INVALID` : Token invalide ou expiré  
- `QR_TOKEN_NO_NETWORK` : Pas de connectivité WiFi
- `QR_TOKEN_ERROR` : Erreur de communication

## Configuration

### Pins UART
- **QR Scanner** : UART2 (RX=GPIO16, TX=GPIO17)
- **Communication NUCLEO** : UART1 (RX=GPIO26, TX=GPIO25)

### Timeouts
- **Validation HTTP** : 10 secondes
- **Rate Limiting** : Selon configuration sécurité

### Sécurité
- Validation TLS/HTTPS avec certificats CA
- Rate limiting sur les requêtes
- Masquage des données sensibles dans les logs
- Vérification d'intégrité des URLs

## Tests

### Test Local
```bash
cd DPM2_ESP
g++ -o test_qr_validation test_qr_validation.cpp
./test_qr_validation
```

### Test Matériel
1. Connecter le scanner QR sur GPIO 16/17
2. Scanner un token au format correct
3. Vérifier les logs série pour le traitement
4. Contrôler la communication UART avec NUCLEO

## Monitoring

### Logs Série
```
[QR] Token QR détecté, envoi à l'orchestrateur
[ORCH] QR Token reçu: qr_xxx_xxx
[ORCH] Validation du QR Token...
[HTTP] Validation QR token: qr_xxx_xxx
[HTTP] POST response: 200 (xx bytes)
[ORCH] QR Token validé avec succès - Envoi commande à NUCLEO
```

### Métriques
- Tokens scannés
- Taux de validation réussite/échec  
- Temps de réponse API
- Erreurs réseau

## Intégration Continue

La fonctionnalité est intégrée dans le pipeline CI/CD existant :
- Compilation automatique
- Tests unitaires (à venir)
- Vérification cppcheck
- Packaging firmware

## Évolutions Futures

1. **Cache local** : Mise en cache temporaire des tokens validés
2. **Retry logic** : Nouvelles tentatives en cas d'échec réseau
3. **Métriques avancées** : Statistiques détaillées d'utilisation
4. **Tests unitaires** : Couverture complète des cas d'usage
5. **Configuration dynamique** : Paramètres modifiables via interface web
