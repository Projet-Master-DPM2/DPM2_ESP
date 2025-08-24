# Mise à Jour des Quantités de Stock - DPM2 ESP32

## Vue d'ensemble

Ce document décrit l'implémentation de la mise à jour des quantités de stock après une livraison réussie de produits par la NUCLEO, **avant** la confirmation de livraison au backend.

## Workflow Mis à Jour

### **Nouvelle séquence des étapes**

```
QR Code → Validation → Livraison NUCLEO → Mise à jour Quantités → Confirmation → Nettoyage
```

### **États du workflow mis à jour**

- `WORKFLOW_IDLE` : En attente de QR code
- `WORKFLOW_VALIDATING_TOKEN` : Validation du token QR
- `WORKFLOW_DELIVERING` : Livraison en cours par NUCLEO
- `WORKFLOW_UPDATING_QUANTITIES` : **NOUVEAU** - Mise à jour des quantités de stock
- `WORKFLOW_CONFIRMING_DELIVERY` : Confirmation de livraison au backend
- `WORKFLOW_COMPLETED` : Commande terminée (nettoyage automatique)

## Endpoint de Mise à Jour des Quantités

### **URL**
```
POST https://iot-vending-machine.osc-fr1.scalingo.io/api/stocks/update-quantity
```

### **Body JSON**
```json
{
  "machine_id": "machine_123456789",
  "product_id": "prod_123456789",
  "quantity": 2,
  "slot_number": 1
}
```

### **Réponse attendue**
- **200 OK** : Mise à jour des quantités réussie
- **4xx/5xx** : Erreur de mise à jour

## Implémentation Technique

### 1. **Service HTTP**

#### **Nouvelle fonction**
```cpp
bool HttpService_UpdateQuantities(const char* machineId, const char* productId, 
                                int quantity, int slotNumber, 
                                QueueHandle_t responseQueue, uint32_t timeoutMs);
```

#### **Utilisation**
```cpp
// Dans l'orchestrateur après livraison réussie
OrderManager::UpdateAllQuantities(httpResponseQueue, 10000);
```

### 2. **OrderManager**

#### **Nouvelle fonction**
```cpp
bool OrderManager::UpdateAllQuantities(QueueHandle_t responseQueue, uint32_t timeoutMs);
```

#### **Logique de mise à jour**
```cpp
// Pour chaque item de la commande, envoyer une requête de mise à jour
// Note: Actuellement implémenté pour le premier item uniquement
// TODO: Étendre pour traiter tous les items séquentiellement
if (current_order.item_count > 0) {
  const OrderItem& item = current_order.items[0];
  return HttpService_UpdateQuantities(current_order.machine_id, 
                                     item.product_id, 
                                     item.quantity, 
                                     item.slot_number, 
                                     responseQueue, 
                                     timeoutMs);
}
```

### 3. **Configuration d'environnement**

#### **Nouvel endpoint dans `.env`**
```bash
API_UPDATE_QUANTITIES_ENDPOINT=/api/stocks/update-quantity
```

#### **Fonctions de configuration**
```cpp
String EnvConfig::GetUpdateQuantitiesUrl();
const char* EnvConfig::GetUpdateQuantitiesEndpoint();
```

## Séquence d'Exécution

### 1. **Réception de `DELIVERY_COMPLETED`**
```cpp
case ORCH_EVT_DELIVERY_COMPLETED:
  if (currentWorkflowState == WORKFLOW_DELIVERING) {
    // Mettre à jour les quantités de stock
    if (OrderManager::UpdateAllQuantities(httpResponseQueue, 10000)) {
      currentWorkflowState = WORKFLOW_UPDATING_QUANTITIES;
      Serial.println("[ORCH] Quantity update request sent");
    }
  }
```

### 2. **Traitement de la réponse de mise à jour des quantités**
```cpp
case WORKFLOW_UPDATING_QUANTITIES:
  if (httpResp.statusCode == 200) {
    // Quantités mises à jour avec succès, confirmer la livraison
    OrderData* order = OrderManager::GetCurrentOrder();
    if (order) {
      String deliveryData = OrderManager::GenerateDeliveryConfirmationData();
      HttpService_ConfirmDelivery(order->order_id, order->machine_id, 
                                 order->timestamp, deliveryData.c_str(), 
                                 httpResponseQueue, 10000);
      currentWorkflowState = WORKFLOW_CONFIRMING_DELIVERY;
    }
  } else {
    // Échec de mise à jour des quantités, nettoyer
    currentWorkflowState = WORKFLOW_IDLE;
    OrderManager::ClearCurrentOrder();
  }
```

