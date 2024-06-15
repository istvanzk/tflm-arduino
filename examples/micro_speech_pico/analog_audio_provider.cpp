/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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
src/analog_audio_provider.cpp
*/

#include "audio_provider.h"

#ifdef ANALOG_MIC
#pragma message("Using analog microphone")

#include <stdio.h>
#include "tensorflow/lite/micro/micro_log.h"
#include "micro_model_settings.h"
#include "peripherals/peripherals.h"

// The following implementation works only on RP2040 based boards
// with support for DMA-based transfer of ADC samples
#ifdef RP2040_BOARD
#include "pico/stdlib.h"

#define ADC_PIN 26
#define CAPTURE_CHANNEL 0

#define ADC_BIAS_VOLTAGE 1.65 //V
//#define ADC_BIAS ((int16_t)((ADC_BIAS_VOLTAGE * 4095) / 3.3)) //bits
#define ADC_BUFFER_SIZE 256 //Samples

namespace {

bool g_is_audio_initialized = false;
// An internal buffer able to fit 16x our sample size
constexpr int kAudioCaptureBufferSize = ADC_BUFFER_SIZE * 16;
int16_t g_audio_capture_buffer[kAudioCaptureBufferSize];
// A buffer that holds our output
int16_t g_audio_output_buffer[kMaxAudioSampleSize];
// Mark as volatile so we can check in a while loop to see if
// any samples have arrived yet.
volatile int32_t g_latest_audio_timestamp = 0;

const struct peripherals::analog_microphone_config config = {
    // GPIO to use for input, must be ADC compatible (GPIO 26 - 28)
    .gpio = ADC_PIN + CAPTURE_CHANNEL,

    // bias voltage of microphone in volts
    .bias_voltage = ADC_BIAS_VOLTAGE,

    // sample rate in Hz
    .sample_rate = kAudioSampleFrequency,

    // number of samples to buffer
    .sample_buffer_size = ADC_BUFFER_SIZE,
};
} // namespace

void CaptureSamples() {
  // This is how many samples of new data we expect to have each time this is called
  const int number_of_samples = ADC_BUFFER_SIZE;
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
    peripherals::analog_microphone_read(g_audio_capture_buffer + capture_index, ADC_BUFFER_SIZE);
  if (num_read != ADC_BUFFER_SIZE) {
    MicroPrintf("### short read (%d/%d) @%dms", num_read,
                ADC_BUFFER_SIZE, time_in_ms);
    while (1) { tight_loop_contents(); }
  }
  // This is how we let the outside world know that new audio data has arrived.
  g_latest_audio_timestamp = time_in_ms;
}

TfLiteStatus InitAudioRecording() {
  if (!g_is_audio_initialized) {
    // initialize the analog microphone
    if (peripherals::analog_microphone_init(&config) < 0) {
        MicroPrintf("Analog microphone initialization failed!");
      while (1) { tight_loop_contents(); }
    }

    // set callback that is called when all the samples in the library
    // internal sample buffer are ready for reading
    peripherals::analog_microphone_set_samples_ready_handler(CaptureSamples);

    // start capturing data from the analog microphone
    if (peripherals::analog_microphone_start() < 0) {
      MicroPrintf("Analog microphone start failed");
      while (1) { tight_loop_contents();  }
    }

    // Block until we have our first audio sample
    while (!g_latest_audio_timestamp) {
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

#else // RP2040_BOARD
#error "RP2040_BOARD not defined"
#endif // RP2040_BOARD
#else // ANALOG_MIC
#pragma message("Analog microphone not used")
//#error "ANALOG_MIC not defined"
#endif // ANALOG_MIC