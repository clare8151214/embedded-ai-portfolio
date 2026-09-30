# AI_USAGE.md — the two W03 entries

Append these to the `AI_USAGE.md` already in your repo from W02. Same four fields as
last week: **what I asked · what it answered · how I checked · what I changed.**

The third field is the one that is marked. An entry whose "how I checked" says
"it compiled" or "it looked right" scores nothing — the whole point of this week is that
both versions compile and both look right.

---

## Entry 1 — the 250 Hz sketch (cycle 1)

```markdown
### 2026-09-24 · 250 Hz ECG sampling sketch

**Tool** GitHub Copilot Chat / <model name and version>

**What I asked**
> Write me an Arduino program that samples an ECG at 250 Hz, reads analog pin A0,
> and prints the value over the serial port.

**What it answered**
<paste the sketch it gave you, or the relevant lines. Name the line that turned
250 into a number of milliseconds.>

**How I checked**
<Not "it compiled". What did you measure, with what, and what number came out?
e.g. "Stripped it to the schedule only, self-timed 1000 intervals with micros(),
ran read.py: mean X ms, std dev Y ms, max deviation Z ms.">

**What I changed**
<What you did about it, and why — in terms of the numbers above.>
```

## Entry 2 — the register-level rewrite (cycle 3)

```markdown
### 2026-09-24 · register-level GPIO toggle

**Tool** <as above>

**What I asked**
> <your prompt, verbatim>

**What it answered**
<the register version it produced>

**How I checked**
<the two printed lines from Sketch D, and the ratio>

**What I changed**
<and whether you would actually use it — a faster toggle is not automatically
the right answer>
```

---

## Why the third field is the whole assignment

Slide 25 is the wall this week runs into:

> Both versions run. Both look reasonable. You cannot tell which one is correct by
> reading them.

Which means "I read it and it seemed fine" is not a check — it is the exact failure the
session is built to demonstrate. A check is something that produces a number you did not
have before. This week that number is a standard deviation.

If you genuinely could not check something, write **"not verified"** and say why. That is
an honest entry and it is marked as one. An unchecked claim presented as a checked one is
not.
