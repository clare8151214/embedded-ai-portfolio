// Sketch 6 - 104 Hz is the rate you wrote on the record sheet. That is a target.
// This measures what your program can actually get off the bus, which is a different thing.
//
// A transaction that the device never answered is not a read. This sketch counts
// attempts and successes separately, and refuses to print a rate when any attempt
// failed - because a number computed from failed transactions looks exactly like a
// number computed from real ones, and that is the whole point of this course.
#include <Arduino.h>
#include <Wire.h>

#define IMU_ADDR      0x68  // your scan result
#define PWR_MGMT_1    0x6B  // this part boots asleep. Wake it before you time it
#define ACCEL_XOUT_H  0x3B  // MPU6050. LSM6DS3 OUTX_L_XL: 0x28
#define SECONDS       3

uint32_t sink = 0;

struct Run { uint32_t attempts, ok, bytes; };

Run freeRun(uint32_t hz, uint32_t perReadDelayMs) {
  Wire.setClock(hz);
  Run r = {0, 0, 0};
  uint32_t guard = millis();
  while (millis() - guard < SECONDS * 1000UL) {
    r.attempts++;
    Wire.beginTransmission(IMU_ADDR);
    Wire.write((uint8_t)ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) {          // 0 = the device acknowledged
      if (perReadDelayMs) delay(perReadDelayMs);
      continue;                                      // nobody answered: not a read
    }
    uint8_t got = Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)6);
    if (got != 6) {                                  // short answer: not a read either
      while (Wire.available()) Wire.read();
      if (perReadDelayMs) delay(perReadDelayMs);
      continue;
    }
    for (int b = 0; b < 6; b++) sink += Wire.read();
    r.ok++; r.bytes += 6;
    if (perReadDelayMs) delay(perReadDelayMs);       // the model's habit, timed
  }
  return r;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin();
  Wire.beginTransmission(IMU_ADDR);                 // wake it: nothing to read otherwise
  Wire.write((uint8_t)PWR_MGMT_1); Wire.write((uint8_t)0x00);
  uint8_t wake = Wire.endTransmission();
  if (wake != 0) {
    Serial.printf("NO DEVICE at 0x%02X (endTransmission returned %u).\n", IMU_ADDR, wake);
    Serial.println("Every rate below would be invented. Fix the bus first.");
  }
  Serial.println("run,clock_hz,delay_ms_per_read,seconds,attempts,ok,failed,bytes,reads_per_s");
  struct { uint32_t hz, d; } cfg[3] = { {100000, 0}, {400000, 0}, {400000, 1} };
  for (int i = 0; i < 3; i++) {
    Run r = freeRun(cfg[i].hz, cfg[i].d);
    uint32_t failed = r.attempts - r.ok;
    Serial.printf("%d,%lu,%lu,%d,%lu,%lu,%lu,%lu,", i + 1, cfg[i].hz, cfg[i].d,
                  SECONDS, r.attempts, r.ok, failed, r.bytes);
    if (r.ok == 0)      Serial.println("INVALID - not one transaction succeeded");
    else if (failed)    Serial.printf("INVALID - %lu of %lu attempts failed\n", failed, r.attempts);
    else                Serial.printf("%.1f\n", r.ok / (float)SECONDS);
  }
  Serial.printf("sink=%lu\n", sink);
  Serial.println("reads_per_s is a BUS read rate. It is not the rate the sensor produces");
  Serial.println("new samples at - that is the ODR, and it is set in the device's registers.");
}

void loop() {}
