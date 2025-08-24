# Confirmation de Livraison - DPM2 ESP32

## Vue d'ensemble

Ce document décrit l'implémentation de la confirmation de livraison au backend après une livraison réussie de produits par la NUCLEO.

## Workflow de Confirmation

### 1. **Séquence des étapes**

```
QR Code → Validation → Livraison NUCLEO → Confirmation → Mise à jour Stock → Mise à jour Statut
```

### 2. **États du workflow**

- `WORKFLOW_IDLE` : En attente de QR code
- `WORKFLOW_VALIDATING_TOKEN` : Validation du token QR
- `WORKFLOW_DELIVERING` : Livraison en cours par NUCLEO
- `WORKFLOW_CONFIRMING_DELIVERY` : **NOUVEAU** - Confirmation de livraison au backend
- `WORKFLOW_UPDATING_STOCK` : Mise à jour du stock
- `WORKFLOW_UPDATING_STATUS` : Mise à jour du statut de commande
- `WORKFLOW_COMPLETED` : Commande terminée

## Endpoint de Confirmation

### **URL**
```
POST https://iot-vending-machine.osc-fr1.scalingo.io/api/order-delivery/confirm
```

### **Body JSON**
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

### **Réponse attendue**
- **200 OK** : Confirmation réussie
- **4xx/5xx** : Erreur de confirmation

## Implémentation Technique

### 1. **Service HTTP**

#### **Nouvelle fonction**
```cpp
bool HttpService_ConfirmDelivery(const char* orderId, const char* machineId, 
                                const char* timestamp, const char* itemsDeliveredJson, 
                                QueueHandle_t responseQueue, uint32_t timeoutMs);
```

#### **Utilisation**
```cpp
// Dans l'orchestrateur après livraison réussie
String deliveryData = OrderManager::GenerateDeliveryConfirmationData();
HttpService_ConfirmDelivery(order->order_id, order->machine_id, 
                           order->timestamp, deliveryData.c_str(), 
                           httpResponseQueue, 10000);
```

### 2. **OrderManager**

#### **Nouvelle fonction**
```cpp
String OrderManager::GenerateDeliveryConfirmationData();
```

#### **Génération du JSON**
```cpp
DynamicJsonDocument doc(1024);
doc["order_id"] = current_order.order_id;
doc["machine_id"] = current_order.machine_id;
doc["timestamp"] = current_order.timestamp;

JsonArray items = doc.createNestedArray("items_delivered");
for (int i = 0; i < current_order.item_count; i++) {
  JsonObject item = items.createNestedObject();
  item["product_id"] = current_order.items[i].product_id;
  item["slot_number"] = current_order.items[i].slot_number;
  item["quantity"] = current_order.items[i].quantity;
}
```

### 3. **Configuration d'environnement**

#### **Nouvel endpoint dans `.env`**
```bash
API_DELIVERY_CONFIRM_ENDPOINT=/api/order-delivery/confirm
```

#### **Fonctions de configuration**
```cpp
String EnvConfig::GetDeliveryConfirmUrl();
const char* EnvConfig::GetDeliveryConfirmEndpoint();
```

## Séquence d'Exécution

### 1. **Réception de `DELIVERY_COMPLETED`**
```cpp
case ORCH_EVT_DELIVERY_COMPLETED:
  if (currentWorkflowState == WORKFLOW_DELIVERING) {
    // Générer les données de confirmation
    String deliveryData = OrderManager::GenerateDeliveryConfirmationData();
    // Envoyer la confirmation
    HttpService_ConfirmDelivery(...);
    currentWorkflowState = WORKFLOW_CONFIRMING_DELIVERY;
  }
```

### 2. **Traitement de la réponse**
```cpp
case WORKFLOW_CONFIRMING_DELIVERY:
  if (httpResp.statusCode == 200) {
    // Confirmation réussie, continuer avec mise à jour du stock
    String stockData = OrderManager::GenerateStockUpdateData();
    HttpService_UpdateStock(stockData.c_str(), ...);
    currentWorkflowState = WORKFLOW_UPDATING_STOCK;
  } else {
    // Échec de confirmation, nettoyer
    currentWorkflowState = WORKFLOW_IDLE;
    OrderManager::ClearCurrentOrder();
  }
```

## Gestion d'Erreurs

### **Erreurs possibles**
1. **Échec de génération des données** : Impossible de créer le JSON de confirmation
2. **Échec de requête HTTP** : Problème réseau ou serveur
3. **Réponse d'erreur du backend** : Statut 4xx/5xx

### **Actions de récupération**
- Nettoyage de la commande courante
- Retour à l'état `WORKFLOW_IDLE`
- Logs détaillés pour debugging

## Logs et Debugging

### **Messages de log**
```
[HTTP] Confirming delivery for order: order_123456789
[HTTP] Machine: machine_123456789, Timestamp: 2024-01-15T10:30:00.000Z
[HTTP] Items delivered: [{"product_id":"prod_123","slot_number":1,"quantity":2}]
[HTTP] Using endpoint: /api/order-delivery/confirm

[ORCH] Delivery confirmation request sent
[ORCH] Delivery confirmed successfully, updating stock
```

### **Validation des données**
- Vérification de la présence de tous les champs obligatoires
- Validation du format JSON généré
- Contrôle de la taille des données

## Intégration avec le Workflow Existant

### **Compatibilité**
- ✅ Compatible avec le workflow existant
- ✅ Pas d'impact sur les autres étapes
- ✅ Gestion d'erreurs cohérente

### **Avantages**
- **Traçabilité complète** : Confirmation explicite de livraison
- **Données détaillées** : Informations complètes sur les items livrés
- **Séparation des responsabilités** : Confirmation distincte de la mise à jour de stock

## Tests et Validation

### **Tests recommandés**
1. **Test de génération JSON** : Vérifier le format des données
2. **Test d'envoi HTTP** : Valider la requête et la réponse
3. **Test d'intégration** : Workflow complet avec confirmation
4. **Test d'erreur** : Gestion des échecs de confirmation

### **Validation des données**
```cpp
// Exemple de validation
if (strlen(order->order_id) == 0 || strlen(order->machine_id) == 0) {
  Serial.println("[ORDER] Validation failed: missing required fields");
  return false;
}
```

## Configuration et Déploiement

### **Mise à jour du fichier `.env`**
```bash
# Ajouter la nouvelle ligne
API_DELIVERY_CONFIRM_ENDPOINT=/api/order-delivery/confirm
```

### **Upload vers ESP32**
```bash
python scripts/upload_env.py
```

### **Vérification**
```cpp
// Dans le code de démarrage
EnvConfig::PrintConfig();
// Doit afficher : Delivery: /api/order-delivery/confirm
```
