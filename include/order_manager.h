#pragma once

#include <Arduino.h>

// Constantes pour la gestion des commandes
#define MAX_ORDER_ITEMS 10
#define MAX_ORDER_ID_LENGTH 32
#define MAX_MACHINE_ID_LENGTH 32
#define MAX_PRODUCT_ID_LENGTH 32
#define MAX_TIMESTAMP_LENGTH 32

// Structure pour un item de commande
typedef struct {
  char product_id[MAX_PRODUCT_ID_LENGTH];
  int slot_number;
  int quantity;
} OrderItem;

// Structure pour une commande complète
typedef struct {
  char order_id[MAX_ORDER_ID_LENGTH];
  char machine_id[MAX_MACHINE_ID_LENGTH];
  char timestamp[MAX_TIMESTAMP_LENGTH];
  char status[16]; // ACTIVE, DELIVERED, etc.
  OrderItem items[MAX_ORDER_ITEMS];
  int item_count;
  bool is_valid;
} OrderData;

// Gestionnaire de commande globale
class OrderManager {
private:
  static OrderData current_order;
  static bool has_active_order;

public:
  // Gestion de la commande courante
  static bool ParseOrderFromJSON(const char* json_response, OrderData* order);
  static void SetCurrentOrder(const OrderData* order);
  static OrderData* GetCurrentOrder();
  static bool HasActiveOrder();
  static void ClearCurrentOrder();
  
  // Génération de commandes UART pour NUCLEO
  static String GenerateDeliveryCommands();
  static String GenerateStockUpdateData();
  
  // Validation
  static bool ValidateOrder(const OrderData* order);
  
  // Debug
  static void PrintOrderDetails(const OrderData* order);
};
