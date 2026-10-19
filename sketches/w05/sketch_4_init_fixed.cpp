// Sketch 4 - the same device, brought up in an order you can defend.
// Four constants below are deliberately blank. They are not on any slide and they are not
// in this folder: they are in the datasheet. Until you fill them in, this sketch will tell
// you truthfully that it has nothing to report.
//
// Three rules this file follows and sketch 3 does not. Each one closes a way a plausible
// number could reach your record sheet without a device being present:
//   1. A read returns success and value SEPARATELY. A failed read and a register that
//      really holds 0x00 produce the same byte; only the flag tells them apart. Sketch 3's
//      helper returns 0 for both, which is why its identity check could never have worked.
//   2. EVERY write is ACK-checked, not just the first one.
//   3. "There is data" comes from the device's own data-ready bit, not from "some axis is
//      non-zero". A board lying flat and still reads 0 on X and 0 on Y.
//
// gap_us is defined here, and slide 45 asks the analyser for the same two edges:
//   opens  at the STOP that ends the LAST configuration write
//   closes at the START of the first output-register read
// Nothing else - no register read-back, no Serial output - happens in between, on the bus
// or in this program. That is what makes the board's number and the instrument's number
// two accounts of one span instead of two numbers about two different things.
#include <Arduino.h>
#include <Wire.h>

#define IMU_ADDR      0x68  // your scan result
#define WHO_AM_I      0x75  // MPU6050. LSM6DS3: 0x0F
#define PWR_MGMT_1    0x6B  // MPU6050 power management. This part boots asleep
#define ACCEL_CONFIG  0x1C  // MPU6050. LSM6DS3 CTRL1_XL: 0x10
#define GYRO_CONFIG   0x1B  // MPU6050. LSM6DS3 CTRL2_G:  0x11
#define INT_ENABLE    0x38  // MPU6050. bit 0 DATA_RDY_EN - arms the bit below
#define INT_STATUS    0x3A  // MPU6050. bit 0 DATA_RDY_INT - cleared by reading it
#define ACCEL_XOUT_H  0x3B  // MPU6050. LSM6DS3 OUTX_L_XL: 0x28 (and the byte order differs)

// ----- the four blanks. From the datasheet: not from a slide, not from a model.
// `volatile` is not decoration. It stops the compiler folding these four away, so the
// build size on slide 33 is the same whether you have filled them in or not.
volatile const uint8_t WHOAMI_EXPECT   = 0x00; // 1. identity byte for the part you have
volatile const uint8_t ACCEL_CFG_VALUE = 0x00; // 2. full scale = +/-4 g, in one byte
volatile const uint8_t GYRO_CFG_VALUE  = 0x00; // 3. full scale = +/-500 dps, in one byte
volatile const float   XL_SENS_MG      = 0.0f; // 4. accel sensitivity at +/-4 g, mg per LSB
// -----

// Not a blank, and not something you are asked to find: the accelerometer's start-up time
// from sleep. No register on this part reports it, so it has to be a number in the code,
// and it is the datasheet's maximum rather than its typical because this is not the place
// to be optimistic.
const uint32_t XL_STARTUP_MS     = 30;
const uint32_t SETTLE_TIMEOUT_MS = 500;

