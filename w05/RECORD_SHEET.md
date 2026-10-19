# W05 Record Sheet

**Name** 鄭宇峻  **Student ID** NQ6141054  **Date** 2026-10-08 (original runs); 2026-10-19 (independent capture and closeout)

**Group / bench number** N/A — no group assigned (user confirmed). **Capture slot** N/A — individual setup. **Analyser** Physical sigrok FX2 LA (8ch). **Capture laptop** Windows PC, ESP32 on COM6; acquisition via libusb 1.0.30, saved-file decoding in PulseView.

Every cell below is measured by you, in this room, today, or copied by you out of a
datasheet with the page number written next to it. There is one number in this deck that
was given to you — the flash cost on slide 33 — and it is labelled as such. Everything
else is yours. If a cell is empty at 12:00, say so; an empty cell is a result, a guessed
cell is not.

---

## 1 · Who is on the bus (slide 11)

| | |
|---|---|
| Addresses that answered | `0x68` |
| Number of devices answering | 1 |
| Therefore the part I have is | MPU6050 on GY-521 module (scan `0x68`, then checked `WHO_AM_I=0x68` in Cycle 2 and the physical capture) |
| Datasheet I am using (title and version) | *MPU-6000 and MPU-6050 Register Map and Descriptions*, RM-MPU-6000A-00, Revision 4.2 |

**Did anything answer that you did not expect?**

> No. One device answered at `0x68`.

## 2 · Three buses, same six bytes (slide 18)

From `bus_table.py`. Copy the row, do not retype the raw counts.

| Bus | Setting | Signal pins | kbit/s | kbit/s per pin | µs per 6-byte frame | Payload verified |
|---|---|---|---|---|---|---|
| I2C | 100kHz | 2 | 52.6 | 26.3 | 911.8 | not checkable |
| I2C | 400kHz | 2 | 179.5 | 89.8 | 267.3 | not checkable |
| SPI | 1MHz | 4 | 763.9 | 191.0 | 62.8 | 12000/12000 |
| SPI | 8MHz | 4 | 3220.2 | 805.0 | 14.9 | 12000/12000 |
| UART | 115200-115200 | 1 | 71.5 | 71.5 | 671.5 | 1200/1200 |
| UART | 115200-125000 | 1 | 71.6 | 71.6 | 670.3 | 320/1200 |

**Which bus moved the most bits per signal pin, and by what factor over the worst?**

> SPI at 8 MHz: 805.0 kbit/s per pin, versus I2C at 100 kHz: 26.3 kbit/s per pin; 805.0 / 26.3 = about 30.6 times.

**The UART rows use the same wire at the same speed. Only the far end's expectation
changed. How many of the 1200 bytes survived the mismatched row?**

> 320 of 1200 bytes matched the known pattern (26.7%); 880 did not verify. The 71.6 kbit/s figure is the rate of the whole attempt, including receive time-outs.

**Why is the "payload verified" column empty for I2C and not for the other two?**

> The I2C row reads live IMU data, whose six bytes are not known in advance, so bytes_ok is -1 (not checkable). The SPI and UART loopbacks compare received bytes against the known transmitted pattern.

## 3 · The four blanks (slide 28)

Fill these from the datasheet. **Write the page or table number you found each one on** —
an answer with no page number is an answer you cannot defend.

| | Value | Found on page / table |
|---|---|---|
| `WHOAMI_EXPECT` | `0x68` | Register Map rev. 4.2, p. 45, §4.32 (`WHO_AM_I` default) |
| `ACCEL_CFG_VALUE` — ±4 g | `0x08` | p. 15, §4.5 (`AFS_SEL=1` in bits 4:3) |
| `GYRO_CFG_VALUE` — ±500 dps | `0x08` | p. 14, §4.4 (`FS_SEL=1` in bits 4:3) |
| `XL_SENS_MG` — mg/LSB at ±4 g | `0.1220703125` mg/LSB | p. 29, §4.17: 8192 LSB/g; 1000/8192 mg/LSB |

**The full-scale field is two bits, so there are four codes. Write them out in the order
the datasheet gives them.** If that order surprised you, say so.

| Code `00` | Code `01` | Code `10` | Code `11` |
|---|---|---|---|
| ±2 g | ±4 g | ±8 g | ±16 g |

## 4 · The model's version against yours (slides 26, 31)

