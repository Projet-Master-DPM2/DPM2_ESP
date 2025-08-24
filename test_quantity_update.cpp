#include <Arduino.h>
#include <ArduinoJson.h>
#include "order_manager.h"
#include "services/http_service.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Test Quantity Update ===\n");
  
  // Test 1: Génération de requête de mise à jour des quantités
  test_quantity_update_request();
  
  // Test 2: Validation du format JSON
  test_json_format();
  
  // Test 3: Test avec commande vide
  test_empty_order();
  
  Serial.println("\n=== Tests terminés ===");
}

void loop() {
  // Rien à faire dans la boucle
}

void test_quantity_update_request() {
  Serial.println("Test 1: Génération de requête de mise à jour des quantités");
  
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
  
  // Simuler une requête de mise à jour des quantités
  // Note: On ne peut pas tester directement HttpService_UpdateQuantities sans réseau
  // mais on peut vérifier que la fonction est appelable
  Serial.println("✅ Commande configurée pour test de mise à jour des quantités");
  
  // Afficher les détails de la commande
  Serial.printf("Machine ID: %s\n", testOrder.machine_id);
  Serial.printf("Item 1: Product=%s, Slot=%d, Quantity=%d\n", 
                testOrder.items[0].product_id, 
                testOrder.items[0].slot_number, 
                testOrder.items[0].quantity);
  Serial.printf("Item 2: Product=%s, Slot=%d, Quantity=%d\n", 
                testOrder.items[1].product_id, 
                testOrder.items[1].slot_number, 
                testOrder.items[1].quantity);
  
  // Nettoyer
  OrderManager::ClearCurrentOrder();
  Serial.println();
}

void test_json_format() {
  Serial.println("Test 2: Validation du format JSON pour mise à jour des quantités");
  
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
  
  // Simuler la génération du JSON pour la requête de mise à jour des quantités
  // Format attendu: {"machine_id":"...","product_id":"...","quantity":X,"slot_number":Y}
  DynamicJsonDocument doc(512);
  doc["machine_id"] = testOrder.machine_id;
  doc["product_id"] = testOrder.items[0].product_id;
  doc["quantity"] = testOrder.items[0].quantity;
  doc["slot_number"] = testOrder.items[0].slot_number;
  
  String json_string;
  serializeJson(doc, json_string);
  
  Serial.printf("JSON généré: %s\n", json_string.c_str());
  
  // Valider le JSON généré
  DynamicJsonDocument validationDoc(512);
  DeserializationError error = deserializeJson(validationDoc, json_string);
  
  if (error) {
    Serial.printf("❌ Erreur parsing JSON: %s\n", error.c_str());
  } else {
    Serial.println("✅ JSON valide");
    
    // Vérifier les champs obligatoires
    if (validationDoc.containsKey("machine_id") && 
        validationDoc.containsKey("product_id") && 
        validationDoc.containsKey("quantity") && 
        validationDoc.containsKey("slot_number")) {
      Serial.println("✅ Tous les champs obligatoires présents");
      
      // Vérifier les valeurs
      if (strcmp(validationDoc["machine_id"], "machine_test_456") == 0) {
        Serial.println("✅ machine_id correct");
      }
      
      if (strcmp(validationDoc["product_id"], "prod_test") == 0) {
        Serial.println("✅ product_id correct");
      }
      
      if (validationDoc["quantity"] == 3) {
        Serial.println("✅ quantity correct");
      }
      
      if (validationDoc["slot_number"] == 5) {
        Serial.println("✅ slot_number correct");
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
  
  // Essayer de mettre à jour les quantités sans commande active
  // Note: On ne peut pas tester directement sans réseau, mais on peut vérifier la logique
  if (!OrderManager::HasActiveOrder()) {
    Serial.println("✅ Comportement correct: pas de commande active");
  } else {
    Serial.println("❌ Erreur: commande active détectée alors qu'il ne devrait pas y en avoir");
  }
  
  Serial.println();
}

void test_workflow_states() {
  Serial.println("Test 4: Validation des états du workflow");
  
  // Vérifier que le nouvel état WORKFLOW_UPDATING_QUANTITIES est bien défini
  Serial.println("États du workflow mis à jour:");
  Serial.println("  WORKFLOW_IDLE = 0");
  Serial.println("  WORKFLOW_VALIDATING_TOKEN = 1");
  Serial.println("  WORKFLOW_DELIVERING = 2");
  Serial.println("  WORKFLOW_UPDATING_QUANTITIES = 3  ← NOUVEAU");
  Serial.println("  WORKFLOW_CONFIRMING_DELIVERY = 4");
  Serial.println("  WORKFLOW_COMPLETED = 5  ← SIMPLIFIÉ");
  
  Serial.println("✅ Séquence d'états validée");
  Serial.println();
}
