import numpy as np
import xscope_fileio
import os
from pathlib import Path
import argparse

SCRIPT_DIR = (Path(__file__).parent)
OUT_DIR = (SCRIPT_DIR / "out")

if __name__ == "__main__":
  parser = argparse.ArgumentParser()

  parser.add_argument("adapter_id", help="Adapter ID of device")
  parser.add_argument("stages", nargs='+', type=str, default=[], help="Name of stage to run (e.g. stage0)")

  # parser.add_argument("wav_in", default=None, help=
  # "wav file to use as input. This file is copied to input.wav prior to running the firmware")

  args = parser.parse_args()

  firmware_map = {stage: os.path.join("bin", stage + ".xe") for stage in args.stages}

  # Verify firmware exists for each stage before executing anything.
  for stage,firmware in firmware_map.items():
    assert os.path.exists(firmware), f"Firmware for {stage} not found ({firmware}). Did you build and install?"

  # Ensure output directory exists
  os.makedirs(OUT_DIR, exist_ok=True)

  # Run the app
  for stage,firmware in firmware_map.items():
    xscope_fileio.run_on_target(args.adapter_id, 
                                firmware, 
                                use_xsim=False)