| | Sketch 3 · the model | Sketch 4 · yours |
|---|---|---|
| `who_am_i` printed | `0x68` | `0x68` (`expected=0x68`) |
| Checked against the datasheet in code? | No; printed only | Yes; halts on mismatch |
| `gap_us` — see the definition below | 1 µs | 29,974 µs (`settled=1`) |
| `raw_z` on frame 0 | 0 codes | 9,140 codes |
| `g_z` on frame 0 | 0.000 g | 1.116 g |
| `raw_z` on frame 5 | 0 codes | 9,190 codes |
| `g_z` on frame 5 | 0.000 g | 1.122 g |

> **What `gap_us` measures, in both sketches and on the analyser.** It opens at the
> **STOP that ends the last configuration write** and closes at the **START of the
> first read of the output registers** (`0x3B`). The data-ready polls of `0x3A` in
> between are inside the window on all three accounts, so they cancel. Neither sketch
> reads a register back or prints anything inside that window — if it did, the two
> numbers would not be about the same span and subtracting them would mean nothing.

**Frame 0 and frame 5 came from the same device under the same configuration. If they
differ, what happened between them, and how long did it take?**

> Sketch 3 stayed at zero. In Sketch 4, frame 5 came about five 1-second delays after frame 0; `raw_z` changed from 9,140 to 9,190 (+50 codes, about +0.006 g). The sensor kept producing new samples; the small change alone does not establish motion.

**Your board is lying flat and still. Physics says one axis reads about 1 g. Which of
the two sketches agrees with physics? If the other one is not zero, by what factor is it
out? If it is zero, write that — zero is not a factor — and say which of Run 3's three
causes you can now rule out, and on what evidence.**

> Sketch 4 is consistent with roughly 1 g on Z (1.116 g at frame 0); Sketch 3 printed zero, so no finite factor can be assigned. The unsupported-range explanation is ruled out: the register-map tables on pp. 14–15 specify `0x08`, and Sketch 4 read back `cfg=0x08,0x08` and produced live samples. Run 3's `cfg=0x00,0x00` and 1 µs gap do not isolate sleep from missing settling time. No `halt=` line appeared in Run 4.

**Earliest useful check and cost:** Read `PWR_MGMT_1` (`0x6B`) immediately after identity: its reset value `0x40` has `SLEEP=1` (register map pp. 8, 40), so one extra one-byte I2C register read would reveal that the model had not woken the device before configuring it.

## 5 · The price of being correct (slide 33)

Check these against your own build output. This is the one pair of numbers the deck gives
you, because the linker produced it, not your board. RAM comes out the same on every
machine. Flash can differ by a few bytes from one machine to another, because the project
path is compiled into the image — so the number to compare is the **difference** between
the two sketches, which should not move.

| | Flash (bytes) | RAM (bytes) |
|---|---|---|
| Sketch 3 · the model's init — deck | 283,925 | 18,760 |
| Sketch 3 — **yours** | 283,877 | 18,760 |
| Sketch 4 · your init — deck | 285,001 | 18,760 |
| Sketch 4 — **yours** | 284,969 | 18,760 |

Difference in flash, yours: **1,092 bytes**. In RAM: **0 bytes**.

Sketch 4 was built with all four blanks filled. The measured Flash difference is 16 bytes
larger than the deck's 1,076-byte difference; both local builds used the same project and
toolchain. The reason for that 16-byte difference has not been established.

Build sketch 4 **with your four blanks filled in**. It should give the same figure
either way — if it does not, say so, because that would mean the compiler threw one
of the checks away and the number above is measuring the wrong program.

## 6 · The transaction, byte by byte (slides 40, 42, 44)

**Fill the *predicted* column before your turn at the bench** — from your own scan result
and slide 38's anatomy. Fill *observed* afterwards, from your own capture. A prediction
you write after seeing the answer is not a prediction, and it is obvious in the marking.

