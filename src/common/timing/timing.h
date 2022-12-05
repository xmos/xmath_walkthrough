
#pragma once

#include <stdint.h>

#ifndef __XC__
# include <xcore/hwtimer.h>
# include <xcore/channel.h>
# include <xcore/chanend.h>
#endif

typedef enum {
  TIMING_SAMPLE = 0,
  TIMING_FRAME = 1,
} timing_type_e;

void timer_start(const timing_type_e type);
void timer_stop(const timing_type_e type);
float timer_avg_ns(const timing_type_e type);

#ifdef __XC__
extern "C" {
void timer_report_task(chanend c_timing);
}
#else
void timer_report_task(chanend_t c_timing);
#endif