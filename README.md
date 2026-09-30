# W03 Student Pack — Timing, Timers & Sampling Jitter

## W03 作業入口（2026-09-30 整理）

姓名：鄭宇峻；學號：NQ6141054。

- [完整實驗紀錄與頻譜比較](w03/RECORD_SHEET.md)：A／B／C 各 1000 筆間隔、統計、Flash／RAM、FFT 與 Sketch D 結果。
- [AI 使用聲明](AI_USAGE.md) 與 [250 Hz 原始回答](w03/ai_250hz_response.md)。
- [原始資料與圖檔](w03/)；[實驗程式](sketches/)；[原始串口日誌](logs/)。

目前 `src/main.cpp` 是 Sketch D；`platformio.ini` 使用 ESP32-S3 原生 USB。A／B／C 保存的量測使用 UART 橋接介面，重現其 Flash／RAM 時需註解 USB CDC build flag，詳見實驗紀錄第 3 節。

在專案根目錄執行以下 PowerShell 指令可重新分析已保存資料，不需要接板子：

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install numpy matplotlib
.\.venv\Scripts\python.exe scripts\read.py w03\a-delay.txt
.\.venv\Scripts\python.exe scripts\read.py w03\b-timer.txt
.\.venv\Scripts\python.exe scripts\read.py w03\c-millis.txt
.\.venv\Scripts\python.exe scripts\floor.py w03\a-delay.txt
.\.venv\Scripts\python.exe scripts\floor.py w03\b-timer.txt
```

`floor.py` 會重新產生對應 PNG；它分析的是合成訊號在實測時間點取樣的頻譜，不是實際 ECG／ADC 的噪聲量測。

GitHub 保存不等於 Google Form 已提交；表單提交與將 Sketch D 結果補入 HW1 報告仍待完成。以下保留原課程套件說明；其中「逐字相同」等敘述指原始套件，後續調整以本節、實驗紀錄及 AI 使用聲明為準。

---

**Embedded Systems (BME 3301) · Week 03 · Thursday 24 Sep, 09:10–12:00 · 醫技醫工館 5776**

Everything you run in class today is in this folder. You do not have to type any of it
from the slides — the files here are character-for-character identical to what is projected.

**`PREFLIGHT.md` is required before you come to class.** About twenty minutes, needs
internet, needs no board. Week 3 is the first session that uses Python, and if forty-five
people install it at 09:15 over the room's wifi we lose the morning. Come with all four
boxes ticked.

---

## What is in here

```
platformio.ini              W02's five lines plus monitor_filters = log2file
sketches/
  sketch_a_delay.cpp        the AI's answer, rewritten to time itself      (cycle 2, run 1)
  sketch_b_timer.cpp        our sample, hardware timer + ISR               (cycle 1, then run 2)
  sketch_c_millis.cpp       the middle option, non-blocking but still yours (cycle 2, run 3)
  sketch_d_toggle.cpp       L2 vs L3 GPIO cost — the W02 measurement we owe you (cycle 3)
scripts/
  read.py                   turns raw intervals into mean / stdev / max deviation
  floor.py                  same tone, your two sampling schedules, two spectra
RECORD_SHEET.md             fill this in as you go. It is the deliverable
AI_USAGE_template.md        the two entries you owe in AI_USAGE.md
PREFLIGHT.md                do this before class
```

## What you produce today

Six files, all of them in your repo under `w03/`:

| File | From | Slide |
|---|---|---|
| `a-delay.txt` | Sketch A, 1000 raw intervals in µs | 32–36 |
| `b-timer.txt` | Sketch B, 1000 raw intervals in µs | 38–39 |
| `c-millis.txt` | Sketch C, 1000 raw intervals in µs | 41 |
| `a-delay.txt.png` | `floor.py` — fixed axes, so the two are comparable | 54 |
| `b-timer.txt.png` | `floor.py` | 55 |
| `RECORD_SHEET.md` | filled in | 31, 42 |

Plus two entries in `AI_USAGE.md`, and the Google Form before you leave.

## The one thing that is not in this folder

**Sketch A is not the sketch you will actually generate.** In cycle 1 you write the prompt
yourself and read what the model gives back. `sketch_a_delay.cpp` here is that answer after
it has been stripped down to the only thing under test — the schedule. Use the pack version
for the measurement so that all 45 of us are measuring the same thing; keep your own
generated version in the repo too, because the difference between them is part of what you
are being asked to notice.

## How your data gets off the board

You do **not** select 1000 lines with the mouse. `monitor_filters = log2file` in
`platformio.ini` makes the Serial Monitor write everything into a
`platformio-device-monitor-*.log` file in your project folder while it displays. After each
run, take the **newest** log, rename it, and move it into `w03/`:

| Run | Sketch | Rename the log to |
|---|---|---|
| 1 | A · `delay(4)` | `a-delay.txt` |
| 2 | B · timer ISR | `b-timer.txt` |
| 3 | C · `millis()` | `c-millis.txt` |

`read.py` only reads lines that are entirely digits, so any banner text left at the top of
the log is ignored. Leave it in.

## Units

Raw files are **microseconds**, one integer per line, no header.
Everything you report is **milliseconds to four decimal places**.
A number without a unit is not a measurement.
