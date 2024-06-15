/* Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.

Modified by IzK to be used in Arduino IDE, June 2024.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef _PICO_ANALOG_MICROPHONE_H_
#define _PICO_ANALOG_MICROPHONE_H_

namespace peripherals {

namespace {

typedef void (*analog_samples_ready_handler_t)(void);

} // namespace

struct analog_microphone_config {
    uint gpio;
    float bias_voltage;
    uint sample_rate;
    uint sample_buffer_size;
};

int analog_microphone_init(const struct analog_microphone_config* config);
void analog_microphone_deinit();

int analog_microphone_start();
void analog_microphone_stop();

void analog_microphone_set_samples_ready_handler(analog_samples_ready_handler_t handler);

int analog_microphone_read(int16_t* buffer, size_t samples);

} // namespace peripherals

#endif // _PICO_ANALOG_MICROPHONE_H_
