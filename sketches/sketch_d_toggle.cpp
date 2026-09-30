// Sketch D - the W02 debt, settled with the same clock you have been using all day.
#include <Arduino.h>
#include "soc/gpio_struct.h"     // ESP32-S3 GPIO register definitions
#define PIN 4                  // silkscreen 4 IS chip GPIO4 on this board
#define LOOPS 100000UL

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  // Wait for the native USB monitor before this one-shot measurement.
  while (!Serial) { delay(10); }
#endif
  delay(2000);
  pinMode(PIN, OUTPUT);

  uint32_t t0 = micros();                          // --- L2: Arduino API ---
  for (uint32_t i = 0; i < LOOPS; i++) { digitalWrite(PIN, HIGH); digitalWrite(PIN, LOW); }
  uint32_t dt_l2 = micros() - t0;

  t0 = micros();                                   // --- L3: register write ---
  for (uint32_t i = 0; i < LOOPS; i++) { GPIO.out_w1ts = (1UL << PIN); GPIO.out_w1tc = (1UL << PIN); }
  uint32_t dt_l3 = micros() - t0;

  Serial.printf("L2 digitalWrite : %lu us for %lu toggles -> %.2f kHz\n",
                dt_l2, LOOPS, LOOPS / (dt_l2 / 1000.0));
  Serial.printf("L3 register     : %lu us for %lu toggles -> %.2f kHz\n",
                dt_l3, LOOPS, LOOPS / (dt_l3 / 1000.0));
  Serial.flush();
}

void loop() {}
