/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

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

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#include <PDM.h>

#include "utility.h"

// The preprocessor definition format in Arduino IDE is:
//    ARDUINO_<PROCESSOR-DESCRIPTOR>_<BOARDNAME>
// The <PROCESSOR-DESCRIPTOR>_<BOARDNAME> for the installed boards
// can be listed by running in the base Arduino directory, the following command:
//    grep board= `find . -name boards.txt` | cut -f2 -d= | sort -u
// E.g. ARDUINO_NANO_RP2040_CONNECT or ARDUINO_SPARKFUN_MICROMOD_RP2040
#if defined(ARDUINO_SPARKFUN_MICROMOD_RP2040) || defined(ARDUINO_NANO_RP2040_CONNECT)
#define RP2040_BOARD

#include <cstdint>
#include "led.h"
#include "analog_microphone.h"

namespace peripherals {

constexpr pin_size_t kI2S_BIT_CLK = D18;
constexpr pin_size_t kI2S_LR_CLK = D19;
constexpr pin_size_t kI2S_DATA_IN = D21;
//constexpr pin_size_t kI2S_DATA_OUT = D22;
constexpr uint32_t kI2S_IRQ_PRIORITY = 7;

constexpr uint32_t kI2C_CLOCK = 100000;

constexpr pin_size_t kLED_DEFAULT_GPIO = D25;

}  // namespace peripherals

#else
#error "Unsupported board! Not a RP2040 based board selected!"
#endif // defined(ARDUINO_SPARKFUN_MICROMOD_RP2040) || defined(ARDUINO_NANO_RP2040_CONNECT)

#else  // ARDUINO
#error "These peripherals implementations are for ARDUINO IDE framework!"
#endif  // ARDUINO

#endif  // PERIPHERALS_H_
