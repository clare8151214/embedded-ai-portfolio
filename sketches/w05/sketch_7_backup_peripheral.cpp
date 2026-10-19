// Sketch 7 - FALLBACK ONLY. Do not flash this unless a TA tells you to.
// It turns a second ESP32-S3-DevKitC-1 into an I2C peripheral that answers at 0x68 and keeps a
// byte-level log of every transaction it takes part in. It stands in for the IMU if the
// modules are missing, and for the analyser if the analysers are missing - but a log
// written by the device under test is not independent evidence, and that limit is the
// whole reason we bought instruments. Wiring: SDA-SDA, SCL-SCL, GND-GND between boards.
//
// It impersonates an MPU6050, including the part that catches everybody: it boots ASLEEP
// and returns zeros until the SLEEP bit in PWR_MGMT_1 is cleared. It goes back to sleep
// when that bit is set again, because a model that only sleeps once is not modelling sleep.
//
// What is modelled: SLEEP both ways, DEVICE_RESET, the settling time after a wake, and the
// data-ready bit that sketch 4 waits on. What is NOT modelled: temperature, self-test, the
// FIFO, the auxiliary I2C master, and clock-source selection - CLKSEL is stored and ignored.
#include <Arduino.h>
#include <Wire.h>

#define VIMU_ADDR  0x68
#define SETTLE_US  15000UL          // this device is not ready the instant you wake it
#define SAMPLE_US  1000UL           // one rate only: a new sample every 1 ms once settled
#define TRACE_N    64

#define R_GYRO_CONFIG   0x1B
#define R_ACCEL_CONFIG  0x1C
#define R_INT_ENABLE    0x38        // bit 0 DATA_RDY_EN
#define R_INT_STATUS    0x3A        // bit 0 DATA_RDY_INT, cleared when read
#define R_ACCEL_XOUT_H  0x3B
#define R_GYRO_XOUT_H   0x43
#define R_PWR_MGMT_1    0x6B        // bit 7 DEVICE_RESET, bit 6 SLEEP
#define R_WHO_AM_I      0x75

volatile uint8_t  reg[0x80];
volatile uint8_t  ptr = 0;
volatile uint32_t wokeAt = 0;
volatile uint32_t samplesAcked = 0;    // samples the controller has already been told about

// Counters, so that nothing this device does has to be inferred from silence.
volatile uint32_t bytesOffered  = 0;   // bytes PUT ON THE BUS when asked. See onReq().
volatile uint32_t traceDropped  = 0;   // trace entries lost because the ring was full
volatile uint32_t resetCount    = 0;

struct Ev { uint32_t us; char kind; uint8_t val; };
volatile Ev trace[TRACE_N];
volatile uint8_t tw = 0, tr = 0;

void note(char kind, uint8_t val) {
  uint8_t n = (uint8_t)((tw + 1) % TRACE_N);
  if (n == tr) { traceDropped++; return; }    // full: drop, but COUNT it. A log that
  trace[tw].us = micros();                    // shortens itself silently is worse than
  trace[tw].kind = kind;                      // no log at all.
  trace[tw].val = val;
  tw = n;
}

const float XL_SENS[4] = { 0.061f, 0.122f, 0.244f, 0.488f };   // mg/LSB for AFS_SEL 0..3

// SLEEP is a bit, not an event. Set it again and this device is asleep again.
bool awake() {
  if (reg[R_PWR_MGMT_1] & 0x40) return false;                  // SLEEP set: asleep, now
  return wokeAt != 0 && (micros() - wokeAt) >= SETTLE_US;
}

// Samples land on the device's OWN cadence, counted from the end of the settling window.
// They do not land SAMPLE_US after whenever you last looked - a poller that asks faster
// than the sample rate must still be told "not yet, not yet, yes", and a model that
// restarted its countdown on every read would answer "not yet" for ever.
uint32_t samplesSinceWake() {
  if (!awake()) return 0;
  return (micros() - (wokeAt + SETTLE_US)) / SAMPLE_US + 1;
}
bool dataReady() {
  if (!(reg[R_INT_ENABLE] & 0x01)) return false;   // the bit only arms if you armed it
  uint32_t n = samplesSinceWake();
  return n != 0 && n > samplesAcked;
}

void applyReset() {                           // PWR_MGMT_1 bit 7 on the real part
  for (int i = 0; i < 0x80; i++) reg[i] = 0x00;
  reg[R_WHO_AM_I]   = 0x68;
  reg[R_PWR_MGMT_1] = 0x40;                   // reset value: SLEEP set. It boots asleep
  wokeAt = 0;
  samplesAcked = 0;
  resetCount++;
}

int16_t axisRaw(uint8_t idx) {                // X, Y, Z of a device lying flat and still
  if (idx != 2) return 0;
  uint8_t fs = (reg[R_ACCEL_CONFIG] >> 3) & 0x03;
  return (int16_t)lroundf(1000.0f / XL_SENS[fs]);              // exactly 1.000 g on Z
}

