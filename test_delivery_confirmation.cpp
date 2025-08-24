#include <Arduino.h>
#include <ArduinoJson.h>
#include "order_manager.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Test Delivery Confirmation ===\n");
  
  // Test 1: Génération de données de confirmation
  test_generate_delivery_confirmation();
  
  // Test 2: Validation du format JSON
  test_json_format();
  
  // Test 3: Test avec commande vide
  test_empty_order();
  
  Serial.println("\n=== Tests terminés ===");
}

void loop() {
  // Rien à faire dans la boucle
}

void test_generate_delivery_confirmation() {
  Serial.println("Test 1: Génération de données de confirmation");
  
  // Créer une commande de test
  OrderData testOrder;
  strcpy(testOrder.order_id, "order_123456789");
  strcpy(testOrder.machine_id, "machine_987654321");
  strcpy(testOrder.timestamp, "2024-01-15T10:30:00.000Z");
  strcpy(testOrder.status, "ACTIVE");
  testOrder.item_count = 2;
  testOrder.is_valid = true;
  
  // Premier item
  strcpy(testOrder.items[0].product_id, "prod_coca_cola");
  testOrder.items[0].slot_number = 1;
  testOrder.items[0].quantity = 2;
  
  // Deuxième item
  strcpy(testOrder.items[1].product_id, "prod_sprite");
  testOrder.items[1].slot_number = 3;
  testOrder.items[1].quantity = 1;
  
  // Définir comme commande courante
  OrderManager::SetCurrentOrder(&testOrder);
  
  // Générer les données de confirmation
  String confirmationData = OrderManager::GenerateDeliveryConfirmationData();
  
  Serial.printf("Données générées: %s\n", confirmationData.c_str());
  
  // Vérifier que les données ne sont pas vides
  if (confirmationData.length() > 0) {
    Serial.println("✅ Données générées avec succès");
  } else {
    Serial.println("❌ Échec de génération des données");
  }
  
  // Nettoyer
  OrderManager::ClearCurrentOrder();
  Serial.println();
}

void test_json_format() {
  Serial.println("Test 2: Validation du format JSON");
  
  // Créer une commande de test
  OrderData testOrder;
  strcpy(testOrder.order_id, "order_test_123");
  strcpy(testOrder.machine_id, "machine_test_456");
  strcpy(testOrder.timestamp, "2024-01-15T10:30:00.000Z");
  strcpy(testOrder.status, "ACTIVE");
  testOrder.item_count = 1;
  testOrder.is_valid = true;
  
  // Un seul item pour simplifier
  strcpy(testOrder.items[0].product_id, "prod_test");
  testOrder.items[0].slot_number = 5;
  testOrder.items[0].quantity = 3;
  
  // Définir comme commande courante
  OrderManager::SetCurrentOrder(&testOrder);
  
  // Générer les données
  String confirmationData = OrderManager::GenerateDeliveryConfirmationData();
  
  // Parser le JSON pour validation
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, confirmationData);
  
  if (error) {
    Serial.printf("❌ Erreur parsing JSON: %s\n", error.c_str());
  } else {
    Serial.println("✅ JSON valide");
    
    // Vérifier les champs obligatoires
    if (doc.containsKey("order_id") && 
        doc.containsKey("machine_id") && 
        doc.containsKey("timestamp") && 
        doc.containsKey("items_delivered")) {
      Serial.println("✅ Tous les champs obligatoires présents");
      
      // Vérifier les valeurs
      if (strcmp(doc["order_id"], "order_test_123") == 0) {
        Serial.println("✅ order_id correct");
      }
      
      if (strcmp(doc["machine_id"], "machine_test_456") == 0) {
        Serial.println("✅ machine_id correct");
      }
      
      if (strcmp(doc["timestamp"], "2024-01-15T10:30:00.000Z") == 0) {
        Serial.println("✅ timestamp correct");
      }
      
      // Vérifier le tableau items_delivered
      JsonArray items = doc["items_delivered"];
      if (items.size() == 1) {
        Serial.println("✅ Nombre d'items correct");
        
        JsonObject item = items[0];
        if (strcmp(item["product_id"], "prod_test") == 0 &&
            item["slot_number"] == 5 &&
            item["quantity"] == 3) {
          Serial.println("✅ Données d'item correctes");
        }
      }
    } else {
      Serial.println("❌ Champs manquants dans le JSON");
    }
  }
  
  // Nettoyer
  OrderManager::ClearCurrentOrder();
  Serial.println();
}

void test_empty_order() {
  Serial.println("Test 3: Test avec commande vide");
  
  // S'assurer qu'il n'y a pas de commande active
  OrderManager::ClearCurrentOrder();
  
  // Essayer de générer des données
  String confirmationData = OrderManager::GenerateDeliveryConfirmationData();
  
  if (confirmationData.length() == 0) {
    Serial.println("✅ Comportement correct: données vides sans commande");
  } else {
    Serial.println("❌ Erreur: données générées sans commande active");
  }
  
  Serial.println();
}
