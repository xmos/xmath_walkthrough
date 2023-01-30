# Copyright 2022-2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import wave
from scipy.io import wavfile

CHANNEL_COUNT = 1
SAMPLE_RATE = 16000 # Sample/sec
DURATION_SEC = 1
DURATION_SMP = DURATION_SEC * SAMPLE_RATE

sig = np.zeros(DURATION_SMP, dtype=float)

time = np.linspace(0, DURATION_SEC, DURATION_SMP)

freq = 1000 # Hz
# sig = 0.75 * np.cos(2*np.pi*freq*time)
sig = np.random.rand(DURATION_SMP)-0.5

sig = np.round(sig * np.ldexp(1, 31)).astype(np.int32)

wavfile.write("input.wav", SAMPLE_RATE, sig)



