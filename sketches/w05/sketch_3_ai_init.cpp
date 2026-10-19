// Sketch 3 - the model's initialisation, reduced to the part under test.
// Nothing here has been corrected. The register names are the model's, the order is the
// model's, the conversion constant is the model's. Your job is to find out what it costs.
//
// One thing IS ours, and it is the ruler, not the thing being measured: gap_us brackets
// the same two edges here, in sketch 4, and on the analyser -
//   opens  at the STOP that ends the LAST configuration write
//   closes at the START of the first output-register read
// so that the two sketches' gap_us answer the same question. Read what this file does
// between those two edges. That, not the number's size, is the finding.
#include <Arduino.h>
#include <Wire.h>

#define IMU_ADDR      0x68  // your scan result
#define WHO_AM_I      0x75  // MPU6050. LSM6DS3: 0x0F
#define ACCEL_CONFIG  0x1C  // MPU6050 accelerometer control. LSM6DS3 CTRL1_XL: 0x10
#define GYRO_CONFIG   0x1B  // MPU6050 gyroscope control.     LSM6DS3 CTRL2_G:  0x11
#define ACCEL_XOUT_H  0x3B  // MPU6050 first accel output.    LSM6DS3 OUTX_L_XL: 0x28

// ----- twelve lines of Wire boilerplate. Identical in sketch 4. Not what is under test.
void w8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(IMU_ADDR); Wire.write(reg); Wire.write(val); Wire.endTransmission();
}
uint8_t r8(uint8_t reg) {
  Wire.beginTransmission(IMU_ADDR); Wire.write(reg); Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0;
}
bool readFrame(int16_t *xyz) {
  Wire.beginTransmission(IMU_ADDR); Wire.write((uint8_t)ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)6);
  for (int a = 0; a < 3; a++) {
    uint8_t hi = Wire.available() ? Wire.read() : 0;   // this part is high byte first
    uint8_t lo = Wire.available() ? Wire.read() : 0;
    xyz[a] = (int16_t)((hi << 8) | lo);
  }
  return true;
}
void dump(int16_t *xyz, float sens_mg) {
  Serial.println("frame,raw_x,raw_y,raw_z,g_x,g_y,g_z");
  for (int f = 0; f < 10; f++) {
    if (f) { delay(1000); readFrame(xyz); }
    Serial.printf("%d,%d,%d,%d,%.3f,%.3f,%.3f\n", f, xyz[0], xyz[1], xyz[2],
                  xyz[0] * sens_mg / 1000.0f,
                  xyz[1] * sens_mg / 1000.0f,
                  xyz[2] * sens_mg / 1000.0f);
  }
}
// -----

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin();
  Wire.setClock(100000);

  Serial.printf("who_am_i=0x%02X\n", r8(WHO_AM_I));   // printed, never checked

  w8(ACCEL_CONFIG, 0x08);             // the model's byte for "+/-4 g"
  w8(GYRO_CONFIG,  0x08);             // the model's byte for "+/-500 dps"
  uint32_t tCfg = micros();           // gap window opens: configuration is on the wire

  int16_t xyz[3];
  uint32_t tRead = micros();          // gap window closes: the read is about to start
  readFrame(xyz);                     // no wait of any kind between config and first read
  uint32_t gap = tRead - tCfg;

  Serial.printf("accel_cfg=0x%02X gyro_cfg=0x%02X\n", r8(ACCEL_CONFIG), r8(GYRO_CONFIG));
  Serial.printf("gap_us=%lu\n", gap);
  dump(xyz, 0.061f);                  // the model's constant, unchanged
}

void loop() {}
