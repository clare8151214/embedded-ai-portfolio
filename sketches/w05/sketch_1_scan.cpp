// Sketch 1 - the cheapest tool on this bus: ask every address whether anyone is home.
// Nothing in here knows what an IMU is. It only knows who answered.
#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(2000);                            // give the USB monitor time to attach
  Wire.begin();                           // board defaults: SDA = GPIO8, SCL = GPIO9
  Wire.setClock(100000);                  // Standard Mode. Start slow, always.

  Serial.println("addr,ack");
  int found = 0;
  for (uint8_t a = 0x08; a <= 0x77; a++) {
    Wire.beginTransmission(a);            // START, then address byte with the write bit
    uint8_t err = Wire.endTransmission(); // 0 means somebody pulled SDA low for the ACK
    if (err == 0) {
      Serial.printf("0x%02X,yes\n", a);
      found++;
    }
  }
  Serial.printf("devices_answering=%d\n", found);
}

void loop() {}
