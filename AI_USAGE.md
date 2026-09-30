# AI usage

### 2026-09-24 · 250 Hz ECG sampling sketch

**Tool** Codex（本次對話）。本次生成時已討論過講義、Sketch B 與暫存器改寫；不是無背景的獨立模型測試。

**What I asked**
> Write me an Arduino program that samples an ECG at 250 Hz, reads analog pin A0, and prints the value over the serial port.

**What it answered**

生成 [sketches/sketch_ai_250hz.cpp](sketches/sketch_ai_250hz.cpp)，使用 `analogRead(A0)` 讀取、`Serial.println()` 輸出，並在每輪末尾呼叫 `delay()`。
完整回答及定時分析保存於 [w03/ai_250hz_response.md](w03/ai_250hz_response.md)。

將 250 Hz 換成毫秒的關鍵程式：

```cpp
constexpr unsigned int SAMPLE_RATE_HZ = 250;
constexpr unsigned long SAMPLE_INTERVAL_MS = 1000UL / SAMPLE_RATE_HZ;
// At the end of loop(), after analogRead() and Serial.println():
delay(SAMPLE_INTERVAL_MS);
```

AI 回答同時指出：4 ms 是工作後的等待時間，不保證實際 250 Hz。

**How I checked**

已上傳並執行生成程式，當時核對 `src/main.cpp` 與保存的生成版本一致；[串口日誌](logs/device-monitor-260924-113021.log) 收到 7928 筆 ADC 原始讀值，確認程式有輸出，但這些數值不是時間間隔，也不能直接證明訊號是 ECG 或取樣率達 250 Hz。

依課程流程，接著改跑套件的 `sketches/sketch_a_delay.cpp`：使用相同的 `delay(4)` 定時方式，加上課程統一負載，以 `micros()` 自我計時，收完 1000 筆才輸出；這是量測用版本，不是 AI 原始程式本身的逐字重跑。

對 [w03/a-delay.txt](w03/a-delay.txt) 執行 `scripts/read.py` 得到：n = 1000、mean = **5.1960 ms**、sample standard deviation = **1.0023 ms**、max deviation from 4.0000 ms = **7.0000 ms**；平均間隔對應約 **192.46 Hz**，因此在此統一負載下，`delay(4)` 沒有達成 250 Hz。

對照原版 Sketch B 的 [w03/b-timer.txt](w03/b-timer.txt)：n = 1000、mean = **4.0001 ms**、sample standard deviation = **0.0023 ms**、max deviation = **0.0730 ms**。B 的數字屬於 timer ISR 間隔，不是 AI 原始程式的結果，也不是實際 ADC 等間隔取樣的直接驗證。

**not verified**：AI 原始 ADC 輸出程式本身的取樣間隔與完整 HRV 表現，尚未直接量測；沒有把 ADC 讀值拿給 `read.py` 當作微秒計算。

**What I changed**

保留 `sketch_ai_250hz.cpp` 原始回答，將實際執行程式切換為課程提供的 A、B、C 量測版本，以統一負載和方法比較。由於 A 的平均間隔 5.1960 ms 與最大偏差 7.0000 ms，放棄把「等待 4 ms」視為「每 4 ms 取樣」的假設，並依此實驗選擇硬體 timer 作為後續定時方案；尚未把原始 AI 程式改造成或宣稱驗證過完整 HRV 系統。

### 2026-09-24 · register-level timer rewrite

**Tool** Codex（本次對話）。

**What I asked**
> Rewrite this ESP32-S3 timer setup at register level, without the IDF driver calls. Keep it firing at 250 Hz.

**What it answered**

把 Sketch B 的 timer 設定改成直接操作 ESP32-S3 Timer Group 0、Timer 0 暫存器，設定 APB 80 MHz、除頻 80、alarm 4000 ticks，即名目 250 Hz；保留原量測負載與 1000 筆間隔輸出。

