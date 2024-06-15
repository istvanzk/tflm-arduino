/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================

Based on: https://github.com/henriwoodcock/pico-wake-word/
src/pdm_audio_provider.cpp
*/

#include "audio_provider.h"

#ifdef PDM_MIC
#pragma message("Using PDM microphone")

#include <stdio.h>
#include "tensorflow/lite/micro/micro_log.h"
#include "micro_model_settings.h"
#include "peripherals/peripherals.h"

#ifdef RP2040_BOARD
// For RP20240 based boards the custom implementation for 
// PDM microphones is used (see src/peripherals/pdm_microphone)
#include "pico/stdlib.h"

#define GPIO_DATA_PIN D2
#define GPIO_CLK_PIN D3

#else
// The Arduino PDM library allows you to use Pulse-density modulation microphones, 
// such as the onboard MP34DT05 on Arduino Nano 33 BLE Sense boards
// See: https://docs.arduino.cc/learn/built-in-libraries/pdm/
#include <PDM.h>
#endif // RP2040_BOARD

#define PDM_BUFFER_SIZE 256 //Bytes, 2 bytes per sample

namespace {

bool g_is_audio_initialized = false;
// An internal buffer able to fit 16x our sample size
constexpr int kAudioCaptureBufferSize = PDM_BUFFER_SIZE * 8;
int16_t g_audio_capture_buffer[kAudioCaptureBufferSize];
// A buffer that holds our output
int16_t g_audio_output_buffer[kMaxAudioSampleSize];
// Mark as volatile so we can check in a while loop to see if
// any samples have arrived yet.
volatile int32_t g_latest_audio_timestamp = 0;

#ifdef RP2040_BOARD
const struct peripherals::pdm_microphone_config pdm_config = {
    .gpio_data = GPIO_DATA_PIN,
    .gpio_clk = GPIO_CLK_PIN,
    .pio = pio0,
    .pio_sm = 0,
    .sample_rate = kAudioSampleFrequency,
    .sample_buffer_size = PDM_BUFFER_SIZE / 2,
};
#endif // RP2040_BOARD
}  // namespace

void CaptureSamples() {
  // This is how many samples of new data we expect to have each time this is called
  // 16-bit, 2 bytes per sample
  const int number_of_samples = PDM_BUFFER_SIZE / 2;
  // Calculate what timestamp the last audio sample represents
  const int32_t time_in_ms =
      g_latest_audio_timestamp +
      (number_of_samples / (kAudioSampleFrequency / 1000));
  // Determine the index, in the history of all samples, of the last sample
  const int32_t start_sample_offset =
      g_latest_audio_timestamp * (kAudioSampleFrequency / 1000);
  // Determine the index of this sample in our ring buffer
  const int capture_index = start_sample_offset % kAudioCaptureBufferSize;
  // Read the data to the correct place in our buffer
#ifdef RP2040_BOARD
  int num_read =
    peripherals::pdm_microphone_read(g_audio_capture_buffer + capture_index, number_of_samples);
  if (num_read != number_of_samples) {
    MicroPrintf("### short read (%d/%d) @%dms", num_read,
                PDM_BUFFER_SIZE, time_in_ms);
    while (1) { tight_loop_contents(); }
  }
#else
  int num_read =
    PDM.read(g_audio_capture_buffer + capture_index, PDM_BUFFER_SIZE);
  if (num_read != PDM_BUFFER_SIZE) {
    MicroPrintf("### short read (%d/%d) @%dms", num_read,
                PDM_BUFFER_SIZE, time_in_ms);
    while (1);
  }
#endif // RP2040_BOARD
  // This is how we let the outside world know that new audio data has arrived.
  g_latest_audio_timestamp = time_in_ms;

  // uint32_t now = to_ms_since_boot(get_absolute_time());
  // printf("now = %u\n", now);
}

TfLiteStatus InitAudioRecording() {
  if (!g_is_audio_initialized) {
#ifdef RP2040_BOARD
    if (peripherals::pdm_microphone_init(&pdm_config) < 0) {
        MicroPrintf("PDM microphone initialization failed!");
      while (1) { tight_loop_contents(); }
    }

    // peripherals::pdm_microphone_set_filter_gain(20);
    // peripherals::pdm_microphone_set_filter_max_volume(128);
    peripherals::pdm_microphone_set_samples_ready_handler(CaptureSamples);

    if (peripherals::pdm_microphone_start() < 0) {
      MicroPrintf("PDM microphone start failed");
      while (1) { tight_loop_contents(); }
    }
#else
    // Set the buffer size (in bytes) used by the PDM interface
    PDM.setBufferSize(PDM_BUFFER_SIZE);
    // Hook up the callback that will be called with each sample
    PDM.onReceive(CaptureSamples);
    // Start listening for audio: MONO @ 16KHz
    if (!PDM.begin(1, kAudioSampleFrequency)){
        MicroPrintf("PDM microphone initialization failed!");
      while (1);
    }
    // gain: -20db (min) + 6.5db (13) + 3.2db (builtin) = -10.3db
    PDM.setGain(13);
#endif // RP2040_BOARD

    // Block until we have our first audio sample
    while (!g_latest_audio_timestamp) {
  #ifdef RP2040_BOARD
      tight_loop_contents(); 
  #endif    
    }
    g_is_audio_initialized = true;
  }

  return kTfLiteOk;
}

TfLiteStatus GetAudioSamples(int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {
  // Set everything up to start receiving audio
  if (!g_is_audio_initialized) {
    TfLiteStatus init_status = InitAudioRecording();
    if (init_status != kTfLiteOk) {
      return init_status;
    }
    g_is_audio_initialized = true;
  }
  // This next part should only be called when the main thread notices that the
  // latest audio sample data timestamp has changed, so that there's new data
  // in the capture ring buffer. The ring buffer will eventually wrap around and
  // overwrite the data, but the assumption is that the main thread is checking
  // often enough and the buffer is large enough that this call will be made
  // before that happens.

  // Determine the index, in the history of all samples, of the first
  // sample we want
  const int start_offset = start_ms * (kAudioSampleFrequency / 1000);
  // Determine how many samples we want in total
  const int duration_sample_count =
      duration_ms * (kAudioSampleFrequency / 1000);
  for (int i = 0; i < duration_sample_count; ++i) {
    // For each sample, transform its index in the history of all samples into
    // its index in g_audio_capture_buffer
    const int capture_index = (start_offset + i) % kAudioCaptureBufferSize;
    // Write the sample to the output buffer
    g_audio_output_buffer[i] = g_audio_capture_buffer[capture_index];
  }

  // Set pointers to provide access to the audio
  *audio_samples_size = kMaxAudioSampleSize;
  *audio_samples = g_audio_output_buffer;

  return kTfLiteOk;
}

int32_t LatestAudioTimestamp() { return g_latest_audio_timestamp; }

#else
#pragma message("PDM microphone not used")
//#error "PDM_MIC not defined"
#endif  // PDM_MIC