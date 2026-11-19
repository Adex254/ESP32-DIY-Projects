
#include <Arduino.h>

// ESP32 PASSIVE BUZZER TEST
// Arduino ESP32 Core 3.0.7

#define BUZZER_PIN 25

void setup() {
  Serial.begin(115200);

  // Configure PWM for passive buzzer
  bool ready = ledcAttach(
    BUZZER_PIN,
    2000,
    8
  );

  if (!ready) {
    Serial.println("PWM FAILED");
    return;
  }

  Serial.println("BUZZER TEST STARTED");
}

void loop() {

  // First tone: 1000 Hz
  ledcWriteTone(BUZZER_PIN, 1000);
  delay(500);

  // Silence
  ledcWriteTone(BUZZER_PIN, 0);
  delay(500);

  // Second tone: 2000 Hz
  ledcWriteTone(BUZZER_PIN, 2000);
  delay(500);

  // Silence
  ledcWriteTone(BUZZER_PIN, 0);
  delay(500);

  // Third tone: 3000 Hz
  ledcWriteTone(BUZZER_PIN, 3000);
  delay(500);

  // Silence before repeating
  ledcWriteTone(BUZZER_PIN, 0);
  delay(1500);
}