| # | Field | Predicted | Observed | Agree? |
|---|---|---|---|---|
| 1 | START | START | START | Yes |
| 2 | address + R/W bit | 0xD0 (0x68, write) | 0xD0 (7-bit 0x68, write) | Yes |
| 3 | ACK / NACK | ACK from IMU | ACK from IMU | Yes |
| 4 | register address | 0x75 (WHO_AM_I) | 0x75 (WHO_AM_I) | Yes |
| 5 | ACK / NACK | ACK from IMU | ACK from IMU | Yes |
| 6 | repeated START | repeated START | repeated START | Yes |
| 7 | address + R/W bit | 0xD1 (0x68, read) | 0xD1 (7-bit 0x68, read) | Yes |
| 8 | ACK / NACK | ACK from IMU | ACK from IMU | Yes |
| 9 | data byte | 0x68 (WHO_AM_I) | 0x68 | Yes |
| 10 | ACK / NACK | NACK from controller (final byte) | NACK from controller (final byte) | Yes |
| 11 | STOP | STOP | STOP | Yes |

**Prediction before capture:** Four wire bytes (write address, register, read address, data), each with an ACK/NACK bit: 4 x 9 = 36 SCL cycles. At the requested 100 kHz, the bit time is about 10 us, so the transaction should take roughly 360 us plus START/STOP overhead. This is distinct from Run 4's 29,974 us configuration-to-first-read gap.

**Any field where prediction and observation disagree is the most interesting line on this
sheet. Say which one and what you concluded.**

> All eleven predicted fields agree with the saved physical capture. The complete WHO_AM_I transaction lasts 408.0 us; its 36 clock periods account for about 360 us, while edge conditions and pauses account for the remainder. PulseView shows the 7-bit address 0x68 plus write/read direction; the corresponding on-wire bytes are 0xD0 and 0xD1.

**The address byte on the wire is not the number your scan printed. Write both, and
explain the relationship in one line.**

> Verified in `w05/whoami-20261019-165030.sr`: the scan address is 7-bit 0x68; (0x68 << 1) | 0 = 0xD0 for write and (0x68 << 1) | 1 = 0xD1 for read.

**Measured SCL period from the analyser cursors: 10.0 µs → 100.0 kHz.
You asked for 100 kHz. Is the difference inside or outside what you would accept, and
why?**

> The PulseView cursors at 2.347412 s and 2.347422 s bracket consecutive SCL rising edges: 20 samples at 2 MHz, or 10.0 us / 100.0 kHz. This agrees with the requested clock at the capture's 0.5 us sample resolution; it does not establish zero frequency error. Screenshot: `w05/scl-period-cursors.png`.

**Graded, 3 marks. If SDA and SCL were swapped in the wiring itself, what would the
analyser show, and why?** Answer in terms of *which line carries timing and which
carries data*. "It would break" is not an answer; neither is "the signal gets slower".

> With the board-to-IMU SDA and SCL wires crossed, the analyser at GPIO8/GPIO9 would still show the controller driving data on GPIO8 and clock pulses on GPIO9, but the IMU would receive those pulses on its data input and the data transitions on its clock input. It would not decode the request or pull SDA low for ACK; the trace would show the controller attempt followed by a NACK/no valid device reply.
>
>
>
## 7 · The cross-check (slide 46)

From `crosscheck.py`.

| | µs |
|---|---|
| `gap_us` - the board's own `micros()` | 29,974 |
| the same gap measured on the analyser | 30,028 |
| difference | +54 |
| difference as % of the board's number | +0.18% |
| the script's verdict - AGREE / DISAGREE / **NO VERDICT** | AGREE, within the stated tolerance |

If it printed `NO VERDICT`, **write down which precondition it named** and what you
would have to change to get past it. That is a complete answer to this section; an
agreement between two numbers that were never measurements is not.

**In W03 you timed the chip with the chip and we said nobody was checking the referee.
Somebody is now. Write one sentence saying what today's two numbers let you claim that
W03's single number did not.**

> A separate FX2 clock measured 30,028 us against the same-reset ESP32 value of 29,974 us, a +54 us (+0.18%) difference inside the script's 599.5 us tolerance; this independently supports the timing of this interval, without establishing absolute clock accuracy or sensor calibration.
>
**Same-run evidence:** `w05/whoami-20261019-165030-serial.txt` and `w05/whoami-20261019-165030.sr`. These were collected during the same reset. The board value happens to equal the older Run 4 result; the comparison used this new paired serial log. The final configuration STOP is sample 4,698,724 (2.349362 s), and the first 0x3B-read START is sample 4,758,780 (2.379390 s). Their separation is 60,056 samples / 2 MHz = 30,028 us. The 0x3A data-ready poll is inside that interval. PulseView cursor screenshot: `w05/gap-cursors.png`; verbatim script output: `w05/crosscheck-20261019-165030.txt`.

