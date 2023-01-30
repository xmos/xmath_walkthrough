// Copyright 2022-2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once

#include <stdint.h>

#include "fileio.h"

#define WAV_HEADER_BYTES 44

typedef struct {
    // RIFF Header
    uint8_t riff_header[4];    // Should be "RIFF"
    uint32_t wav_size;         // File size - 8 = data_bytes + WAV_HEADER_BYTES - 8
    uint8_t wave_header[4];    // Should be "WAVE"

    // Format Subsection
    uint8_t fmt_header[4];     // Should be "fmt "
    uint32_t fmt_chunk_size;   // Size of the rest of this subchunk
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;          // sample_rate * num_channels * (bit_depth/8)
    uint16_t sample_alignment;   // num_channels * (bit_depth/8)
    uint16_t bit_depth;          // bits per sample

    // Data Subsection
    uint8_t data_header[4];      // Should be "data"
    uint32_t data_bytes;         // frame count * num_channels * (bit_depth/8)
} wav_header_t;


unsigned wav_header_check_details(
    const wav_header_t* header,
    const uint16_t exp_channels,
    const uint32_t exp_sample_rate,
    const uint16_t exp_bit_depth);

unsigned wav_header_get_file_size(
    const wav_header_t* header);

unsigned wav_header_get_sample_count(
    const wav_header_t* header);