## Différences avec l'Ancien Workflow

### **Avant (ancien workflow)**
```
Livraison → Confirmation → Mise à jour Stock → Mise à jour Statut
```

### **Après (nouveau workflow)**
```
Livraison → Mise à jour Quantités → Confirmation → Nettoyage
```

### **Avantages de la nouvelle approche**
- **Séparation des responsabilités** : Mise à jour des quantités distincte de la confirmation
- **Traçabilité améliorée** : Chaque étape est clairement identifiée
- **Gestion d'erreurs granulaire** : Possibilité de gérer les erreurs à chaque étape
- **Conformité API** : Respect des endpoints spécifiques du backend

## Gestion d'Erreurs

### **Erreurs possibles**
1. **Échec de requête HTTP** : Problème réseau ou serveur
2. **Réponse d'erreur du backend** : Statut 4xx/5xx
3. **Données manquantes** : Impossible de récupérer les informations de commande

### **Actions de récupération**
- Nettoyage de la commande courante
- Retour à l'état `WORKFLOW_IDLE`
- Logs détaillés pour debugging

## Logs et Debugging

### **Messages de log**
```
[HTTP] Updating quantities for product: prod_123456789
[HTTP] Machine: machine_123456789, Slot: 1, Quantity: 2
[HTTP] Using endpoint: /api/stocks/update-quantity

[ORCH] Quantity update request sent
[ORCH] Quantities updated successfully, confirming delivery
```

### **Validation des données**
- Vérification de la présence de tous les champs obligatoires
- Validation des valeurs numériques (quantity, slot_number)
- Contrôle de la taille des données

## Limitations Actuelles

### **Implémentation actuelle**
- **Un seul item** : Seul le premier item de la commande est traité
- **Séquentiel** : Pas de traitement parallèle des items

### **Améliorations futures**
- **Traitement multi-items** : Gérer tous les items d'une commande
- **File de requêtes** : Implémenter une file pour traiter les items séquentiellement
- **Retry automatique** : Réessayer en cas d'échec temporaire

## Tests et Validation

### **Tests recommandés**
1. **Test de requête HTTP** : Valider la requête et la réponse
2. **Test d'intégration** : Workflow complet avec mise à jour des quantités
3. **Test d'erreur** : Gestion des échecs de mise à jour
4. **Test multi-items** : Validation avec plusieurs items (futur)

### **Validation des données**
```cpp
// Exemple de validation
if (strlen(machineId) == 0 || strlen(productId) == 0) {
  Serial.println("[HTTP] Validation failed: missing required fields");
  return false;
}

if (quantity <= 0 || slotNumber <= 0) {
  Serial.println("[HTTP] Validation failed: invalid numeric values");
  return false;
}
```

## Configuration et Déploiement

### **Mise à jour du fichier `.env`**
```bash
# Ajouter la nouvelle ligne
API_UPDATE_QUANTITIES_ENDPOINT=/api/stocks/update-quantity
```

### **Upload vers ESP32**
```bash
python scripts/upload_env.py
```

### **Vérification**
```cpp
// Dans le code de démarrage
EnvConfig::PrintConfig();
// Doit afficher : Quantities: /api/stocks/update-quantity
```

## Intégration avec le Workflow Existant

### **Compatibilité**
- ✅ Compatible avec le workflow existant
- ✅ Pas d'impact sur les autres étapes
- ✅ Gestion d'erreurs cohérente

### **Impact sur les performances**
- **Latence** : Ajout d'une requête HTTP supplémentaire
- **Fiabilité** : Amélioration de la traçabilité
- **Maintenance** : Séparation claire des responsabilités

## Exemple de Workflow Complet

### **Séquence d'exécution typique**
1. **QR Code scanné** → `WORKFLOW_VALIDATING_TOKEN`
2. **Token validé** → `WORKFLOW_DELIVERING`
3. **Livraison NUCLEO** → `WORKFLOW_UPDATING_QUANTITIES`
4. **Quantités mises à jour** → `WORKFLOW_CONFIRMING_DELIVERY`
5. **Livraison confirmée** → `WORKFLOW_COMPLETED`
6. **Nettoyage automatique** → `WORKFLOW_IDLE`

Cette implémentation garantit une traçabilité complète et une séparation claire des responsabilités à chaque étape du processus de livraison.
