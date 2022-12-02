
import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile
from scipy.signal import lfilter
import os
import sys
import argparse

TAP_COUNT = 1024
FRAME_SIZE = 256
SAMPLE_RATE = 16000  # Sample/sec


def run(args):

  # Get the input waveform
  input_signal = wavfile.read(os.path.join(
      "xmath_walkthrough", "wav", "input.wav"))[1]
  N = len(input_signal)
  dex = np.arange(N)

  # Convert to float using exponent -31
  float_signal = np.ldexp(input_signal, -31)

  # Create filter coefficients
  coef: np.ndarray = np.ones(TAP_COUNT, dtype=float) / TAP_COUNT

  # Compute reference output
  ref_out = lfilter(coef, [1.0], float_signal)
  ref_out_pcm = np.round(np.ldexp(ref_out, 31)).astype(np.int32)

  # Do each stage
  for stage in args.stages:
    spath = os.path.join("out", f"output-{stage}.wav")
    if not os.path.exists(spath):
      print(f"Couldn't find {spath}. Skipping.")
      continue

    stage_res = wavfile.read(spath)[1]

    fig, (ax1, ax2) = plt.subplots(2, 1)

    ax1.plot(dex, ref_out_pcm, label="Ref (PCM)")
    ax1.plot(dex, stage_res, label=f"{stage}")
    ax1.set_title(f"{stage} Output vs Reference")
    ax1.set_xlabel("Sample Index")
    ax1.set_ylabel("Sample Value")
    ax1.grid()
    ax1.legend()

    ax2.plot(dex, stage_res - ref_out_pcm, label="Diff")
    ax2.set_title(f"{stage} Output - Reference")
    ax2.set_xlabel("Sample Index")
    ax2.set_ylabel("Sample Diff")
    ax2.grid()

    plt.tight_layout()
    plt.show()


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("--stages", default='all', type=str,
                      help="The stage name (e.g. 'stage1')")
  args = parser.parse_args()
  if args.stages == 'all':
    args.stages = [f"stage{d}" for d in range(13)]
  else:
    args.stages = [args.stages]

  print(f"Plotting {', '.join(args.stages)}..")
  run(args)
