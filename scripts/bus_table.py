# bus_table.py  -  turns the six rows your board printed into the trade-off table.
# Usage:  python bus_table.py buses.txt
#
# Nothing in here knows what a "fast" bus is. It divides bytes by seconds and by pins,
# and prints what comes out. The judgement is yours and it goes on the record sheet.
#
# What it refuses to do is turn a transfer that never happened into a speed. Two things
# make a row not a measurement, and both are visible in the log:
#   - the sketch printed `no_ack_at=` above the table: nothing answered on I2C, so the two
#     I2C rows are bus time-outs, not transfers. They are printed as a refusal, not a rate.
#   - elapsed_us is 0: the clock did not run across the transfer, so there is nothing to
#     divide by. Same treatment.
# A row whose bytes_ok is short of the payload still gets its rate, but it is flagged: the
# elapsed time of that row includes up to 100 ms of receive time-out per 60-byte block, so
# the rate is of the whole attempt, not of the bytes that arrived.
import re, sys

FIELDS = 6
rows = []
text = open(sys.argv[1]).read()
no_ack = re.search(r"no_ack_at=(0x[0-9A-Fa-f]{2})", text)
for line in text.splitlines():
    p = [c.strip() for c in line.strip().split(",")]
    if len(p) != FIELDS or not p[2].isdigit() or not p[3].isdigit():
        continue                       # header, banner, sink= line: all ignored
    bus, setting = p[0], p[1]
    payload, us, pins, ok = int(p[2]), int(p[3]), int(p[4]), int(p[5])
    rows.append((bus, setting, payload, us, pins, ok))

if not rows:
    print("no data rows found in", sys.argv[1])
    sys.exit(1)

if no_ack:
    print("NOTE: the sketch printed no_ack_at=%s. Nothing acknowledged on I2C, so the two"
          % no_ack.group(1))
    print("      I2C rows below are time-outs, not transfers, and are not given a rate.")
    print("      Fix the bus (Run 1), take Run 2 again, and run this on the new log.")
    print()

hdr = ("bus", "setting", "pins", "kB/s", "kbit/s", "kbit/s/pin", "us/6B", "payload")
print("%-6s %-15s %5s %10s %10s %11s %9s %10s" % tuple(hdr))
refused = 0
for bus, setting, payload, us, pins, ok in rows:
    reason = None
    if bus == "i2c" and no_ack:
        reason = "no ACK at %s: a time-out, not a transfer" % no_ack.group(1)
    elif us == 0:
        reason = "elapsed_us=0: the transfer did not happen, nothing to divide by"
    if reason:
        print("%-6s %-15s %5d %10s %10s %11s %9s %10s   <- NOT A MEASUREMENT: %s"
              % (bus, setting, pins, "-", "-", "-", "-", "-", reason))
        refused += 1
        continue
    s = us / 1e6
    kBs = payload / s / 1000.0
    kbits = kBs * 8
    per_pin = kbits / pins if pins else float("nan")
    per_frame = us / (payload / 6.0)
    flag = ""
    if ok < 0:
        checked = "not check"          # a real device answered, so we cannot predict the bytes
    else:
        checked = "%d/%d" % (ok, payload)
        if ok < payload:
            flag = "   <- short: elapsed includes receive time-outs, rate is of the attempt"
    print("%-6s %-15s %5d %10.1f %10.1f %11.1f %9.1f %10s%s"
          % (bus, setting, pins, kBs, kbits, per_pin, per_frame, checked, flag))

print()
print("us/6B is the time to move one six-byte sample frame: the number that decides")
print("whether a 104 Hz sensor fits inside your loop, and the one to put on the sheet.")
if refused:
    print()
    print("%d row(s) were not given a rate. Copy the reason onto the record sheet instead of"
          % refused)
    print("a number; an empty cell with a reason beside it is a result, a time-out is not.")
    sys.exit(2)
