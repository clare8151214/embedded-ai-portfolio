// Sketch A - the delay(4) loop the model gave you: read, process, then delay(4).
// Timed with micros() at the top of every pass, which is the moment the sample is taken.
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
  delay(2000);                       // give the monitor time to attach
  uint32_t prev = 0;
  for (int k = 0; k <= N; k++) {
    uint32_t now = micros();         // the sampling instant
    if (k > 0) interval[k - 1] = now - prev;
    prev = now;
    work(k);                         // the loop body the model wrote
    delay(4);                        // "250 Hz -> 4 ms". But it waits 4 ms AFTER the work,
  }                                  // so the work's time is added to every period.
  for (int n = 0; n < N; n++) Serial.println(interval[n]);   // dump after, never during
}

void loop() {}
