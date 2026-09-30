// Sketch B - the same 1000 intervals, scheduled by the chip instead of by your code.
// The main loop runs exactly the same work() as A and C. The timer does not care.
#include <Arduino.h>
#define N 1000

hw_timer_t *timer = NULL;
volatile uint32_t interval[N];         // shared with the ISR, so: volatile
volatile int n = 0;
volatile uint32_t prev = 0;

// ---- the workload: IDENTICAL in sketches A, B and C ---------------------------
// A real ECG sketch does more per sample than read the ADC: it filters, looks for
// R-peaks, prints. Every so often it sends a packet or writes a block to SD. Those
// costs vary from sample to sample. work() stands in for them with a fixed,
// repeatable pattern, so every board in the room runs exactly the same load.
volatile int ecg;
void work(int k) {
  ecg = analogRead(A0);                          // the read the prompt asked for
  uint32_t h = (uint32_t)k * 2654435761u;        // same "random" sequence on every board
  delayMicroseconds(200 + (h >> 16) % 1600);     // per-sample processing: 0.2 - 1.8 ms
  if (k % 50 == 49) delayMicroseconds(6000);     // every 50th sample: a 6 ms packet send
}
// -------------------------------------------------------------------------------

void IRAM_ATTR onTick() {              // fires every 4000 us, whatever the main loop is doing
  uint32_t now = micros();             // IRAM_ATTR: keep the handler out of flash
  if (n < N) { interval[n++] = now - prev; }
  prev = now;                          // no Serial, no delay, nothing slow in here
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  prev = micros();
  timer = timerBegin(0, 80, true);     // 80 MHz / 80 = 1 MHz -> 1 tick = 1 us
  timerAttachInterrupt(timer, &onTick, true);
  timerAlarmWrite(timer, 4000, true);  // 4000 us = 250 Hz
  timerAlarmEnable(timer);
  int done = 0;
  while (done < N) {                   // the main loop processes each sample the timer took.
    if (done < n) { work(done); done++; }   // a 6 ms packet send makes it fall behind for a
  }                                         // moment; the sampling instants do not move.
  timerAlarmDisable(timer);
  for (int i = 0; i < N; i++) Serial.println(interval[i]);
}

void loop() {}
