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
#include "pico/stdlib.h"
#include "peripherals/peripherals.h"

#define PDM_BUFFER_SIZE 512 //Bytes, 2 bytes per sample

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

} // namespace

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
  int num_read =
    PDM.read(g_audio_capture_buffer + capture_index, PDM_BUFFER_SIZE);
  if (num_read != PDM_BUFFER_SIZE) {
    MicroPrintf("### short read (%d/%d) @%dms", num_read,
                PDM_BUFFER_SIZE, time_in_ms);
    while (1);
  }
  // This is how we let the outside world know that new audio data has arrived.
  g_latest_audio_timestamp = time_in_ms;

  // uint32_t now = to_ms_since_boot(get_absolute_time());
  // printf("now = %u\n", now);
}

TfLiteStatus InitAudioRecording() {
  if (!g_is_audio_initialized) {
    // Set the buffer size (in bytes) used by the PDM interface
    PDM.setBufferSize(PDM_BUFFER_SIZE);
    // Hook up the callback that will be called with each sample
    PDM.onReceive(CaptureSamples);
    // Gain: -20db (min) + 6.5db (13) + 3.2db (builtin) = -10.3db
    PDM.setGain(20);
    // Start listening for audio: MONO @ 16KHz
    if (!PDM.begin(1, kAudioSampleFrequency)){
        MicroPrintf("PDM microphone initialization failed!");
      while (1);
    }

    // Block until we have our first audio sample
    while (!g_latest_audio_timestamp) {
      tight_loop_contents(); 
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