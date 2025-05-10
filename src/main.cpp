#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Hello depuis PlatformIO !");
}

void loop() {
  delay(1000);
  Serial.println("Tick...");
}