## 8 · What you can actually get off the bus (slide 48)

From Sketch 6.

| Run | Clock | delay per read | reads/s |
|---|---|---|---|
| 1 | 100 kHz | 0 ms | 1096.0 |
| 2 | 400 kHz | 0 ms | 3727.0 |
| 3 | 400 kHz | 1 ms | 1000.0 |

**(a) You want 104 six-byte frames a second. Does each of these three runs clear that bar?**

> Yes. Runs 1, 2 and 3 achieved 1096.0, 3727.0 and 1000.0 successful six-byte reads/s, respectively; each exceeds 104/s and each had failed=0.

**(b) Run 2 and run 3 differ by one `delay(1)`. What did that one line cost you, as a
factor?** Write the division, not just the answer.

> 3727.0 / 1000.0 = 3.727, so the 1 ms delay reduced the bus-read rate by about 3.73 times.

**(c) Now the harder half: the Register Map (§4.2, p. 12) says the accelerometer's output
rate is 1 kHz. Given that, how many *new* samples did the `delay(1)` actually lose you?
What does your answer to (b) therefore mean?**

> At the stated 1 kHz sensor output rate, at most 1000 fresh samples exist each second. Both runs reach at least 1000 bus reads/s, so the delay loses zero fresh samples in ideal rate accounting; it mostly removes redundant reads (up to about 2727/s). This log does not identify individual fresh samples, so an exact count of missed samples is not directly measured.

## 9 · Method note (one sentence, required)

> The original bus/IMU logs were collected through COM6 on 2026-10-08; a physical FX2 independently sampled GPIO8/SDA and GPIO9/SCL at 2 MHz on 2026-10-19, with a same-reset serial log, and PulseView cursors confirmed the SCL period and configuration-to-first-read interval.

Anything else about how you measured that a reader would need in order to repeat it —
including anything that did **not** work:

> The first Run 7 attempt printed `NO DEVICE at 0x68 (endTransmission returned 2).` at wake-up, so I excluded that attempt despite its later successful reads; after a separate RTS reset, the saved `w05/stream.txt` had three rows with `failed=0` and no `NO DEVICE` line. The cause of the initial NACK was not established.

**Capture method and assistance:** Native PulseView still reported `Failed to open device / generic/unspecified error`. An isolated official libusb 1.0.30 library and an asynchronous USB reader collected 10,000,000 physical 8-bit samples (5 s) without reported USB transfer errors. PulseView then opened and decoded the saved `.sr` file with SDA=D0, SCL=D1. Codex located the edges from sample indices and entered those exact positions into the PulseView cursors; the displayed readouts were checked and saved. The first captured acceleration frame (718, -466, 9144) exactly matches the same-reset serial frame. This used the real IMU and an independent FX2, not the VIMU self-reported fallback; no Windows driver was changed. `w05/cursor-measurements-20261019.json` records the cursor positions.

## 10 · Judgement (one sentence, required)

The model's initialisation compiled, ran, printed no errors, and produced a number.
Name **the one check** that would have caught it earliest, and say what it would have
cost to add — **with your own numbers in the sentence.**

> A checked read of `PWR_MGMT_1` (`0x6B`) immediately after identity would catch a still-asleep IMU (`SLEEP=1` at its datasheet reset value `0x40`); it costs one extra one-byte register read (about 36 SCL cycles, or 360 us at 100 kHz, calculated), before accepting Sketch 3's measured `gap_us=1` and `g_z=0.000 g` instead of Sketch 4's `gap_us=29,974` and `g_z=1.116 g`.
>
---

## Before you leave

- [x] `scan.txt`, `buses.txt`, `ai-init.txt`, `fixed.txt`, `stream.txt` in `w05/`
- [x] `whoami-capture.png` **and** the saved capture file it came from, in `w05/`
- [x] `bus_table.py` and `crosscheck.py` committed, so your numbers can be reproduced
- [x] Two entries in `AI_USAGE.md` — the init prompt and the datasheet cross-check (including 2026-10-19 verification)
- [x] This sheet, filled in, **with page numbers in section 3**
- [ ] Google Form / course upload: not submitted by this session; the submission URL has not been supplied.

Files in the repo the same day. The analyser goes back in the box with its clips.