以下為當時生成並上傳版本的關鍵片段（節錄，不是完整可單獨編譯的程式）：

```cpp
static constexpr uint32_t TIMER_CONFIG =
    (80U << TIMG_T0_DIVIDER_S) | TIMG_T0_INCREASE | TIMG_T0_AUTORELOAD;
static constexpr uint32_t ALARM_TICKS = 4000;

// Configure Timer 0 while stopped; USE_XTAL is clear, selecting APB.
REG_WRITE(TIMG_T0CONFIG_REG(0), TIMER_CONFIG);
REG_WRITE(TIMG_T0LOADHI_REG(0), 0);
REG_WRITE(TIMG_T0LOADLO_REG(0), 0);
REG_WRITE(TIMG_T0LOAD_REG(0), 1);
REG_WRITE(TIMG_T0ALARMHI_REG(0), 0);
REG_WRITE(TIMG_T0ALARMLO_REG(0), ALARM_TICKS);

// CPU interrupt allocation is still a system service, not a timer driver.
esp_err_t result = esp_intr_alloc(ETS_TG0_T0_LEVEL_INTR_SOURCE,
                                 ESP_INTR_FLAG_LEVEL1, onTick, nullptr,
                                 &timerInterrupt);

// After successful interrupt allocation:
REG_SET_BIT(TIMG_INT_ENA_TIMERS_REG(0), TIMG_T0_INT_ENA);
prev = micros();
REG_WRITE(TIMG_T0CONFIG_REG(0), TIMER_CONFIG | TIMG_T0_ALARM_EN | TIMG_T0_EN);

// In the ISR: acknowledge Timer 0, then re-arm while more samples are needed.
REG_WRITE(TIMG_INT_CLR_TIMERS_REG(0), TIMG_T0_INT_CLR);
// ... store interval and increment n ...
if (n < N) {
  REG_SET_BIT(TIMG_T0CONFIG_REG(0), TIMG_T0_ALARM_EN);
}
```

完整版本也包含群組時鐘啟用、80 MHz APB 檢查、中斷配置失敗處理、1000 筆後停止及中斷資源釋放；它沒有 Arduino／IDF timer driver 呼叫，但仍使用 `esp_intr_alloc()`／`esp_intr_free()` 管理 CPU 中斷，因此不是完全不使用 IDF 系統服務。

**How I checked**

當時以專案的 `espressif32@7.1.3`、Arduino core 2.0.17 編譯成功，Flash = **274,409 bytes**、RAM = **22,816 bytes**；也檢查了編譯後的符號位置，ISR 位於 IRAM，而 `micros()` 位於 flash，因此沒有設定 `ESP_INTR_FLAG_IRAM` 或宣稱整條 ISR 呼叫鏈都能在 flash cache 關閉時執行。

上板後保存 [logs/device-monitor-260924-111916.log](logs/device-monitor-260924-111916.log)，並以相同的 `scripts/read.py` 分析：n = **1000**、mean = **4.0000 ms**、sample standard deviation = **0.0001 ms**、max deviation = **0.0020 ms**；原始資料是首筆 **4002 us**，其餘 **999 筆為 4000 us**。這是在 `micros()` 的解析度與本次負載下，觀察到符合名目 4 ms 的 ISR 間隔，未使用外部儀器。

原版 B 的首筆為 4073 us，且起始時間戳位置與暫存器版不同；因此不能用兩次包含首筆的標準差直接斷言「暫存器一定更準」。顯示 4000 us 也不代表不存在更小尺度的抖動。

**What I changed**

把 `timerBegin()`、`timerAttachInterrupt()`、`timerAlarmWrite()`、`timerAlarmEnable()`／`timerAlarmDisable()` 改成直接的 timer 暫存器讀寫；明確處理 alarm 觸發後自動清除啟用位元的行為，在 ISR 內清除 Timer 0 中斷並重新啟用 alarm。初始時間戳移至完成設定、即將啟動計時器的位置，因此首筆不再包含全部設定耗時；這是量測方法差異，已在比較時註明。

