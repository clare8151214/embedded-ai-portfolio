# floor.py  -  the same clean tone, read at YOUR two sets of sampling instants.
# Usage:  python floor.py a-delay.txt     then     python floor.py b-timer.txt
import sys, numpy as np, matplotlib.pyplot as plt

# digits-only, exactly like read.py, so a monitor log banner cannot break it
us   = np.array([int(x) for x in open(sys.argv[1]) if x.strip().isdigit()], float)
t    = np.cumsum(us) * 1e-6               # the instants you actually sampled at
sig  = np.sin(2 * np.pi * 10.0 * t)       # a perfect 10 Hz tone, read at those instants

win  = np.hanning(len(sig))               # window first, or its own leakage dominates
spec = 20 * np.log10(np.abs(np.fft.rfft(sig * win)) + 1e-12)
spec -= spec.max()                        # 0 dB = the tone, so both files share one scale
freq = np.fft.rfftfreq(len(sig), d=np.mean(us) * 1e-6)

band = (freq > 30) & (freq < 120)         # far from the tone: floor and nothing else
print(sys.argv[1], " floor (dB below tone): ", round(float(np.median(spec[band])), 2))

plt.plot(freq, spec); plt.xlabel("Hz"); plt.ylabel("dB below tone"); plt.title(sys.argv[1])
plt.ylim(-140, 5); plt.xlim(0, 125)       # FIXED axes. Auto-scaled plots cannot be compared
plt.savefig(sys.argv[1] + ".png", dpi=150)
