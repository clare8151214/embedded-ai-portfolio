// AI-generated simple example: nominal 250 Hz using delay().
// The full loop period also includes ADC and serial-output time.
#include <Arduino.h>

constexpr int ECG_PIN = A0;
constexpr unsigned int SAMPLE_RATE_HZ = 250;
constexpr unsigned long SAMPLE_INTERVAL_MS = 1000UL / SAMPLE_RATE_HZ;

void setup() {
  Serial.begin(115200);
  pinMode(ECG_PIN, INPUT);
}

void loop() {
  int value = analogRead(ECG_PIN);
  Serial.println(value);
  delay(SAMPLE_INTERVAL_MS);
}
