# W05 AI usage

## 2026-10-08 · IMU initialisation over I2C (Cycle 2)

**Tool** Codex (GPT-6) for this session's analysis and edits. `sketch_3_ai_init.cpp` is the course pack's supplied model example; its original model and version are not identified in the pack.

**What I asked**
> 接下來做Cycle 2

The course's reference prompt, printed in `AI_USAGE_template.md`, asks for an Arduino MPU6050 program at ±4 g and ±500 dps. That prompt was not separately sent in this chat, so I do not attribute the supplied Sketch 3 to this Codex session.

**What it answered**
Codex identified the supplied Sketch 3 sequence: print `WHO_AM_I` without checking it, write `0x08` to `ACCEL_CONFIG` (`0x1C`), write `0x08` to `GYRO_CONFIG` (`0x1B`), immediately read output registers, and multiply raw counts by `0.061f` mg/LSB. The file neither writes `PWR_MGMT_1` to wake the device nor checks write ACKs or waits for a fresh sample. It ran unchanged and printed `who_am_i=0x68`, configuration readbacks `0x00,0x00`, `gap_us=1`, and ten all-zero frames.

**How I checked**
I used *MPU-6000 and MPU-6050 Register Map and Descriptions*, RM-MPU-6000A-00, Revision 4.2: p. 45 for `WHO_AM_I=0x68`; p. 8 for the reset `PWR_MGMT_1=0x40`; p. 40 for its `SLEEP` bit; pp. 14–15 for the two `0x08` full-scale bytes; and p. 29 for 8192 LSB/g at ±4 g, or `1000/8192 = 0.1220703125` mg/LSB. The course-supplied Sketch 4 then checked identity and ACKs, woke the device, waited for data readiness, and read back `pwr=0x00 cfg=0x08,0x08`. Its frame 0 was `raw_z=9140`, `g_z=1.116 g`, with `gap_us=29974 settled=1`. A board approximately flat should have an axis near 1 g. The two raw logs are `w05/ai-init.txt` and `w05/fixed.txt`.

**What I changed**
I kept the supplied Sketch 3 unmodified for comparison. In `src/main.cpp` I copied the supplied Sketch 4 and filled only its four blank constants: `0x68`, `0x08`, `0x08`, and `0.1220703125f`. Sketch 4's existing wake, wait, ACK checks, identity check and readback made the output defensible. The correct sensitivity is about twice Sketch 3's `0.061f`; applying the latter to a valid nonzero raw sample would report about half the g value. In this actual Run 3, raw samples were zero, so no measured ratio can be assigned.

## 2026-10-08 prediction / 2026-10-19 independent verification (Cycle 3)

**Tool** Codex (GPT-6), with PulseView, a physical FX2 and official libusb 1.0.30.

**What I asked**
> 把能做的全做完

For the later instrument check on 2026-10-19:
> 你幫我打開試試 已經插上分析儀了

**What it answered**
The pre-capture prediction was START, 0xD0 (7-bit 0x68 plus write), ACK, register 0x75, ACK, repeated START, 0xD1 (read), ACK, data 0x68, controller NACK and STOP. It was already present in the saved record sheet before the 2026-10-19 capture was inspected. Four bytes with nine clock periods each predict 36 SCL cycles, about 360 us at 100 kHz before edge overhead.

**How I checked**
PulseView's native FX2 opening still failed with 'Failed to open device / generic/unspecified error'. Codex used an isolated official libusb 1.0.30 library to acquire 10,000,000 physical FX2 samples at 2 MHz, paired with the same-reset COM6 serial log. No Windows driver was changed. The resulting w05/whoami-20261019-165030.sr was opened in PulseView and decoded with SDA=D0 and SCL=D1; all eleven predicted fields agreed. The final data byte was 0x68 followed by the controller's NACK and STOP. The first six captured acceleration bytes decoded to (718, -466, 9144), exactly matching serial frame 0.

Codex extracted edge sample positions, placed PulseView cursors at those positions and checked the displayed readouts. Consecutive SCL rising edges at 2.347412 s and 2.347422 s gave 10.0 us / 100.0 kHz. The final configuration STOP at 2.349362 s and first 0x3B-read START at 2.379390 s gave 30,028 us. The same-run serial log reported 29,974 us, settled=1, matching identity and no halt. crosscheck.py printed AGREE: +54 us (+0.18%), inside its unchanged 599.5 us tolerance. The 0.5 us sample interval limits time resolution; agreement does not prove absolute accuracy or accelerometer calibration.

**What I changed**
I completed the observed and clock fields, updated the cross-check and method note, and inserted the actual decoded capture and cursor figures into the homework. The old Run 3/Run 4 comparison logs remain unchanged. Section 7 uses the new matching serial file, not an unrelated older reset. Full screenshots, cursor sample indices, USB acquisition metadata, raw capture and the verbatim crosscheck.py output are retained in w05/. The VIMU self-reported fallback was not used. Native PulseView capture remains unrepaired; saved-file decoding works.

## 2026-10-19 Figures correction

At the student's request, Codex replaced text-only figure bodies with actual saved-log/application screenshots and original PDF excerpts. Seven Notepad screenshots show the original scan, analysis output, both init runs, verbatim build endings, complete crosscheck output and stream rows. Four source PDF excerpts substantiate the four constants; separated page regions are disclosed in their captions. The existing three physical PulseView screenshots remain. No new hardware data were taken and no original measurement log changed. All 13 revised Word pages were rendered and visually reviewed. Images and source hashes are in w05/figures/.

## 2026-10-19 Figure 7b revision

At the student's request, Codex reopened the existing physical capture in PulseView, labelled D0 as SDA and D1 as SCL, hid unused channels, and made three genuine screenshots: gap overview, final configuration STOP, and first 0x3B-read START. A composite stacks unscaled crops with headings outside the screenshots and discloses the different time scales. Full originals and hashes are in w05/figures/. Only Figure 7b image, caption, alt text and dimensions changed in the homework. All 13 pages were rendered and inspected. Raw measurements were unchanged.
