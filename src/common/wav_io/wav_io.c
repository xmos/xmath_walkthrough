
// #include "xs3_math.h"

#include "wav_io.h"

#define CHANNEL_COUNT   (1)
#define BIT_DEPTH       (32)

#define PRINTERVAL      (1024)
#define FRAME_SIZE      (256)

#define MAX_WAV_BYTES   (150000)

static int wav_input_bytes = 0;
static uint8_t wav_input_buff[MAX_WAV_BYTES];
static uint8_t wav_output_buff[MAX_WAV_BYTES];


typedef struct {
  wav_header_t header;
  int32_t data[];
} wav_file_1chan_pcm32_t;

static wav_file_1chan_pcm32_t* wav_input = 
    (wav_file_1chan_pcm32_t*) &wav_input_buff[0];

static wav_file_1chan_pcm32_t* wav_output = 
    (wav_file_1chan_pcm32_t*) &wav_output_buff[0];


/**
 * 
 * 
 */
static 
int read_input_wav(
    const char* input_file_name )
{
  printf("Reading input: %s... ", input_file_name);

  file_t wav_input;
  const int ret = file_open(&wav_input, input_file_name, "rb");
  if(ret != 0) return ret;

  wav_input_bytes = get_file_size(&wav_input);
  if( wav_input_bytes > sizeof(wav_input_buff) ){
    printf("Input WAV file too large. (%d > %u)\n", 
           wav_input_bytes, sizeof(wav_input_buff));
    return 100;
  }

  file_read(&wav_input, (void*) &wav_input_buff[0], wav_input_bytes);

  printf("done.\n");
  
  return 0;
}

/**
 * 
 * 
 */
static
int write_output_wav(
    const char* output_file_name)
{
  printf("Writing: %s... ", output_file_name);

  // This is the only way that seems to consistently write out the file.
  FILE* tmp = fopen(output_file_name, "wb");
  for(int k = 0; k < wav_input_bytes; k++){
    fwrite(&wav_output_buff[k], 1, 1, tmp);
  }
  fclose(tmp);

  printf("done.\n");
  return 0;
}


/**
 * 
 * 
 */
int write_performance_info(
    const char* perf_file_name,
    const float ave_sample_time_ns,
    const float ave_frame_time_ns)
{
  file_t json_output;
  const int ret = file_open(&json_output, 
                            perf_file_name, 
                            "wb");
  if(ret != 0) return ret;

  char str_buff[100] = {0};
  unsigned c = sprintf(str_buff, 
      "{\n\"sample_time\": %0.02f,\n\"tap_time\": %0.02f\n,\n\"frame_time\": %0.02f\n}\n", 
      ave_sample_time_ns,
      ave_sample_time_ns / 1024,
      ave_frame_time_ns);
      
  file_write(&json_output, 
             str_buff, 
             c);

  file_close(&json_output);

  printf("Average sample time: %0.02f ns\n", ave_sample_time_ns);
  printf("Average tap time: %0.02f ns\n", ave_sample_time_ns / 1024);
  printf("Average frame time: %0.02f ns\n", ave_frame_time_ns);
  return 0;
}


/**
 * Sends a full frame to the next stage and gets the output.
 * The output samples will be placed in `frame`'s buffer.
 */
void send_frame(
    int32_t samples_out[],
    const int32_t samples_in[],
    const unsigned sample_count,
    const chanend_t c_audio)
{
  for(int s = 0; s < sample_count; s++)
    chan_out_word(c_audio, samples_in[s]);
  for(int s = sample_count; s < FRAME_SIZE; s++)
    chan_out_word(c_audio, 0);

  // Use a temp buffer just to make sure we don't overrun wav_output_buff[].
  int32_t out_buff[FRAME_SIZE];

  // We expect to get FRAME_SIZE samples back from the pipeline.
  for(int s = 0; s < FRAME_SIZE; s++)
    out_buff[s] = chan_in_word(c_audio);
  
  memcpy(&samples_out[0], &out_buff[0], sizeof(int32_t) * sample_count);
}


/**
 * 
 * 
 */
void wav_io_task(
  chanend_t c_audio, 
  chanend_t c_timing,
  const char* input_file_name, 
  const char* output_file_name,
  const char* perf_file_name)
{
  assert( !read_input_wav(input_file_name) );

  // Check that the wav file seems to contain what we expect.
  assert( !wav_header_check_details(&(wav_input->header), 
              CHANNEL_COUNT, 0, BIT_DEPTH) );

  // Copy the input header to the output header.
  wav_output->header = wav_input->header;

  const unsigned sample_count = wav_header_get_sample_count(&(wav_input->header));
  
  unsigned next_sample = 0;

  while(next_sample < sample_count){
    const unsigned samples_left = sample_count - next_sample;
    const unsigned iter_samples = (samples_left >= FRAME_SIZE)? FRAME_SIZE 
                                                              : samples_left;
    
    send_frame(&(wav_output->data[next_sample]), 
               &(wav_input->data[next_sample]),
               iter_samples, c_audio);
               
    next_sample += iter_samples;

    if(next_sample % PRINTERVAL == 0){
      printf("Processed %u samples..\n", next_sample);
    }
  }

  printf("Finished processing audio data.\n");

  // Write the output wav file
  assert( !write_output_wav(output_file_name) );

  // Get the timing info from tile1.
  chan_in_word(c_timing);
  chan_out_word(c_timing, 0);

  unsigned tmp = chan_in_word(c_timing);
  float sample_timing_ns = ((float*)&tmp)[0];
  tmp = chan_in_word(c_timing);
  float frame_timing_ns = ((float*)&tmp)[0];

  write_performance_info(perf_file_name, 
                         sample_timing_ns, 
                         frame_timing_ns);
}
