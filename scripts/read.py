# read.py  -  turn your raw intervals into the three numbers you will be asked for.
# Usage:  python read.py a-delay.txt
import sys, statistics as st

us = [int(x) for x in open(sys.argv[1]) if x.strip().isdigit()]
ms = [v / 1000.0 for v in us]                 # microseconds -> milliseconds
target = 4.0                                  # 250 Hz, by definition

print("file            ", sys.argv[1])
print("n               ", len(ms))
print("mean       (ms) ", round(st.mean(ms), 4))
print("stdev      (ms) ", round(st.stdev(ms), 4))     # sample stdev - say so in your report
print("max dev    (ms) ", round(max(abs(v - target) for v in ms), 4))
print("min / max  (ms) ", round(min(ms), 4), round(max(ms), 4))
