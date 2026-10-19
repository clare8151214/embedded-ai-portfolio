# crosscheck.py  -  the board's account of an event, next to the instrument's account.
# Usage:  python crosscheck.py fixed.txt <gap_us_measured_on_the_analyser>
#
# Week 3 you timed the chip with the chip and we said out loud that nobody was checking
# the referee. Today somebody is. This script does the subtraction and nothing else:
# it does not tell you which number is right.
#
# What it WILL refuse to do is subtract two numbers that were never measurements. An
# earlier version of this script would take a log in which the sketch halted before it
# ever reached the device, read settled=0, take -1 from the command line, and still print
# that the two clocks agreed. That is the failure this whole week exists to teach against,
# so the preconditions below are checked first and the verdict is withheld if any of them
# fails. A refusal names the precondition. It never names a number.
#
# ---------------------------------------------------------------------------------------
# WHAT "the same gap" MEANS. Both numbers must bracket the same two edges:
#   opens  at the STOP condition that ends the LAST configuration write
#   closes at the START condition of the first output-register read (register 0x3B)
# Sketch 4 is written so that nothing else - no register read-back, no Serial output -
# happens in between, and slide 45 asks you to put the analyser's cursors on those same
# two edges. The polling reads of INT_STATUS (0x3A) that sit inside the window appear on
# both accounts, so they cancel; they are distinguishable in the decoded trace by their
# register byte.
#
# TOLERANCE. Two independent clocks measuring one span of roughly thirty milliseconds:
#   the ESP32's micros() runs off a crystal-derived timer, tens of ppm
#   a 24 MHz analyser quantises each edge to +/- 1 sample, about 0.04 us each
# Both are negligible. What is not negligible is where your cursor lands and whether you
# put it on the right edge. So the threshold below is a judgement about cursor placement,
# not a metrological limit, and it is deliberately loose:
TOL_PCT    = 2.0     # per cent of the board's figure
TOL_ABS_US = 20.0    # ...or this many microseconds, whichever is the more forgiving
# A disagreement wider than that is not automatically an error. It is a result, and it is
# the most interesting line on your record sheet. Report it; do not tune this number.
# ---------------------------------------------------------------------------------------
import re, sys

if len(sys.argv) != 3:
    sys.exit("usage: python crosscheck.py <logfile> <gap_us_measured_on_the_analyser>")

log = open(sys.argv[1]).read()


def grab(pattern):
    m = re.search(pattern, log)
    return m.group(1) if m else None


who     = grab(r"who_am_i=(0x[0-9A-Fa-f]{2})")
expect  = grab(r"expected=(0x[0-9A-Fa-f]{2})")
settled = grab(r"settled=(\d)")
board   = grab(r"gap_us=(\d+)")
halt    = grab(r"(halt=\S+)")

try:
    analyser_us = float(sys.argv[2])
except ValueError:
    analyser_us = float("nan")

print("file                 ", sys.argv[1])
print("WHO_AM_I read        ", who, " expected", expect)
print("settled flag         ", settled)
print("halt line            ", halt if halt else "none")
print("gap, board  micros() ", board if board else "not printed")
print("gap, analyser        ", sys.argv[2])
print()

# ---- preconditions. Nothing is compared until all of these hold. ----
bad = []

if who is None or expect is None:
    bad.append("no who_am_i=/expected= pair in this file. This is not a sketch 4 log; "
               "sketch 3 does not print `expected=` because it never checks.")
else:
    w, e = int(who, 16), int(expect, 16)
    if w != e:
        bad.append("identity mismatch: read %s, expected %s. The device that answered is "
                   "not the part this build was written for." % (who, expect))
    elif w in (0x00, 0xFF):
        bad.append("identity is %s. That is what an undriven bus reads - 0x00 if something "
                   "holds the line low, 0xFF if the pull-ups win - and it is also what an "
                   "unfilled blank compares equal to. Both of these can agree with each "
                   "other while no device exists." % who)

if halt:
    bad.append("the sketch halted (%s). Everything printed after a halt line is from "
               "before it, and everything the halt prevented is missing." % halt)

if settled is None:
    bad.append("no settled= line. The sketch never reached the point where it says whether "
               "the device produced data.")
elif settled != "1":
    bad.append("settled=%s. The device never reported data ready, so there is no "
               "configuration-to-first-read interval to measure." % settled)

if board is None:
    bad.append("no gap_us= line. Sketch 4 prints one only when settled=1, which is the "
               "point: an absent number is better than a zero standing in for one.")
else:
    b = float(board)
    if b <= 0:
        bad.append("gap_us=%s. A span is strictly positive or it is not a span." % board)

if analyser_us != analyser_us:                      # NaN
    bad.append("the analyser figure %r is not a number. Type the microseconds the cursor "
               "readout showed." % sys.argv[2])
elif analyser_us <= 0:
    bad.append("the analyser figure is %g. Negative and zero are not durations; -1 is "
               "what a tool prints when it has nothing, and it must not become a "
               "measurement here." % analyser_us)

if bad:
    print("PRECONDITIONS NOT MET - %d:" % len(bad))
    for i, m in enumerate(bad, 1):
        print("  %d. %s" % (i, m))
    print()
    print("NO VERDICT. Two numbers were not compared, because at least one of them was")
    print("not a measurement. Fix the run, or write on the record sheet that you could")
    print("not make this comparison and which precondition stopped you. An honest gap in")
    print("section 7 scores; an agreement between two things that never happened does not.")
    sys.exit(2)

# ---- only now is there anything to subtract ----
b = float(board)
d = analyser_us - b
pct = 100.0 * d / b
tol = max(TOL_ABS_US, TOL_PCT / 100.0 * b)

print("gap, board  micros() ", "%10.1f us" % b)
print("gap, analyser        ", "%10.1f us" % analyser_us)
print("difference           ", "%10.1f us  (%+.2f%% of the board's number)" % (d, pct))
print("tolerance            ", "%10.1f us  (%.1f%% or %.0f us, whichever is larger)"
      % (tol, TOL_PCT, TOL_ABS_US))
print()
if abs(d) <= tol:
    print("AGREE, within the tolerance stated at the top of this file.")
else:
    print("DISAGREE by more than the tolerance stated at the top of this file.")
    print("Before you call it an error: check that both cursors are on the edges named")
    print("above, and that the capture was deep enough to contain the whole start-up.")
print()
print("Two clocks that never met produced the two numbers above. Say on the record")
print("sheet whether that is close enough for what you are claiming, and why.")