// ----- Wire boilerplate. Same registers as sketch 3, different contract on every call.
bool w8(uint8_t reg, uint8_t val) {                 // true only if the device acknowledged
  Wire.beginTransmission(IMU_ADDR); Wire.write(reg); Wire.write(val);
  return Wire.endTransmission() == 0;
}
bool r8(uint8_t reg, uint8_t *val) {                // success and value, separately
  Wire.beginTransmission(IMU_ADDR); Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)1) != 1) return false;
  *val = Wire.read();
  return true;
}
bool readFrame(int16_t *xyz) {
  Wire.beginTransmission(IMU_ADDR); Wire.write((uint8_t)ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)IMU_ADDR, (uint8_t)6) != 6) return false;
  for (int a = 0; a < 3; a++) {
    uint8_t hi = Wire.read(), lo = Wire.read();     // high byte first on this part
    xyz[a] = (int16_t)((hi << 8) | lo);
  }
  return true;
}
// The device's own data-ready bit, armed by INT_ENABLE above. It says "a new sample has
// been written to the output registers" - a real statement, and not the same statement as
// "some axis came back non-zero". What it does NOT say is that the analogue front end has
// finished starting up; the datasheet states that separately, as a time, which is why
// XL_STARTUP_MS is waited out first. Neither condition alone is enough.
// Returns 1 = a frame is in xyz, 0 = timed out, -1 = the bus stopped answering.
int waitForData(int16_t *xyz, uint32_t *tRead) {
  uint32_t guard = millis();
  while (millis() - guard < SETTLE_TIMEOUT_MS) {
    uint8_t st = 0;
    if (!r8(INT_STATUS, &st)) return -1;            // no answer: not the same as "no data"
    if (!(st & 0x01)) continue;                     // no new sample yet
    *tRead = micros();                              // gap window closes here, and the very
    return readFrame(xyz) ? 1 : -1;                 // next thing on the bus is the read
  }
  return 0;
}
void dump(int16_t *xyz, float sens_mg) {
  Serial.println("frame,raw_x,raw_y,raw_z,g_x,g_y,g_z");
  for (int f = 0; f < 10; f++) {
    if (f) {
      delay(1000);
      if (!readFrame(xyz)) {          // a failed read is not a frame. Falling through here
        Serial.printf("%d,read_failed\n", f);   // would reprint the previous frame, and
        continue;                     // stale data looks exactly like fresh data.
      }
    }
    Serial.printf("%d,%d,%d,%d,%.3f,%.3f,%.3f\n", f, xyz[0], xyz[1], xyz[2],
                  xyz[0] * sens_mg / 1000.0f,
                  xyz[1] * sens_mg / 1000.0f,
                  xyz[2] * sens_mg / 1000.0f);
  }
}
// The four blanks, checked before anything is read. An unfilled blank is not a value.
const char *unfilledBlank() {
  if (WHOAMI_EXPECT   == 0x00)  return "1 WHOAMI_EXPECT";
  if (ACCEL_CFG_VALUE == 0x00)  return "2 ACCEL_CFG_VALUE";   // legal byte, wrong answer
  if (GYRO_CFG_VALUE  == 0x00)  return "3 GYRO_CFG_VALUE";    // to the question asked
  if (XL_SENS_MG      <= 0.0f)  return "4 XL_SENS_MG";
  return NULL;
}
// -----

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin();
  Wire.setClock(100000);

  const char *blank = unfilledBlank();                // 0. before touching the bus at all
  if (blank) {
    Serial.printf("halt=blank_not_filled blank=%s\n", blank);
    Serial.println("That constant is still at its shipped zero. Nothing below this line");
    Serial.println("could mean anything, so nothing below it runs.");
    return;
  }

  uint8_t id = 0;
  if (!r8(WHO_AM_I, &id)) {                           // 1. ask who is there, and check
    Serial.println("halt=identity_read_failed");      //    the ACK. No answer is not 0x00
    return;
  }
  Serial.printf("who_am_i=0x%02X expected=0x%02X\n", id, (uint8_t)WHOAMI_EXPECT);
  if (id != WHOAMI_EXPECT) { Serial.println("halt=identity_mismatch"); return; }

  struct Step { uint8_t reg, val; const char *what; };
  Step init[] = {                                     // 2. configure, checking every ACK
    { PWR_MGMT_1,   0x00,                      "PWR_MGMT_1 wake" },
    { ACCEL_CONFIG, (uint8_t)ACCEL_CFG_VALUE,  "ACCEL_CONFIG" },
    { GYRO_CONFIG,  (uint8_t)GYRO_CFG_VALUE,   "GYRO_CONFIG" },
    { INT_ENABLE,   0x01,                      "INT_ENABLE DATA_RDY_EN" },
  };
  for (int i = 0; i < 4; i++) {
    if (!w8(init[i].reg, init[i].val)) {
      Serial.printf("halt=write_nacked step=%d reg=%s\n", i + 1, init[i].what);
      return;                                         // one silent NACK and every number
    }                                                 // after it would be invented
  }
  uint32_t tCfg = micros();                           // 3. gap window OPENS here
  delay(XL_STARTUP_MS);                               // 4. the datasheet's start-up time
  int16_t xyz[3] = {0, 0, 0}; uint32_t tRead = 0;
  int live = waitForData(xyz, &tRead);                // 5. then the device's own say-so
  if (live != 1) {
    Serial.printf("settled=0 waited_us=%lu\n", micros() - tCfg);
    Serial.println(live == 0 ? "halt=no_data_ready_before_timeout"
                             : "halt=bus_stopped_answering");
    return;                                           // no gap_us line is printed, because
  }                                                   // there is no gap to report
  Serial.printf("gap_us=%lu settled=1\n", tRead - tCfg);

  uint8_t pwr = 0, acc = 0, gyr = 0;                  // 6. read the control registers back
  if (r8(PWR_MGMT_1, &pwr) && r8(ACCEL_CONFIG, &acc) && r8(GYRO_CONFIG, &gyr))
    Serial.printf("pwr=0x%02X cfg=0x%02X,0x%02X\n", pwr, acc, gyr);
  else
    Serial.println("readback=failed");                //    AFTER the window closed, so
                                                      //    this traffic is outside it
  dump(xyz, XL_SENS_MG);                              // 7. scale to the range you set
}

void loop() {}
