# W05 completed laboratory and Homework 5

鄭宇峻 · NQ6141054 · 無分組。原始量測：2026-10-08；FX2 獨立擷取與收尾：2026-10-19。

[完成版 Homework 5](Embedded%20System%20-%20Homework%20Week%205%20-%20NQ6141054%20-%20Completed.docx)、[實驗紀錄](RECORD_SHEET.md)、[AI 使用聲明](AI_USAGE.md)。

Figures 已於 2026-10-19 改為圖片：7 張原始日誌／分析輸出畫面、4 張原始資料表節錄，加上 3 張既有 PulseView 截圖；全卷 13 頁、14 張圖片已逐頁檢查。來源與雜湊見 [figures/](figures/)。

## 實驗結果與證據

| 項目 | 結果 | 原始檔 |
|---|---|---|
| IMU 掃描 | 一台，0x68 | [scan.txt](scan.txt) |
| 六組匯流排比較 | 全部保留；UART 錯配只有 320/1200 bytes 驗證成功 | [buses.txt](buses.txt) |
| 初始化比較 | Sketch 3 全零；Sketch 4 喚醒、等待、查驗後讀到約 1 g | [ai-init.txt](ai-init.txt)、[fixed.txt](fixed.txt) |
| Flash / RAM | Sketch 4 比 Sketch 3 多 1092 / 0 bytes | 三份 verified / Sketch 3 build log |
| WHO_AM_I 實體波形 | D0 ACK 75 ACK repeated START D1 ACK 68 NACK STOP；408.0 µs | [whoami.sr](whoami.sr)、[完整畫面](whoami-capture.png) |
| SCL 游標 | 10.0 µs，100.0 kHz | [scl-period-cursors.png](scl-period-cursors.png) |
| 相同事件的兩個時鐘 | ESP32 29974 µs，FX2 30028 µs，差 +54 µs / +0.18%，AGREE | [同次 reset 串口](whoami-20261019-165030-serial.txt)、[游標](gap-cursors.png)、[原樣輸出](crosscheck-20261019-165030.txt) |
| 串流三組 | 1096.0、3727.0、1000.0 reads/s，全部 failed=0 | [stream.txt](stream.txt) |

2 MHz 擷取有 0.5 µs 的時間解析度；上述一致性不代表絕對時基準確度或加速度校正。讀取速率也不等於感測器的新樣本率。Flash 差比投影片的差多 16 bytes，原因未確立。

## 重現已存資料的分析

在 repository 根目錄，以 Python 3 執行；三個分析腳本僅使用標準函式庫，不需要接板子：

```powershell
python scripts/bus_table.py w05/buses.txt
python scripts/analyse_capture.py w05/whoami.sr w05/whoami-20261019-165030-serial.txt
python scripts/crosscheck.py w05/whoami-20261019-165030-serial.txt 30028
```

預期輸出為四個線上位元組 D0、75、D1、68，SCL 10.0 µs，gap 30028 µs，第一幀 (718, -466, 9144) 與串口完全相同，交叉檢查 AGREE。`analyse_capture.py` 從 .sr 的數位樣本重新解碼，未使用預先產生的 decoded JSON 充當輸入。

用 PulseView 開啟 `w05/whoami.sr`，加入 I²C decoder：SDA=D0，SCL=D1。SCL 游標為 2.347412 → 2.347422 s；gap 游標為 2.349362 → 2.379390 s，從最後一筆 D0 38 01 設定寫入的 STOP 到第一筆 D0 3B 的 START，中間的 0x3A poll 包含在內。

## 程式與方法

已填好的 Sketch 4 在 [project/src/main.cpp](project/src/main.cpp)，[project/platformio.ini](project/platformio.ini) 固定 espressif32@7.1.3。可在根目錄執行 `pio run -d w05/project`；上傳時接 ESP32 的 COM USB，若有多個連接埠須明確指定自己的 port。

所有 W05 範例在 [../sketches/w05/](../sketches/w05/)。`sketch_4_init_fixed.cpp` 是原始空白模板；可用的填寫版是 `sketch_4_init_fixed_filled.cpp`，與目前 main.cpp 的 SHA256 相同。

Native PulseView 仍無法直接開啟這台 FX2，錯誤是 Failed to open device / generic/unspecified error。本次用官方 libusb 1.0.30 及獨立非同步 USB reader 取得真實 1000 萬筆樣本，再交由 PulseView 解碼；未更換 Windows driver。Codex 先找出樣本位置，再放置 PulseView 游標並檢查實際讀值，完整畫面均保留。這次未用 VIMU 備援。

[method/capture_fx2.py](method/capture_fx2.py) 是當日實際使用的擷取程式原檔，保留當日的路徑、日期與 COM6 設定，並非可直接重跑的安裝包；執行前須重新指定輸出根目錄、COM port 和 libusb DLL / Python 相依套件位置。USB DLL 與套件未納入提交。

`whoami-20261019-165030.json` 保留取得方式、DLL hash、錯誤清單與原始樣本 SHA256；.sr 已包含全部 10,000,000 個原始位元組，故不另放重複的 10 MB .bin。分析腳本會重算內部原始樣本的 hash。完整 PNG 為原始畫面的無損轉檔；報告中的裁切有標明，未修改波形內容。

## 繳交狀態

文件、實驗資料、波形、討論與兩筆 AI 使用紀錄已完成。本機 Git 提交與遠端推送是不同步驟；實際 commit hash 另見本機收尾狀態。尚未取得課程繳交網址，這次尚未送出 Google Form／課程作業。GitHub 存檔也不能當作已送出課程表單。

Figure 7b has been revised with an overview and both endpoint zooms: [standalone PNG](figures/gap-figure-7b.png). The three genuine full screenshots and source hashes are retained in figures/. The same physical capture and 30028 us interval are unchanged.
