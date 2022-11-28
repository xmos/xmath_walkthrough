
#pragma once

#include <stdint.h>

#ifndef __XC__
# include <xcore/hwtimer.h>
# include <xcore/channel.h>
# include <xcore/chanend.h>
#endif


void timer_start();
void timer_stop();
float timer_avg_ns();

#ifdef __XC__
extern "C" {
void timer_report_task(chanend c_timing);
}
#else
void timer_report_task(chanend_t c_timing);
#endif