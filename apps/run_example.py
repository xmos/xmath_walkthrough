import numpy as np
import xscope_fileio
import os
import argparse


if __name__ == "__main__":
  parser = argparse.ArgumentParser()

  parser.add_argument("adapter_id", help="Adapter ID of device")
  parser.add_argument("stages", nargs='+', type=str, default=[], help="Name of stage to run (e.g. stage0)")

  # parser.add_argument("wav_in", default=None, help=
  # "wav file to use as input. This file is copied to input.wav prior to running the firmware")

  args = parser.parse_args()

  for stage in args.stages:
    firmware = os.path.join("bin", stage + ".xe")
    xscope_fileio.run_on_target(args.adapter_id, 
                                firmware, 
                                use_xsim=False)