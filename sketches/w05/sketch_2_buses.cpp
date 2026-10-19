// Sketch 2 - the same six-byte payload, moved three different ways, timed by the chip.
// I2C talks to the real device. SPI and UART talk to a jumper wire, because the thing
// under test is the bus, not whoever is on the far end of it.
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#define IMU_ADDR  0x68      // put YOUR scan result here. MPU6050: 0x68/0x69. LSM6DS3: 0x6A/0x6B
#define OUT_REG   0x3B      // MPU6050 ACCEL_XOUT_H. LSM6DS3: 0x28. Any readable block will do
#define FRAMES    2000      // six-byte frames per run
#define U1_TX     6         // jumper 6 -> 7. That single wire is the whole UART bus
#define U1_RX     5         // not connected
#define U2_TX     15        // not connected
#define U2_RX     7

uint32_t sink = 0;          // keeps the compiler from optimising the transfers away

void row(const char *bus, const char *setting, uint32_t bytes,
         uint32_t us, int pins, long ok) {
  Serial.printf("%s,%s,%lu,%lu,%d,%ld\n", bus, setting, bytes, us, pins, ok);
}

void i2cRun(uint32_t hz, const char *label) {
  Wire.setClock(hz);
  uint8_t buf[6];
  uint32_t t0 = micros();
  for (uint32_t i = 0; i < FRAMES; i++) {
    Wire.beginTransmission(IMU_ADDR);
    Wire.write((uint8_t)OUT_REG);
    Wire.endTransmission(false);                      // repeated START, keep the bus
    Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)6);
    for (int b = 0; b < 6; b++) buf[b] = Wire.available() ? Wire.read() : 0;
    for (int b = 0; b < 6; b++) sink += buf[b];
  }
  row("i2c", label, FRAMES * 6, micros() - t0, 2, -1);  // -1: payload not checkable here
}

void spiRun(uint32_t hz, const char *label) {
  SPI.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
  uint32_t ok = 0;
  uint32_t t0 = micros();
  for (uint32_t i = 0; i < FRAMES; i++) {
    for (int b = 0; b < 6; b++) {
      uint8_t sent = (uint8_t)(i + b);
      if (SPI.transfer(sent) == sent) ok++;            // MOSI is wired to MISO
    }
  }
  uint32_t dt = micros() - t0;
  SPI.endTransaction();
  row("spi", label, FRAMES * 6, dt, 4, (long)ok);
}

void uartRun(uint32_t txBaud, uint32_t rxBaud, const char *label) {
  const uint32_t BLOCKS = 20, BLK = 60;
  Serial1.begin(txBaud, SERIAL_8N1, U1_RX, U1_TX);
  Serial2.begin(rxBaud, SERIAL_8N1, U2_RX, U2_TX);
  delay(50);
  while (Serial2.available()) Serial2.read();          // drop anything already in the pipe
  uint32_t ok = 0;
  uint32_t t0 = micros();
  for (uint32_t b = 0; b < BLOCKS; b++) {
    for (uint32_t i = 0; i < BLK; i++) Serial1.write((uint8_t)i);
    Serial1.flush();                                   // wait until the last bit is out
    uint32_t n = 0, guard = millis();
    while (n < BLK && millis() - guard < 100) {
      if (Serial2.available()) {
        if ((uint8_t)Serial2.read() == (uint8_t)n) ok++;
        n++;
      }
    }
  }
  uint32_t dt = micros() - t0;
  Serial1.end();
  Serial2.end();
  row("uart", label, BLOCKS * BLK, dt, 1, (long)ok);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin();
  SPI.begin();

  Wire.beginTransmission(IMU_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.printf("no_ack_at=0x%02X  (the i2c rows will be timeouts, not transfers)\n",
                  IMU_ADDR);
  }

  Serial.println("bus,setting,payload_bytes,elapsed_us,signal_pins,bytes_ok");
  i2cRun(100000,  "100kHz");
  i2cRun(400000,  "400kHz");
  spiRun(1000000, "1MHz");
  spiRun(8000000, "8MHz");
  uartRun(115200, 115200, "115200-115200");
  uartRun(115200, 125000, "115200-125000");
  Serial.printf("sink=%lu\n", sink);
}

void loop() {}
