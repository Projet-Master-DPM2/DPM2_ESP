#pragma once

#include <Arduino.h>

// Types d'erreurs de supervision
enum SupervisionErrorType {
  SUPERVISION_ERROR_WATCHDOG_TIMEOUT = 0,
  SUPERVISION_ERROR_WATCHDOG_RESET = 1,
  SUPERVISION_ERROR_TASK_HANG = 2,
  SUPERVISION_ERROR_MEMORY_LOW = 3,
  SUPERVISION_ERROR_NETWORK_DISCONNECTED = 4,
  SUPERVISION_ERROR_CRITICAL_SERVICE_FAILURE = 5,
  SUPERVISION_ERROR_HARDWARE_FAULT = 6,
  SUPERVISION_ERROR_SYSTEM_CRASH = 7
};

// Structure pour les événements de supervision
struct SupervisionEvent {
  String error_id;
  String machine_id;
  SupervisionErrorType error_type;
  String message;
  unsigned long timestamp;
};

// API du service de supervision
class SupervisionService {
public:
  static void Initialize();
  static void SendErrorNotification(SupervisionErrorType error_type, const String& message);
  static void SendErrorNotification(const SupervisionEvent& event);
  static String GenerateErrorId();
  static String GetMachineId();
  static void SetMachineId(const String& machine_id);
  
private:
  static String machine_id;
  static bool is_initialized;
  static unsigned long last_notification_time;
  static const unsigned long NOTIFICATION_COOLDOWN_MS = 30000; // 30 secondes
  
  static String ErrorTypeToString(SupervisionErrorType error_type);
  static bool ShouldSendNotification();
};
