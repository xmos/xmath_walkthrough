// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "fileio.h"
#include "wav_utils.h"

#ifndef DEBUG
# define DEBUG    (0)
#endif
#define DEBUG_PRINT   if(DEBUG) printf("[wav_utils.c:%u]: ", __LINE__); \
                      if(DEBUG) printf


#define RIFF_SECTION_SIZE (12)
#define FMT_SUBCHUNK_MIN_SIZE (24)
#define EXTENDED_FMT_GUID_SIZE (16)
// static const char wav_default_header[WAV_HEADER_BYTES] = {
//         0x52, 0x49, 0x46, 0x46,
//         0x00, 0x00, 0x00, 0x00,
//         0x57, 0x41, 0x56, 0x45,
//         0x66, 0x6d, 0x74, 0x20,
//         0x10, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x00, 0x00, 0x00, 0x00,
//         0x64, 0x61, 0x74, 0x61,
//         0x00, 0x00, 0x00, 0x00,
// };

unsigned wav_header_check_details(
    const wav_header_t* header,
    const uint16_t exp_channels,
    const uint32_t exp_sample_rate,
    const uint16_t exp_bit_depth)
{
  if( memcmp(header->riff_header, "RIFF", sizeof(header->riff_header)) != 0 )
    return 1;

  if( memcmp(header->wave_header, "WAVE", sizeof(header->wave_header)) != 0 ) 
    return 2;
  
  if( memcmp(header->fmt_header, "fmt ", sizeof(header->fmt_header)) != 0 ) 
    return 3;
  
  DEBUG_PRINT("%lu\n", header->fmt_chunk_size);
  if( header->fmt_chunk_size != 0x00000010 )
    return 4;

  DEBUG_PRINT("%u\n", header->num_channels);
  if(exp_channels && (exp_channels != header->num_channels))
    return 5;
  

  DEBUG_PRINT("%lu\n", header->sample_rate);
  if(exp_sample_rate && (exp_sample_rate != header->sample_rate))
    return 6;


  DEBUG_PRINT("%u\n", header->bit_depth);
  if(exp_bit_depth && (exp_bit_depth != header->bit_depth))
    return 7;

  const uint32_t byte_depth = header->bit_depth >> 3;

  DEBUG_PRINT("0x%04X\n", (unsigned) header->audio_format);
  if(header->audio_format != 0x0001)
    return 8;


  DEBUG_PRINT("%lu\n", header->byte_rate);
  const uint32_t exp_byte_rate = 
      (byte_depth * header->num_channels * header->sample_rate);
  if(header->byte_rate != exp_byte_rate)
    return 9;


  DEBUG_PRINT("%u\n", header->sample_alignment);
  const uint16_t exp_sample_alignment = (byte_depth * header->num_channels);
  if(header->sample_alignment != exp_sample_alignment)
    return 10;


  DEBUG_PRINT("%lu\n", header->wav_size);
  if(header->wav_size != (header->data_bytes + WAV_HEADER_BYTES - 8))
    return 11;

  return 0;
}

unsigned wav_header_get_file_size(
    const wav_header_t* header)
{
  return header->wav_size + 8;
}

unsigned wav_header_get_sample_count(
    const wav_header_t* header)
{
  const unsigned byte_depth = header->bit_depth >> 3;
  const unsigned bytes_per_sample = byte_depth * header->num_channels;
  return header->data_bytes / bytes_per_sample;
}

