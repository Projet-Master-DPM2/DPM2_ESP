# Workflow Complet de Commande QR - DPM2 ESP32

## Vue d'ensemble

Ce document décrit le workflow complet de traitement d'une commande via QR code, depuis la lecture du token jusqu'à la livraison des produits et la mise à jour du système.

## Architecture des Données

### Structure OrderData
```cpp
typedef struct {
  char order_id[32];           // ID unique de la commande
  char machine_id[32];         // ID de la machine distributrice
  char timestamp[32];          // Horodatage de la commande
  char status[16];             // Statut: ACTIVE, DELIVERED, etc.
  OrderItem items[10];         // Jusqu'à 10 items par commande
  int item_count;              // Nombre d'items
  bool is_valid;              // Validation de la structure
} OrderData;

typedef struct {
  char product_id[32];         // ID du produit
  int slot_number;             // Numéro du slot (1-99)
  int quantity;                // Quantité à livrer (1-10)
} OrderItem;
```

## Workflow Détaillé

### 1. Lecture et Détection QR
```
Scanner QR (UART2) → Service QR → Événement ORCH_EVT_QR_TOKEN_READ
```
- Le scanner envoie le token via UART2 (GPIO 16/17)
- Le service QR détecte le pattern `qr_*_*`
- Envoi immédiat de l'événement à l'orchestrateur

### 2. Validation du Token
```
Orchestrateur → HTTP Service → API Backend
```
**Endpoint** : `POST /api/order-validation/validate-token`
**Payload** :
```json
{
  "qr_code_token": "qr_6565917e-a288-4d69-8693-b55e19ea8ba8_1755260515417"
}
```

### 3. Réception des Données de Commande
**Réponse API (200 OK)** :
```json
{
  "order_id": "order_123456789",
  "machine_id": "machine_123456789",
  "timestamp": "2024-01-15T10:30:00.000Z",
  "status": "ACTIVE",
  "items": [
    {
      "product_id": "prod_123456789",
      "slot_number": 1,
      "quantity": 2
    },
    {
      "product_id": "prod_987654321", 
      "slot_number": 3,
      "quantity": 1
    }
  ]
}
```

### 4. Parsing et Stockage
```
OrderManager::ParseOrderFromJSON() → Validation → Stockage en mémoire
```
- Parsing JSON avec ArduinoJson
- Validation des champs obligatoires
- Stockage dans `OrderManager::current_order`

### 5. Génération des Commandes de Livraison
```
OrderManager::GenerateDeliveryCommands() → "VEND slot qty product_id;..."
```
**Exemple de commande générée** :
```
ORDER_START:VEND 1 2 prod_123456789;VEND 3 1 prod_987654321
```

### 6. Communication avec NUCLEO
```
ESP32 (UART1) → NUCLEO : "ORDER_START:..."
NUCLEO → ESP32 : "DELIVERY_COMPLETED" ou "DELIVERY_FAILED"
```
- Envoi des commandes via UART1 (GPIO 25/26)
- Attente de confirmation de livraison
- Gestion des événements `ORCH_EVT_DELIVERY_COMPLETED/FAILED`

### 7. Mise à Jour des Quantités de Stock
```
Orchestrateur → HTTP Service → API Backend
```
**Endpoint** : `POST /api/stocks/update-quantity`
**Payload** :
```json
{
  "machine_id": "machine_123456789",
  "product_id": "prod_123456789",
  "quantity": 2,
  "slot_number": 1
}
```

### 8. Confirmation de Livraison
```
Orchestrateur → HTTP Service → API Backend
```
**Endpoint** : `POST /api/order-delivery/confirm`
**Payload** :
```json
{
  "order_id": "order_123456789",
  "machine_id": "machine_123456789", 
  "timestamp": "2024-01-15T10:30:00.000Z",
  "items_delivered": [
    {
      "product_id": "prod_123456789",
      "slot_number": 1,
      "quantity": 2
    },
    {
      "product_id": "prod_987654321",
      "slot_number": 3, 
      "quantity": 1
    }
  ]
}
```

### 9. Nettoyage
```
OrderManager::ClearCurrentOrder() → Libération mémoire
```
- Suppression des données de commande
- Retour à l'état `WORKFLOW_IDLE`

## États du Workflow

```cpp
enum OrderWorkflowState {
  WORKFLOW_IDLE = 0,                // Prêt pour nouvelle commande
  WORKFLOW_VALIDATING_TOKEN = 1,    // Validation en cours
  WORKFLOW_DELIVERING = 2,          // Livraison en cours  
  WORKFLOW_UPDATING_QUANTITIES = 3, // Mise à jour des quantités
  WORKFLOW_CONFIRMING_DELIVERY = 4, // Confirmation de livraison
  WORKFLOW_COMPLETED = 5            // Workflow terminé
};
```

## Gestion d'Erreurs

### Erreurs de Validation
- **Token invalide** → `QR_TOKEN_INVALID` vers NUCLEO
- **Pas de réseau** → `QR_TOKEN_NO_NETWORK` vers NUCLEO  
- **Workflow occupé** → `QR_TOKEN_BUSY` vers NUCLEO

### Erreurs de Livraison
- **Échec livraison** → `DELIVERY_FAILED` depuis NUCLEO
- **Timeout** → Nettoyage automatique après timeout
- **Erreur parsing** → `QR_TOKEN_ERROR` vers NUCLEO

### Erreurs API
- **Stock update failed** → Log erreur, nettoyage
- **Status update failed** → Log erreur, nettoyage
- **HTTP timeout** → Réessai automatique selon configuration

## Sécurité

### Validation des Données
- Vérification format UUID pour order_id
- Validation range slot_number (1-99)
- Validation range quantity (1-10)
- Vérification longueur product_id

### Communication Sécurisée
- TLS/HTTPS pour toutes les requêtes API
- Validation des certificats CA
- Rate limiting sur les requêtes
- Masquage des données sensibles dans les logs

## Monitoring et Debug

### Logs Principaux
```
[ORDER] Parsed order: order_123 (2 items) - VALID
[ORCH] Order validated, delivery commands sent to NUCLEO  
[ORCH] Stock updated successfully, updating order status
[ORCH] Order status updated to DELIVERED - Workflow completed!
```

### Métriques
- Temps total de traitement d'une commande
- Taux de succès des validations
- Taux de succès des livraisons
- Temps de réponse API

## Configuration

### Timeouts
- **Validation HTTP** : 10 secondes
- **Stock Update** : 10 secondes  
- **Status Update** : 10 secondes
- **Livraison NUCLEO** : Selon configuration NUCLEO

### Limites
- **Items par commande** : 10 maximum
- **Longueur order_id** : 32 caractères
- **Longueur product_id** : 32 caractères
- **Range slot_number** : 1-99
- **Range quantity** : 1-10

## Tests

### Test Token Valide
```bash
# Scanner ce token de test
qr_test-uuid-1234-5678-abcd_1735000000000
```

### Test Logs
```bash
# Surveiller les logs série
pio device monitor --port /dev/cu.usbserial-* --baud 115200
```

### Test API (curl)
```bash
curl -X POST https://iot-vending-machine.osc-fr1.scalingo.io/api/order-validation/validate-token \
  -H "Content-Type: application/json" \
  -d '{"qr_code_token": "qr_test-uuid_1735000000000"}'
```
