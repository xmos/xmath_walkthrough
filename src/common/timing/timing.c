
#include "timing.h"

#define TYPE_COUNT    (2)

static 
uint32_t t_start[TYPE_COUNT] = {0};

static 
unsigned t_count[TYPE_COUNT] = {0};

static 
uint64_t t_total[TYPE_COUNT] = {0};

// ignore this many frames before starting to track them
// this will ensure the avg is closer to the steady state timing
static 
unsigned ignore_next_frames = 8; 

void timer_start(
    const timing_type_e type)
{
  t_start[type] = get_reference_time();
}

void timer_stop(
    const timing_type_e type)
{
  if(ignore_next_frames){
    if(type == TIMING_FRAME) ignore_next_frames--;
    return;
  }

  uint32_t dur = get_reference_time() - t_start[type];
  t_count[type]++;
  t_total[type] += dur;
}

// Get the average execution time in nanoseconds.
float timer_avg_ns(
    const timing_type_e type)
{
  // reference clock freq is 100 MHz, so 1 tick is 10 ns
  return (t_total[type] / ((float)t_count[type])) * (10.0f);
}


void timer_report_task(
    chanend_t c_timing)
{
  // This task will wait until the tile0 task is finished and then report the
  // timing numbers once it's done.

  // Simple handshake to make sure they're synchronized
  chan_out_word(c_timing, 0);
  chan_in_word(c_timing);

  float sample_timing_ns = timer_avg_ns(TIMING_SAMPLE);
  float frame_timing_ns = timer_avg_ns(TIMING_FRAME);

  chan_out_word(c_timing, ((unsigned*) &sample_timing_ns)[0]);
  chan_out_word(c_timing, ((unsigned*) &frame_timing_ns)[0]);
}