/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/lite/micro/system_setup.h"

#include <limits>
#include <cstdarg>
#include <cstdint>
#include <cstring>

#include "tensorflow/lite/micro/debug_log.h"

// The preprocessor definition format in Arduino IDE is:
//    ARDUINO_<PROCESSOR-DESCRIPTOR>_<BOARDNAME>
// The <PROCESSOR-DESCRIPTOR>_<BOARDNAME> for the installed boards 
// can be listed by running in the base Arduino directory, the following command:
//    grep board= `find . -name boards.txt` | cut -f2 -d= | sort -u
// E.g. ARDUINO_ARDUINO_NANO33BLE or ARDUINO_SPARKFUN_MICROMOD_RP2040
#if defined(ARDUINO) && !defined(ARDUINO_SPARKFUN_MICROMOD_RP2040)
#define ARDUINO_EXCLUDE_CODE
#endif  // defined(ARDUINO) && !defined(ARDUINO_SPARKFUN_MICROMOD_RP2040)

#ifndef ARDUINO_EXCLUDE_CODE

#include <Arduino.h>
#include <api/RingBuffer.h>

// The Arduino DUE uses a different object for the default serial port shown in
// the monitor than most other models, so make sure we pick the right one. See
// https://github.com/arduino/Arduino/issues/3088#issuecomment-406655244
#if defined(__SAM3X8E__)
#define DEBUG_SERIAL_OBJECT (SerialUSB)
#else
#define DEBUG_SERIAL_OBJECT (Serial)
#endif

#ifndef TF_LITE_STRIP_ERROR_STRINGS
#include <stdio.h>

#include "pico/stdlib.h"
#endif  // TF_LITE_STRIP_ERROR_STRINGS

namespace tflite {

void InitializeTarget(const int baud) {
#ifndef TF_LITE_STRIP_ERROR_STRINGS
// Not needed. See https://github.com/earlephilhower/arduino-pico/issues/1433#issuecomment-1546783109%22
//  stdio_init_all();
DEBUG_SERIAL_OBJECT.begin(baud);
#endif  // TF_LITE_STRIP_ERROR_STRINGS
}

}  // namespace tflite

extern "C" void DebugLog(const char* format, va_list args) {
#ifndef TF_LITE_STRIP_ERROR_STRINGS
  // Reusing TF_LITE_STRIP_ERROR_STRINGS to disable DebugLog completely to get
  // maximum reduction in binary size. This is because we have DebugLog calls
  // via TF_LITE_CHECK that are not stubbed out by TF_LITE_REPORT_ERROR.
  //vfprintf(stderr, format, args);
  DEBUG_SERIAL_OBJECT.printf(format, args);
#endif
}

#ifndef TF_LITE_STRIP_ERROR_STRINGS
// Only called from MicroVsnprintf (micro_log.h)
int DebugVsnprintf(char* buffer, size_t buf_size, const char* format,
                   va_list vlist) {
  return vsnprintf(buffer, buf_size, format, vlist);
}
#endif


namespace test_over_serial {

// Change baud rate on default serial port
// void SerialChangeBaudRate(const int baud) {
//   DEBUG_SERIAL_OBJECT.begin(baud);
//   ulong start_time = millis();
//   while (!DEBUG_SERIAL_OBJECT) {
//     // allow for Arduino IDE Serial Monitor synchronization
//     if (millis() - start_time > tflite::kSerialMaxInitWait) {
//       break;
//     }
//   }
// }
// The RingBufferN class, which is used for buffering incoming serial data, is defined in the ArduinoCore-API.
class _RingBuffer : public RingBufferN<kSerialMaxInputLength + 1> {
 public:
  bool need_reset = false;
};

static _RingBuffer _ring_buffer;

// SerialReadLine
// Read a set of ASCII characters from the default
// serial port.  Data is read up to the first newline ('\\n') character.
// This function uses an internal buffer which is automatically reset.
// The buffer will not contain the newline character.
// The buffer will be zero ('\\0') terminated.
// The <timeout> value is in milliseconds.  Any negative value means that
// the wait for data will be forever.
// Returns std::pair<size_t, char*>.
// The first pair element is the number of characters in buffer not including
// the newline character or zero terminator.
// Returns {0, NULL} if the timeout occurs.
std::pair<size_t, char*> SerialReadLine(int timeout) {
  if (_ring_buffer.need_reset) {
    _ring_buffer.need_reset = false;
    _ring_buffer.clear();
  }

  ulong start_time = millis();

  while (true) {
    int value = DEBUG_SERIAL_OBJECT.read();
    if (value >= 0) {
      if (value == '\n') {
        // read a newline character
        _ring_buffer.store_char('\0');
        _ring_buffer.need_reset = true;
        break;
      } else {
        // read other character
        _ring_buffer.store_char(value);
        if (_ring_buffer.availableForStore() == 1) {
          // buffer is full
          _ring_buffer.store_char('\0');
          _ring_buffer.need_reset = true;
          break;
        }
      }
    }
    if (timeout < 0) {
      // wait forever
      continue;
    } else if (millis() - start_time >= static_cast<ulong>(timeout)) {
      // timeout
      return std::make_pair(0UL, reinterpret_cast<char*>(NULL));
    }
  }

  return std::make_pair(static_cast<size_t>(_ring_buffer.available() - 1),
                        reinterpret_cast<char*>(_ring_buffer._aucBuffer));
}

// SerialWrite
// Write the ASCII characters in <buffer> to the default serial port.
// The <buffer> must be zero terminated.
void SerialWrite(const char* buffer) { DEBUG_SERIAL_OBJECT.print(buffer); }

}  // namespace test_over_serial

#endif  // ARDUINO_EXCLUDE_CODE
