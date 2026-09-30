# 250 Hz ECG sampling: AI prompt, response, and timing review

Date: 2026-09-24

Tool: Codex, this conversation.

Context: This response was generated after discussing the lecture, Sketch B,
and its register-level rewrite. It is not a separate, context-free model test.

## Prompt

> Write me an Arduino program that samples an ECG at 250 Hz, reads analog pin A0, and prints the value over the serial port.

## AI response

以下是 Arduino 簡單版本：讀取 A0，透過 115200 baud 的串口輸出，
再用 `delay()` 等待 4 ms。250 Hz 換算的理想間隔是 `1000 / 250 = 4 ms`。
不過，這個版本把 4 ms 當成每輪工作後的等待時間，因此不保證實際取樣率為 250 Hz。

```cpp
// AI-generated simple example: nominal 250 Hz using delay().
// The full loop period also includes ADC and serial-output time.
#include <Arduino.h>

constexpr int ECG_PIN = A0;
constexpr unsigned int SAMPLE_RATE_HZ = 250;
constexpr unsigned long SAMPLE_INTERVAL_MS = 1000UL / SAMPLE_RATE_HZ;

void setup() {
  Serial.begin(115200);
  pinMode(ECG_PIN, INPUT);
}

void loop() {
  int value = analogRead(ECG_PIN);
  Serial.println(value);
  delay(SAMPLE_INTERVAL_MS);
}
```

此程式輸出的是 ADC 原始數值；是否為 ECG 訊號取決於 A0 的輸入。

## Timing review

- **使用的定時方式：** `delay()`，在每輪讀取和輸出完成後等待。
- **把 250 轉成毫秒的程式：** `SAMPLE_INTERVAL_MS = 1000UL / SAMPLE_RATE_HZ`，結果是 4。
- **實際等待的位置：** `delay(SAMPLE_INTERVAL_MS)`，位於 `loop()` 最後。
- **理想週期：** 4.0000 ms。
- **實際週期：** 包含 ADC 讀取、串口輸出、等待及其他排程開銷；不是單靠 `delay(4)` 就能保證每 4 ms 取樣。
- **與 Sketch B 的差別：** 這份程式由主迴圈執行進度決定讀取時刻；Sketch B 由硬體 timer 排定中斷時刻。
- **Sketch B 的量測範圍：** 套件版本在 ISR 記時間戳，ADC 工作仍在主迴圈，所以它量的是中斷間隔，不是直接證明 ADC 也等間隔取樣。

## Verification status

更新至 2026-09-24：已上傳生成程式，[串口日誌](../logs/device-monitor-260924-113021.log) 有 7928 筆 ADC 原始讀值；這確認程式有輸出，不能用來直接驗證 250 Hz。

已另用套件提供的 `sketches/sketch_a_delay.cpp` 完成統一負載下的定時量測，保存 1000 筆間隔為 `w03/a-delay.txt`；以 `scripts/read.py` 算得 mean 5.1960 ms、sample standard deviation 1.0023 ms、max deviation 7.0000 ms（相對 4.0000 ms 目標）。

**not verified**：原始 AI 程式本身的實際取樣間隔及完整 HRV 表現尚未直接量測；上述統計來自量測用 Sketch A，`b-timer.txt` 則是原版 Sketch B 的量測，兩者都不是原始 AI ADC 日誌的時間統計。

可在專案根目錄重現 Sketch A 的分析：

```powershell
.\.venv\Scripts\python.exe scripts\read.py w03\a-delay.txt
```

這個正式量測版本有統一的模擬工作負載，並在量完後才輸出數據；
它用來比較定時方式，與上面的 AI 原始程式並非逐字相同。

生成程式另存於 [sketch_ai_250hz.cpp](../sketches/sketch_ai_250hz.cpp)。
