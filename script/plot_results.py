
import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile
import os

SAMPLE_RATE = 16000 # Sample/sec

signals = dict()

if True:
  fname = "output-ref.wav"
  fpath = os.path.join("wav", fname)
  if os.path.exists(fpath):
    rate, samples = wavfile.read(fpath)
    assert rate == SAMPLE_RATE, f"Sample rate of {fname} was wrong. Was {rate}, expected {SAMPLE_RATE}."
    signals['ref'] = samples

for k in range(13):
  fname = f"output-stage{k}.wav"
  fpath = os.path.join("wav", fname)
  if not os.path.exists(fpath):
    continue
  rate, samples = wavfile.read(fpath)
  assert rate == SAMPLE_RATE, f"Sample rate of {fname} was wrong. Was {rate}, expected {SAMPLE_RATE}."
  signals[k] = samples

plt.figure()

if 'ref' in signals:
  sig = signals['ref']
  xx = np.arange(len(sig))
  plt.plot(xx, sig, label=f"Reference Output", marker='o')

for stage, sig in signals.items():
  if stage=='ref': continue
  xx = np.arange(len(sig))
  plt.plot(xx, sig, label=f"Stage {stage}")

plt.xlabel("Sample Index")
plt.ylabel("Sample Value")
plt.title("Filtered Waveforms")
plt.legend()
plt.grid()
plt.show()

