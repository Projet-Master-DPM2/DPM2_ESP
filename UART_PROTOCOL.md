# Protocole UART ESP32 ↔ NUCLEO - Livraison de Produits

## Vue d'ensemble

Ce document décrit le protocole de communication UART entre l'ESP32 et la NUCLEO pour la livraison de produits après validation d'un QR code de commande.

## Configuration UART

- **Port UART**: UART1 (HardwareSerial)
- **Baudrate**: 115200 bps
- **Format**: 8N1 (8 bits de données, pas de parité, 1 bit de stop)
- **Pins ESP32**: 
  - TX: GPIO 17 (UART_TX_PIN)
  - RX: GPIO 16 (UART_RX_PIN)

## Format des Commandes de Livraison

### Structure générale

```
ORDER_START:<order_id>
VEND <slot_number> <quantity> <product_id>
VEND <slot_number> <quantity> <product_id>
...
ORDER_END
```

### Détail des champs

- **order_id**: Identifiant unique de la commande (ex: "order_123456789")
- **slot_number**: Numéro du channel du multiplexeur (1-99)
  - Correspond au numéro de slot physique sur la machine
  - Channel 1 = Slot 1, Channel 2 = Slot 2, etc.
- **quantity**: Nombre d'unités à livrer (1-10)
- **product_id**: Identifiant du produit pour traçabilité (ex: "prod_123456789")

### Exemple de commande

```
ORDER_START:order_987654321
VEND 1 2 prod_123456789
VEND 3 1 prod_987654321
ORDER_END
```

**Interprétation**: Livrer 2 unités du produit "prod_123456789" depuis le slot 1, puis 1 unité du produit "prod_987654321" depuis le slot 3.

## Réponses de la NUCLEO

### Confirmations de commande

- **ORDER_ACK**: Commande reçue et acceptée
- **ORDER_NAK:<reason>**: Commande rejetée avec raison

### Statuts de livraison par item

- **VEND_COMPLETED:<slot_number>**: Livraison réussie pour le slot spécifié
- **VEND_FAILED:<slot_number>:<reason>**: Échec de livraison pour le slot avec raison

### Statuts de livraison globale

- **DELIVERY_COMPLETED**: Toute la commande a été livrée avec succès
- **DELIVERY_FAILED:<reason>**: Échec de livraison de la commande complète

### Exemples de réponses

```
ORDER_ACK
VEND_COMPLETED:1
VEND_COMPLETED:3
DELIVERY_COMPLETED
```

```
ORDER_NAK:SLOT_EMPTY
```

```
VEND_FAILED:1:MOTOR_ERROR
DELIVERY_FAILED:MOTOR_ERROR
```

## Workflow de Livraison

### 1. Validation QR Code (ESP32)
- ESP32 reçoit QR code token
- Validation via API backend
- Parsing des données de commande (order_id, items, etc.)

### 2. Envoi Commande (ESP32 → NUCLEO)
- Génération des commandes VEND pour chaque item
- Envoi via UART avec format structuré
- Attente de confirmation ORDER_ACK

### 3. Exécution Livraison (NUCLEO)
- Parsing des commandes VEND
- Contrôle des moteurs via multiplexeur
- Livraison séquentielle des items
- Retour des statuts VEND_COMPLETED/VEND_FAILED

### 4. Confirmation Finale (NUCLEO → ESP32)
- Envoi DELIVERY_COMPLETED ou DELIVERY_FAILED
- ESP32 met à jour le stock via API
- ESP32 met à jour le statut de commande

## Gestion d'Erreurs

### Erreurs de communication
- **ERR:RATE_LIMIT**: Trop de commandes envoyées
- **ERR:LINE_TOO_LONG**: Ligne UART trop longue
- **ERR:BAD_CHAR**: Caractères invalides détectés
- **ERR:UNKNOWN_CMD**: Commande non reconnue

### Erreurs de livraison
- **SLOT_EMPTY**: Slot vide (pas de produit)
- **MOTOR_ERROR**: Erreur moteur
- **SENSOR_ERROR**: Erreur capteur de stock
- **TIMEOUT**: Timeout de livraison

## Sécurité et Validation

### Validation des données
- Vérification de la longueur des lignes UART
- Validation des numéros de slot (1-99)
- Validation des quantités (1-10)
- Rate limiting des commandes

### Logging sécurisé
- Masquage des données sensibles dans les logs
- Logs détaillés pour debugging
- Traçabilité complète des commandes

## Intégration avec le Workflow

### États du workflow (ESP32)
1. **WORKFLOW_IDLE**: En attente de QR code
2. **WORKFLOW_VALIDATING_TOKEN**: Validation du token
3. **WORKFLOW_DELIVERING**: Livraison en cours
4. **WORKFLOW_UPDATING_STOCK**: Mise à jour du stock
5. **WORKFLOW_UPDATING_STATUS**: Mise à jour du statut
6. **WORKFLOW_COMPLETED**: Commande terminée

### Synchronisation
- L'ESP32 attend DELIVERY_COMPLETED avant de continuer
- En cas d'échec, nettoyage de la commande courante
- Retry automatique possible selon la politique d'erreur
