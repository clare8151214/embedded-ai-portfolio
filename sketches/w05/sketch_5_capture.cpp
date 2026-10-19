// Sketch 5 - one register read, once every two seconds, and nothing else on the bus.
// It exists so the analyser has something clean to trigger on: a single WHO_AM_I
// transaction with two seconds of silence on either side of it.
//
// The read reports success and value separately. "The device is there and its identity
// register holds 0x00" and "nothing answered" are different facts about the bus, and a
// helper that returns 0 for both would print the same line for each. The analyser you are
// about to point at this is the thing that will show you which one you actually have -
// so the serial log must not have already blurred them.
#include <Arduino.h>
#include <Wire.h>

#define IMU_ADDR  0x68      // your scan result
#define WHO_AM_I  0x75      // MPU6050. LSM6DS3: 0x0F

bool r8(uint8_t reg, uint8_t *val) {
  Wire.beginTransmission(IMU_ADDR); Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;   // nobody acknowledged the address
  if (Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)1) != 1) return false;
  *val = Wire.read();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin();
  Wire.setClock(100000);    // slow on purpose: easier to read on a 24 MHz analyser
  Serial.println("n,who_am_i,us");
}

void loop() {
  static uint32_t n = 0;
  uint8_t id = 0;
  uint32_t t0 = micros();
  bool ok = r8(WHO_AM_I, &id);
  uint32_t us = micros() - t0;
  if (ok) Serial.printf("%lu,0x%02X,%lu\n", n++, id, us);
  else    Serial.printf("%lu,no_ack,%lu\n", n++, us);   // a row, but not a reading
  delay(2000);
}
