
// #include "xs3_math.h"

#include "wav_proc.h"
#include "framing.h"

#define CHANNEL_COUNT   (1)
#define FRAME_ADVANCE   (1)

#define BUFFER_WORDS     (32)

// Number of samples processed so far
unsigned proc_samples = 0;

#define PRINTERVAL      (1024)

#define FRAME_SIZE      (256)

unsigned frame_buffer_mem[FRAME_BUFF_WORDS(FRAME_SIZE)];

// Sends a full frame to the next stage and gets the output.
// The output samples will be placed in `frame`'s buffer.
void send_frame(
    const chanend_t c_pcm_out,
    const chanend_t c_pcm_in,
    frame_buffer_t* frame)
{
  // If we've filled up the frame, send the frame to the next thread
  // for processing.
  for(int s = 0; s < FRAME_SIZE; s++)
    chan_out_word(c_pcm_out, frame->data[s]);

  // Reset the frame buffer for the next frame
  frame_reset(frame, FRAME_SIZE);

  // We expect to get FRAME_SIZE samples back from the pipeline.
  for(int s = 0; s < FRAME_SIZE; s++)
    frame->data[s] = chan_in_word(c_pcm_in);
}

void write_performance_info(
    unsigned stage_num, 
    float ave_filter_time_ns)
{
  file_t perf_file;
  char fn_buff[30];
  sprintf(fn_buff, "perf/stage%u.json", stage_num);
  file_open(&perf_file, fn_buff, "wb");

  char str_buff[100] = {0};
  unsigned c = sprintf(str_buff, "{\n\"filter_time\": %0.02f,\n\"tap_time\": %0.02f\n}\n", 
                    ave_filter_time_ns,
                    ave_filter_time_ns / 1024);
  file_write(&perf_file, str_buff, c);

  file_close(&perf_file);
}


void wav_io_thread(
  chanend_t c_pcm_out, 
  chanend_t c_pcm_in, 
  chanend_t c_timing,
  unsigned stage_number,
  const char* input_file_name, 
  const char* output_file_name)
{
  file_t input_file, output_file;

  frame_buffer_t* frame = (frame_buffer_t*) &frame_buffer_mem[0];
  frame_reset(frame, FRAME_SIZE);

  // Open input wav file containing mic and ref channels of input data
  int ret = file_open(&input_file, input_file_name, "rb");
  assert((!ret) && "Failed to open file");

  // Open output wav file that will contain the AEC output
  ret = file_open(&output_file, output_file_name, "wb");
  assert((!ret) && "Failed to open file");

  wav_header input_header_struct, output_header_struct;
  unsigned input_header_size;
  if( get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0) {
    printf("error in get_wav_header_details()\n");
    _Exit(1);
  }

  file_seek(&input_file, input_header_size, SEEK_SET);

  // Ensure 32bit wav file
  if( input_header_struct.bit_depth != 32 ) {
    printf("Error: unsupported wav bit depth (%d) for %s file. Only 32 supported\n", 
        input_header_struct.bit_depth, input_file_name);
    _Exit(1);
  }

  // Ensure input wav file contains correct number of channels 
  if( input_header_struct.num_channels != CHANNEL_COUNT ){
    printf("Error: wav channel count (%d) must be %u\n", 
        input_header_struct.num_channels, CHANNEL_COUNT);
    _Exit(1);
  }
  
  unsigned sample_count = wav_get_num_frames(&input_header_struct);

  wav_form_header(&output_header_struct,
      input_header_struct.audio_format,
      CHANNEL_COUNT,
      input_header_struct.sample_rate,
      input_header_struct.bit_depth,
      sample_count);

  file_write(&output_file, 
      (uint8_t*) (&output_header_struct),  
      WAV_HEADER_BYTES);

  int32_t input_read_buffer[BUFFER_WORDS] = {0};

  for(unsigned sample_start = 0; sample_start < sample_count; 
                                 sample_start += BUFFER_WORDS) {

    // Words to be grabbed this iteration
    const unsigned iter_words = ((sample_count - sample_start) >= BUFFER_WORDS)? 
        BUFFER_WORDS : (sample_count - sample_start);

    long input_location = wav_get_frame_start(&input_header_struct,
                                              sample_start, 
                                              input_header_size);

    file_seek(&input_file, input_location, SEEK_SET);
    file_read(&input_file, 
              (uint8_t*) &input_read_buffer[0], 
              iter_words * sizeof(int32_t));
    
    for(int k = 0; k < BUFFER_WORDS; k++){

      // Add the next sample to the frame.
      if(frame_add_sample(frame, input_read_buffer[k])){

        // Send frame to next stage and get result. The contents of frame's
        // buffer will be updated with output samples.
        send_frame(c_pcm_out, c_pcm_in, frame);
        
        // Write output samples back to the host
        file_write(&output_file, (uint8_t*) &frame->data[0], 
                   FRAME_SIZE * sizeof(int32_t));
      }

      proc_samples++;
      if(proc_samples % PRINTERVAL == 0){
        printf("Processed %u samples..\n", proc_samples);
      }
    }
  }

  // If we've reached here, we've processed all the samples in the wav file. If
  // the frame buffer has a partial frame, pad the rest out with zeros and 
  // process one final frame.
  unsigned smp_left = frame->current;
  if(frame_finish(frame)){
    // Send frame to next stage and get result. The contents of frame's
    // buffer will be updated with output samples.
    send_frame(c_pcm_out, c_pcm_in, frame);
    
    // Write output samples back to the host, but only write back
    // as many as we had input samples.
    file_write(&output_file, (uint8_t*) &frame->data[0], 
                smp_left * sizeof(int32_t));
  }

  file_close(&input_file);
  file_close(&output_file);
  shutdown_session();

  // Get the timing info from tile1.

  // Handshake to ensure synchronized
  chan_in_word(c_timing);
  chan_out_word(c_timing, 0);

  unsigned tmp = chan_in_word(c_timing);
  float timing_ns = ((float*)&tmp)[0];
  write_performance_info(stage_number, timing_ns);
  printf("Average filter time: %0.02f ns\n", timing_ns);
}