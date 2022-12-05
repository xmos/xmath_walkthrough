
import numpy as np
import os
import json

perf_info = dict()

for stage_num in range(12):
  fname = f"out/stage{stage_num}.json"
  with open(fname) as fi:
    contents = json.load(fi)
    filter_time = contents["filter_time"]
    tap_time = contents["tap_time"]
    perf_info[stage_num] = (filter_time, tap_time)

print("Filter Times (1024 taps)")
for sn, info in perf_info.items():
  print(f"\tStage {sn}: {info[0]} ns")
