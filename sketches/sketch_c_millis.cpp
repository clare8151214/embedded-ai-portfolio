// Sketch C - non-blocking, but still scheduled by your own loop. Same 1000 intervals.
// Save as c-millis.txt and run read.py on it too: row C of the sheet.
#include <Arduino.h>
#define N 1000
uint32_t interval[N];

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

void setup() {
  Serial.begin(115200);
  delay(2000);
  uint32_t prev_us = 0;
  uint32_t next_ms = millis();                    // a deadline, not a duration
  for (int k = 0; k <= N; k++) {
    next_ms += 4;                                 // 250 Hz
    while ((int32_t)(millis() - next_ms) < 0) { } // busy-wait: the CPU does nothing else
    uint32_t now = micros();                      // the sampling instant
    if (k > 0) interval[k - 1] = now - prev_us;
    prev_us = now;
    work(k);   // fine while work < 4 ms. When it is longer, the next deadline has already
  }            // passed: that sample is late, and the one after it comes early to catch up.
  for (int n = 0; n < N; n++) Serial.println(interval[n]);
}

void loop() {}
