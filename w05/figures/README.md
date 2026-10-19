# W05 figure sources

Seven PNGs are genuine Windows Notepad screenshots made on 2026-10-19 from saved serial logs, actual analysis stdout and saved build-log excerpts. They are cropped to the relevant content; no fake terminal interface or retyped measurement image was generated. The screenshot source map and hashes are in provenance.json. Original raw logs remain unchanged.

Four PNGs contain regions rendered directly from the supplied original Register Map rev. 4.2: WHO_AM_I p.45, gyro full scale p.14, accelerometer full scale p.15, and sensitivity p.29. The p.14/p.15 images join two separated original regions; captions disclose that. These are PDF excerpts, not claims of a new instrument acquisition.

The build display file contains verbatim RAM, Flash and SUCCESS lines from all three 2026-10-15 build logs, with descriptive labels. The bus-table display file contains the actual script stdout from the preserved buses.txt. Other screenshot displays opened the original source log directly. Existing physical PulseView WHO_AM_I, SCL and gap screenshots remain in the parent directory.

Homework 5 now contains images in every Figures box: 14 images total, across 13 visually reviewed pages. Prior text transcriptions in figure bodies were replaced; question/answer wording and measured values were preserved.

## Figure 7b endpoint views

The revised gap-figure-7b.png combines three genuine PulseView screenshot crops of the same physical capture: a 30.028 ms overview, the final 0x38=0x01 configuration STOP at 2.349362 s, and the first 0x3B-read START at 2.379390 s. SDA (D0), SCL (D1), decoder rows and cursor readings remain visible. Overview and detail time scales differ. Headings are outside the unmodified crops. All three full JPEG screenshots and the individual PNG crops are retained here; provenance.json records source and asset hashes. This re-presents the existing capture and does not claim a new hardware measurement.
