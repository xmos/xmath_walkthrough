

#include "framing.h"

#include <assert.h>

void frame_reset(
    frame_buffer_t* fb,
    const unsigned frame_size)
{
  fb->size = frame_size;
  fb->current = 0;
}


unsigned frame_add_sample(
    frame_buffer_t* fb,
    const int32_t sample)
{
  assert(fb->current < fb->size);

  fb->data[fb->current] = sample;
  fb->current++;

  return (fb->current == fb->size);
}


unsigned frame_finish(
    frame_buffer_t* fb)
{
  if(fb->current == 0) return 0;
  if(fb->current == fb->size) return 1;

  while(!frame_add_sample(fb, 0)){}
  fb->current = fb->size;
  return 1;
}