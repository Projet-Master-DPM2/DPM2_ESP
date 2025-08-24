#include "order_manager.h"
#include <ArduinoJson.h>

// Variables statiques
OrderData OrderManager::current_order = {};
bool OrderManager::has_active_order = false;

bool OrderManager::ParseOrderFromJSON(const char* json_response, OrderData* order) {
  if (!json_response || !order) return false;
  
  // Initialiser la structure
  memset(order, 0, sizeof(OrderData));
  order->is_valid = false;
  
  // Parser le JSON
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, json_response);
  
  if (error) {
    Serial.printf("[ORDER] JSON parse error: %s\n", error.c_str());
    return false;
  }
  
  // Extraire les champs principaux
  if (doc.containsKey("order_id")) {
    strncpy(order->order_id, doc["order_id"], MAX_ORDER_ID_LENGTH - 1);
  }
  
  if (doc.containsKey("machine_id")) {
    strncpy(order->machine_id, doc["machine_id"], MAX_MACHINE_ID_LENGTH - 1);
  }
  
  if (doc.containsKey("timestamp")) {
    strncpy(order->timestamp, doc["timestamp"], MAX_TIMESTAMP_LENGTH - 1);
  }
  
  if (doc.containsKey("status")) {
    strncpy(order->status, doc["status"], 15);
  }
  
  // Extraire les items
  if (doc.containsKey("items") && doc["items"].is<JsonArray>()) {
    JsonArray items = doc["items"];
    order->item_count = min((int)items.size(), MAX_ORDER_ITEMS);
    
    for (int i = 0; i < order->item_count; i++) {
      JsonObject item = items[i];
      
      if (item.containsKey("product_id")) {
        strncpy(order->items[i].product_id, item["product_id"], MAX_PRODUCT_ID_LENGTH - 1);
      }
      
      if (item.containsKey("slot_number")) {
        order->items[i].slot_number = item["slot_number"];
      }
      
      if (item.containsKey("quantity")) {
        order->items[i].quantity = item["quantity"];
      }
    }
  }
  
  // Valider la commande
  order->is_valid = ValidateOrder(order);
  
  Serial.printf("[ORDER] Parsed order: %s (%d items) - %s\n", 
                order->order_id, order->item_count, 
                order->is_valid ? "VALID" : "INVALID");
  
  return order->is_valid;
}

void OrderManager::SetCurrentOrder(const OrderData* order) {
  if (!order) return;
  
  memcpy(&current_order, order, sizeof(OrderData));
  has_active_order = order->is_valid;
  
  Serial.printf("[ORDER] Set current order: %s\n", current_order.order_id);
  PrintOrderDetails(&current_order);
}

OrderData* OrderManager::GetCurrentOrder() {
  return has_active_order ? &current_order : nullptr;
}

bool OrderManager::HasActiveOrder() {
  return has_active_order && current_order.is_valid;
}

void OrderManager::ClearCurrentOrder() {
  memset(&current_order, 0, sizeof(OrderData));
  has_active_order = false;
  Serial.println("[ORDER] Cleared current order");
}

String OrderManager::GenerateDeliveryCommands() {
  if (!has_active_order || !current_order.is_valid) {
    return "";
  }
  
  String commands = "";
  
  for (int i = 0; i < current_order.item_count; i++) {
    const OrderItem& item = current_order.items[i];
    
    // Format: VEND <slot_number> <quantity> <product_id>
    commands += "VEND " + String(item.slot_number) + " " + 
                String(item.quantity) + " " + String(item.product_id);
    
    if (i < current_order.item_count - 1) {
      commands += ";"; // Séparateur pour plusieurs commandes
    }
  }
  
  Serial.printf("[ORDER] Generated delivery commands: %s\n", commands.c_str());
  return commands;
}

String OrderManager::GenerateStockUpdateData() {
  if (!has_active_order || !current_order.is_valid) {
    return "";
  }
  
  // Générer un JSON pour la mise à jour du stock
  DynamicJsonDocument doc(1024);
  doc["order_id"] = current_order.order_id;
  doc["machine_id"] = current_order.machine_id;
  
  JsonArray items = doc.createNestedArray("items");
  for (int i = 0; i < current_order.item_count; i++) {
    JsonObject item = items.createNestedObject();
    item["product_id"] = current_order.items[i].product_id;
    item["slot_number"] = current_order.items[i].slot_number;
    item["quantity_delivered"] = current_order.items[i].quantity;
  }
  
  String json_string;
  serializeJson(doc, json_string);
  
  Serial.printf("[ORDER] Generated stock update data: %s\n", json_string.c_str());
  return json_string;
}

bool OrderManager::ValidateOrder(const OrderData* order) {
  if (!order) return false;
  
  // Vérifier les champs obligatoires
  if (strlen(order->order_id) == 0) {
    Serial.println("[ORDER] Validation failed: missing order_id");
    return false;
  }
  
  if (strlen(order->machine_id) == 0) {
    Serial.println("[ORDER] Validation failed: missing machine_id");
    return false;
  }
  
  if (order->item_count <= 0 || order->item_count > MAX_ORDER_ITEMS) {
    Serial.printf("[ORDER] Validation failed: invalid item_count (%d)\n", order->item_count);
    return false;
  }
  
  // Vérifier chaque item
  for (int i = 0; i < order->item_count; i++) {
    const OrderItem& item = order->items[i];
    
    if (strlen(item.product_id) == 0) {
      Serial.printf("[ORDER] Validation failed: missing product_id for item %d\n", i);
      return false;
    }
    
    if (item.slot_number <= 0 || item.slot_number > 99) {
      Serial.printf("[ORDER] Validation failed: invalid slot_number (%d) for item %d\n", 
                    item.slot_number, i);
      return false;
    }
    
    if (item.quantity <= 0 || item.quantity > 10) {
      Serial.printf("[ORDER] Validation failed: invalid quantity (%d) for item %d\n", 
                    item.quantity, i);
      return false;
    }
  }
  
  // Vérifier le statut
  if (strcmp(order->status, "ACTIVE") != 0) {
    Serial.printf("[ORDER] Warning: order status is '%s', expected 'ACTIVE'\n", order->status);
  }
  
  return true;
}

void OrderManager::PrintOrderDetails(const OrderData* order) {
  if (!order) return;
  
  Serial.println("[ORDER] === Order Details ===");
  Serial.printf("Order ID: %s\n", order->order_id);
  Serial.printf("Machine ID: %s\n", order->machine_id);
  Serial.printf("Timestamp: %s\n", order->timestamp);
  Serial.printf("Status: %s\n", order->status);
  Serial.printf("Items (%d):\n", order->item_count);
  
  for (int i = 0; i < order->item_count; i++) {
    const OrderItem& item = order->items[i];
    Serial.printf("  [%d] Product: %s, Slot: %d, Qty: %d\n", 
                  i, item.product_id, item.slot_number, item.quantity);
  }
  
  Serial.printf("Valid: %s\n", order->is_valid ? "YES" : "NO");
  Serial.println("[ORDER] === End Details ===");
}
