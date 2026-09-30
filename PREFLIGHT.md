# Before class on Thursday 24 Sep

Two things matter tonight. The first takes five seconds, the second takes about twenty
minutes.

## 1 · Bring a USB-C cable that carries data

Not a charge-only cable. One you have actually used to move files or sync a phone with.

They look identical, they both charge, and a charge-only cable will silently stop your board
from being found. This was the most common failure in Week 2, and we only had twelve boards
then. **You get your own board tomorrow, so bring a cable.**

If you are not sure whether yours carries data, bring two.

## 2 · Try the software setup — but do not stay up for it

**Please try. If something fails, stop and come anyway — we will do it together in class.**
This is late notice and that is on us, so nothing below is a requirement. It is simply
faster for you if it is already done.

- [ ] Download `W03_student_pack.zip` from Moodle and unzip it somewhere you can find again
- [ ] Python 3.8 or newer
- [ ] `numpy` and `matplotlib`
- [ ] VS Code with PlatformIO working (**Mac users, see step 5 — yours needs a clean install**)

If a step fails, **post the whole error message in the course channel** — not just the last
line. Someone else has the same problem, and we would rather sort it out tonight than at
09:20 tomorrow.

---

## 3 · Python 3

```bash
python --version
```

You should see something like `Python 3.12.4`. Anything 3.8 or newer is fine. If you get an
error, or the version starts with `2.`, install from <https://www.python.org/downloads/>.

**Windows:** on the first screen of the installer, tick **"Add python.exe to PATH"**. It is
off by default, and if you miss it the terminal will not find `python` even though it is
installed. Already installed without it? Run the installer again, choose **Modify**, and tick
it there.

**Mac:** you may only have `python3`, not `python`. That is normal. Type `python3` wherever
the slides say `python`.

## 4 · numpy and matplotlib

```bash
python -m pip install numpy matplotlib
```

About 60 MB. Then check:

```bash
python -c "import numpy, matplotlib; print('ok')"
```

If it prints `ok`, this step is done.

**If this one fails, it costs you the least.** `read.py` uses only the standard library, so
cycles 1 and 2 work without it. Only cycle 3 needs these two packages, and that is the last
block of the morning.

## 5 · Mac users: reinstall VS Code, then install PlatformIO

Windows users skip this.

We hit real problems with PlatformIO on macOS while testing, and a clean install fixed them.
**Do this even if your VS Code seems fine** — it is much less painful tonight than in class.

**Uninstall**

1. Quit VS Code with `Cmd+Q`. Closing the window is not enough.
2. In Finder → **Applications**, drag **Visual Studio Code** to the Trash and empty it.
3. Delete the old settings and extensions:

   ```bash
   rm -rf ~/Library/Application\ Support/Code ~/.vscode
   ```

   This removes all your VS Code settings and extensions. That is the point — the new install
   starts clean. If you keep other work in VS Code and would rather not lose your setup, skip
   this and talk to a TA in class instead.

**Reinstall**

4. Download the Mac version from <https://code.visualstudio.com/download>.
5. Drag **Visual Studio Code** into **Applications** and open it from there, not from
   Downloads.

**Install PlatformIO**

6. Open Extensions (`Cmd+Shift+X`), search **PlatformIO IDE**, click **Install**.
7. Wait for it to finish. The first install downloads PlatformIO Core in the background and
   can take several minutes. There is a progress message in the bottom-right corner. Do not
   close VS Code until it is done, and restart when it asks.

It worked when an ant-head icon appears in the left sidebar and clicking it opens **PIO
Home**.

---

## What is in the pack, and why you want it before class

You do not have to read any of it tonight. Just have it unzipped.

| | |
|---|---|
| `platformio.ini` | The project settings. **Copy this over the one PlatformIO generates** — it names the right board and it makes the Serial Monitor save your data to a file automatically |
| `sketches/` | The four programs we run. You will not be typing code off the projector |
| `scripts/` | `read.py` and `floor.py` — the two scripts that turn your raw numbers into results |
| `RECORD_SHEET.md` | What you fill in as you go. This is the deliverable |
| `README.md` | What to do with all of the above |

**One thing worth knowing now:** the board we are using is the **ESP32-S3-DevKitC-1**, and it
has **two USB-C sockets**. Only one of them gives you a serial port. We will show you which
one in class — do not worry about it tonight.

## If you get stuck

Post in the course channel with your OS, the command you ran, and the full error message.

And then stop. Come to class. We will fix it there.