保留原版 Sketch B 與其 `w03/b-timer.txt` 作為課程 A／B／C 比較基準，沒有用暫存器版結果覆蓋 B；測完後切回課程範例繼續 A／B／C 實驗。此改寫顯示底層控制需要自行處理更多細節，單次量測不足以支持為 HRV 改用暫存器版本。

**Scope note**：第二筆記錄的是實際完成的 **timer** 暫存器改寫，與 cycle 3 的 **GPIO toggle／Sketch D** 不同；Sketch D 後續於 2026-09-30 補測完成，過程記錄如下。

### 2026-09-30 · Sketch D: register-level GPIO toggle

**Tool** Codex（本次對話）。

**What I asked**
> 現在完成Sketch D

**What it answered**

AI 檢查課程提供的 [sketches/sketch_d_toggle.cpp](sketches/sketch_d_toggle.cpp)，其原本就包含 `digitalWrite()` 與 GPIO 暫存器兩種寫法；本次是檢查、調整執行環境並實測既有範例，不是以新的獨立提示詞從零生成暫存器版本。

回答說明：對 GPIO4 使用 `GPIO.out_w1ts` 設定 HIGH、`GPIO.out_w1tc` 設定 LOW，每次只寫入 bit 4 的遮罩，兩種方法各執行 100000 次高低切換迴圈。實際量測程式的暫存器片段如下：

```cpp
#include "soc/gpio_struct.h"
#define PIN 4
#define LOOPS 100000UL

// After pinMode(PIN, OUTPUT), inside the timed section:
for (uint32_t i = 0; i < LOOPS; i++) {
  GPIO.out_w1ts = (1UL << PIN);
  GPIO.out_w1tc = (1UL << PIN);
}
```

**How I checked**

編譯與上傳均成功，燒錄工具的 Flash hash 校驗通過；這些只驗證建置與傳輸，速度比較另以上板執行得到的兩行數據為依據。

在 ESP32-S3 的 COM7 原生 USB Serial/JTAG 擷取以下輸出，完整保存於 [w03/d-toggle.txt](w03/d-toggle.txt)：

```text
L2 digitalWrite : 63385 us for 100000 toggles -> 1577.66 kHz
L3 register     : 12602 us for 100000 toggles -> 7935.25 kHz
```

L3-to-L2 速度比 = L3 kHz / L2 kHz = L2 耗時 / L3 耗時 = **63385 / 12602 = 5.03 倍**（以未四捨五入的耗時計算）。兩段計時使用同一個 `micros()`，各迴圈都是 HIGH 後 LOW，沒有在計時區間內列印；此結果是一次自我計時的軟體執行成本比較，未以示波器或邏輯分析儀驗證接腳波形、邊緣或電氣品質。

**What I changed**

將 `src/main.cpp` 切換為 Sketch D，並同步在它與範例檔加上明確的 `soc/gpio_struct.h` 標頭。因本次連接的是原生 USB 而不是先前 A/B/C 使用的 UART 橋接介面，在 `platformio.ini` 啟用 `ARDUINO_USB_CDC_ON_BOOT=1`；為避免一次性結果在監視器連上前遺失，在量測前等待 USB 串口連線，量測輸出後呼叫 `Serial.flush()`。兩段計時迴圈及計時位置沿用原範例。

一般 RTS 重啟後未收到輸出，後續使用燒錄工具的 ESP32-S3 重啟功能清除強制下載狀態並以 watchdog reset 重啟，才擷取到上述實測兩行；沒有將無輸出的嘗試當作量測結果。

本次 L3 約快 5.03 倍，但會把 `digitalWrite()` 作為一般程式的預設寫法，只有在量測證實 GPIO 操作成為瓶頸、且可接受 ESP32-S3 平台相依性時才採用直接暫存器存取。