uint8_t readReg(uint8_t a) {
  if (a == R_INT_STATUS) {
    uint8_t v = dataReady() ? 0x01 : 0x00;
    samplesAcked = samplesSinceWake();        // reading it clears it, as on the real part.
    return v;                                 // See onReq() for why "reading" is generous.
  }
  if (a >= R_ACCEL_XOUT_H && a <= R_ACCEL_XOUT_H + 5) {
    if (!awake()) return 0x00;
    int16_t v = axisRaw((uint8_t)((a - R_ACCEL_XOUT_H) / 2));
    return ((a - R_ACCEL_XOUT_H) & 1) ? (uint8_t)(v & 0xFF) : (uint8_t)(v >> 8);
  }
  if (a >= R_GYRO_XOUT_H && a <= R_GYRO_XOUT_H + 5) {
    return 0x00;                              // still device: zero rate on every axis
  }
  return reg[a & 0x7F];
}

void onRecv(int len) {
  if (len <= 0) return;
  ptr = (uint8_t)Wire.read();
  note('P', ptr);
  for (int i = 1; i < len; i++) {
    uint8_t v = (uint8_t)Wire.read();
    uint8_t a = (uint8_t)((ptr + (i - 1)) & 0x7F);
    note('W', v);
    if (a == R_WHO_AM_I) continue;            // identity is read-only
    if (a == R_PWR_MGMT_1 && (v & 0x80)) { applyReset(); continue; }   // DEVICE_RESET
    bool wasAsleep = (reg[R_PWR_MGMT_1] & 0x40) != 0;
    reg[a] = v;
    if (a == R_PWR_MGMT_1 && wasAsleep && !(v & 0x40)) { wokeAt = micros(); samplesAcked = 0; }
  }
}

// The controller asks for N bytes; this callback has to have them ready before it knows
// what N is, so it always offers eight. Eight is therefore the number of bytes OFFERED,
// which is not the number of bytes the controller took - an offered byte that nobody
// clocked out never existed on the wire. The trace marks them 'O' for that reason, and
// bytesOffered is named for what it counts. If you need bytes actually read, the analyser
// is the only thing in the room that can tell you.
void onReq() {
  uint8_t out[8];
  for (int i = 0; i < 8; i++) {
    out[i] = readReg((uint8_t)((ptr + i) & 0x7F));
    note('O', out[i]);
  }
  bytesOffered += 8;
  Wire.write(out, 8);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  applyReset();                               // the same path a DEVICE_RESET write takes
  resetCount = 0;                             // that first one was the power-on, not a write
  Wire.onReceive(onRecv);
  Wire.onRequest(onReq);
  Wire.begin((uint8_t)VIMU_ADDR, SDA, SCL, 0);
#if CONFIG_IDF_TARGET_ESP32
  uint8_t seed[8] = {0x68, 0, 0, 0, 0, 0, 0, 0};
  Wire.slaveWrite(seed, 8);                   // the original ESP32 needs the first byte primed
#endif
  Serial.printf("backup_peripheral addr=0x%02X settle_us=%lu sample_us=%lu\n",
                VIMU_ADDR, SETTLE_US, SAMPLE_US);
  Serial.println("spec: 0x75 WHO_AM_I=0x68 | 0x6B PWR_MGMT_1 bit7=DEVICE_RESET "
                 "bit6=SLEEP, reset 0x40 | 0x1C ACCEL_CONFIG [4:3]=AFS_SEL "
                 "| 0x1B GYRO_CONFIG [4:3]=FS_SEL | 0x38 INT_ENABLE bit0=DATA_RDY_EN "
                 "| 0x3A INT_STATUS bit0=DATA_RDY, cleared on read "
                 "| 0x3B.. accel out, high byte first");
  Serial.println("spec: AFS_SEL 0,1,2,3 -> 0.061, 0.122, 0.244, 0.488 mg/LSB");
  Serial.println("spec: not modelled - temperature, self-test, FIFO, aux I2C, CLKSEL");
  Serial.println("us,kind,byte      P=pointer W=written O=offered (offered is not read)");
}

void loop() {
  while (tr != tw) {
    Serial.printf("%lu,%c,0x%02X\n", trace[tr].us, trace[tr].kind, trace[tr].val);
    tr = (uint8_t)((tr + 1) % TRACE_N);
  }
  static uint32_t pOff = 0xFFFFFFFF, pDrop = 0xFFFFFFFF, pRst = 0xFFFFFFFF;
  if (bytesOffered != pOff || traceDropped != pDrop || resetCount != pRst) {
    pOff = bytesOffered; pDrop = traceDropped; pRst = resetCount;
    Serial.printf("stat: bytes_offered=%lu trace_dropped=%lu resets=%lu awake=%d\n",
                  bytesOffered, traceDropped, resetCount, awake() ? 1 : 0);
  }
  delay(200);
}
