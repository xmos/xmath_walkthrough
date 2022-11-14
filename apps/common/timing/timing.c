
#include "timing.h"

static uint32_t t_start = 0;

static unsigned t_count = 0;
static uint64_t t_total = 0;

// ignore this many samples before starting to track them
// this will ensure the avg is closer to the steady state timing
static unsigned ignore_next = 2048; 

void timer_start()
{
  t_start = get_reference_time();
}

void timer_stop()
{
  if(ignore_next){
    ignore_next--;
    return;
  }

  uint32_t dur = get_reference_time() - t_start;
  t_count++;
  t_total += dur;
}

// Get the average execution time in nanoseconds.
float timer_avg_ns()
{
  // reference clock freq is 100 MHz, so 1 tick is 10 ns
  return (t_total / ((float)t_count)) * (10.0f);
}


void timer_report_task(
    chanend_t c_timing)
{
  // This task will wait until the tile0 task is finished and then report the
  // timing number once it's done.

  // Simple handshake to make sure they're synchronized
  chan_out_word(c_timing, 0);
  chan_in_word(c_timing);

  float timing_ns = timer_avg_ns();
  chan_out_word(c_timing, ((unsigned*) &timing_ns)[0]);
}