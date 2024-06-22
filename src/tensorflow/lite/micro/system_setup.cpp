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

// IzK: From old (up to 2023) version of the micro_string.cpp
#if !defined(TF_LITE_STRIP_ERROR_STRINGS)
// Only pulling in the implementation of these functions for builds where we
// expect to make use of detailed error logging 
// to be extra cautious about not increasing the code size.

namespace {

// Int formats can need up to 10 bytes for the value plus a single byte for the
// sign.
constexpr int kMaxIntCharsNeeded = 10 + 1;
// Hex formats can need up to 8 bytes for the value plus two bytes for the "0x".
constexpr int kMaxHexCharsNeeded = 8 + 2;

// Float formats can need up to 7 bytes for the fraction plus 3 bytes for "x2^"
// plus 3 bytes for the exponent and a single sign bit.
constexpr float kMaxFloatCharsNeeded = 7 + 3 + 3 + 1;

// All input buffers to the number conversion functions must be this long.
const int kFastToBufferSize = 48;

// Reverses a zero-terminated string in-place.
char* ReverseStringInPlace(char* start, char* end) {
  char* p1 = start;
  char* p2 = end - 1;
  while (p1 < p2) {
    char tmp = *p1;
    *p1++ = *p2;
    *p2-- = tmp;
  }
  return start;
}

// Appends a string to a string, in-place. You need to pass in the maximum
// string length as the second argument.
char* StrCatStr(char* main, int main_max_length, const char* to_append) {
  char* current = main;
  while (*current != 0) {
    ++current;
  }
  char* current_end = main + (main_max_length - 1);
  while ((*to_append != 0) && (current < current_end)) {
    *current = *to_append;
    ++current;
    ++to_append;
  }
  *current = 0;
  return current;
}

// Populates the provided buffer with an ASCII representation of the number.
char* FastUInt32ToBufferLeft(uint32_t i, char* buffer, int base) {
  char* start = buffer;
  do {
    int32_t digit = i % base;
    char character;
    if (digit < 10) {
      character = '0' + digit;
    } else {
      character = 'a' + (digit - 10);
    }
    *buffer++ = character;
    i /= base;
  } while (i > 0);
  *buffer = 0;
  ReverseStringInPlace(start, buffer);
  return buffer;
}

// Populates the provided buffer with an ASCII representation of the number.
char* FastInt32ToBufferLeft(int32_t i, char* buffer) {
  uint32_t u = i;
  if (i < 0) {
    *buffer++ = '-';
    u = -u;
  }
  return FastUInt32ToBufferLeft(u, buffer, 10);
}

// Converts a number to a string and appends it to another.
char* StrCatInt32(char* main, int main_max_length, int32_t number) {
  char number_string[kFastToBufferSize];
  FastInt32ToBufferLeft(number, number_string);
  return StrCatStr(main, main_max_length, number_string);
}

// Converts a number to a string and appends it to another.
char* StrCatUInt32(char* main, int main_max_length, uint32_t number, int base) {
  char number_string[kFastToBufferSize];
  FastUInt32ToBufferLeft(number, number_string, base);
  return StrCatStr(main, main_max_length, number_string);
}

// Populates the provided buffer with ASCII representation of the float number.
// Avoids the use of any floating point instructions (since these aren't
// supported on many microcontrollers) and as a consequence prints values with
// power-of-two exponents.
char* FastFloatToBufferLeft(float f, char* buffer) {
  char* current = buffer;
  char* current_end = buffer + (kFastToBufferSize - 1);
  // Access the bit fields of the floating point value to avoid requiring any
  // float instructions. These constants are derived from IEEE 754.
  const uint32_t sign_mask = 0x80000000;
  const uint32_t exponent_mask = 0x7f800000;
  const int32_t exponent_shift = 23;
  const int32_t exponent_bias = 127;
  const uint32_t fraction_mask = 0x007fffff;
  uint32_t u;
  memcpy(&u, &f, sizeof(int32_t));
  const int32_t exponent =
      ((u & exponent_mask) >> exponent_shift) - exponent_bias;
  const uint32_t fraction = (u & fraction_mask);
  // Expect ~0x2B1B9D3 for fraction.
  if (u & sign_mask) {
    *current = '-';
    current += 1;
  }
  *current = 0;
  // These are special cases for infinities and not-a-numbers.
  if (exponent == 128) {
    if (fraction == 0) {
      current = StrCatStr(current, (current_end - current), "Inf");
      return current;
    } else {
      current = StrCatStr(current, (current_end - current), "NaN");
      return current;
    }
  }
  // 0x007fffff (8388607) represents 0.99... for the fraction, so to print the
  // correct decimal digits we need to scale our value before passing it to the
  // conversion function. This scale should be 10000000/8388608 = 1.1920928955.
  // We can approximate this using multiply-adds and right-shifts using the
  // values in this array. The 1. portion of the number string is printed out
  // in a fixed way before the fraction, below.
  const int32_t scale_shifts_size = 13;
  const int8_t scale_shifts[13] = {3,  4,  8,  11, 13, 14, 17,
                                   18, 19, 20, 21, 22, 23};
  uint32_t scaled_fraction = fraction;
  for (int i = 0; i < scale_shifts_size; ++i) {
    scaled_fraction += (fraction >> scale_shifts[i]);
  }
  *current = '1';
  current += 1;
  *current = '.';
  current += 1;
  *current = 0;

  // Prepend leading zeros to fill in all 7 bytes of the fraction. Truncate
  // zeros off the end of the fraction. Every fractional value takes 7 bytes.
  // For example, 2500 would be written into the buffer as 0002500 since it
  // represents .00025.
  constexpr int kMaxFractionalDigits = 7;

  // Abort early if there is not enough space in the buffer.
  if (current_end - current <= kMaxFractionalDigits) {
    return current;
  }

  // Pre-fill buffer with zeros to ensure zero-truncation works properly.
  for (int i = 1; i < kMaxFractionalDigits; i++) {
    *(current + i) = '0';
  }

  // Track how large the fraction is to add leading zeros.
  char* previous = current;
  current = StrCatUInt32(current, (current_end - current), scaled_fraction, 10);
  int fraction_digits = current - previous;
  int leading_zeros = kMaxFractionalDigits - fraction_digits;

  // Overwrite the null terminator from StrCatUInt32 to ensure zero-trunctaion
  // works properly.
  *current = '0';

  // Shift fraction values and prepend zeros if necessary.
  if (leading_zeros != 0) {
    for (int i = 0; i < fraction_digits; i++) {
      current--;
      *(current + leading_zeros) = *current;
      *current = '0';
    }
    current += kMaxFractionalDigits;
  }

  // Truncate trailing zeros for cleaner logs. Ensure we leave at least one
  // fractional character for the case when scaled_fraction is 0.
  while (*(current - 1) == '0' && (current - 1) > previous) {
    current--;
  }
  *current = 0;
  current = StrCatStr(current, (current_end - current), "*2^");
  current = StrCatInt32(current, (current_end - current), exponent);
  return current;
}

int FormatInt32(char* output, int32_t i) {
  return static_cast<int>(FastInt32ToBufferLeft(i, output) - output);
}

int FormatUInt32(char* output, uint32_t i) {
  return static_cast<int>(FastUInt32ToBufferLeft(i, output, 10) - output);
}

int FormatHex(char* output, uint32_t i) {
  return static_cast<int>(FastUInt32ToBufferLeft(i, output, 16) - output);
}

int FormatFloat(char* output, float i) {
  return static_cast<int>(FastFloatToBufferLeft(i, output) - output);
}

}  // namespace

// IzK: From old (up to 2023) version of the micro_string.cpp, MicroVsnprintf()
// Function changed to match declaration in debug_log.h
extern "C" int DebugVsnprintf(char* output, size_t len, const char* format,
                              va_list args) {
  int output_index = 0;
  const char* current = format;
  // One extra character must be left for the null terminator.
  const int usable_length = len - 1;
  while (*current != '\0' && output_index < usable_length) {
    if (*current == '%') {
      current++;
      switch (*current) {
        case 'd':
          // Cut off log message if format could exceed log buffer length.
          if (usable_length - output_index < kMaxIntCharsNeeded) {
            output[output_index++] = '\0';
            return output_index;
          }
          output_index +=
              FormatInt32(&output[output_index], va_arg(args, int32_t));
          current++;
          break;
        case 'u':
          if (usable_length - output_index < kMaxIntCharsNeeded) {
            output[output_index++] = '\0';
            return output_index;
          }
          output_index +=
              FormatUInt32(&output[output_index], va_arg(args, uint32_t));
          current++;
          break;
        case 'x':
          if (usable_length - output_index < kMaxHexCharsNeeded) {
            output[output_index++] = '\0';
            return output_index;
          }
          output[output_index++] = '0';
          output[output_index++] = 'x';
          output_index +=
              FormatHex(&output[output_index], va_arg(args, uint32_t));
          current++;
          break;
        case 'f':
          if (usable_length - output_index < kMaxFloatCharsNeeded) {
            output[output_index++] = '\0';
            return output_index;
          }
          output_index +=
              FormatFloat(&output[output_index], va_arg(args, double));
          current++;
          break;
        case '%':
          output[output_index++] = *current++;
          break;
        case 'c':
          if (usable_length - output_index < 1) {
            output[output_index++] = '\0';
            return output_index;
          }
          output[output_index++] = va_arg(args, int32_t);
          current++;
          break;
        case 's':
          char* string = va_arg(args, char*);
          int string_idx = 0;
          while (string_idx + output_index < usable_length &&
                 string[string_idx] != '\0') {
            output[output_index++] = string[string_idx++];
          }
          current++;
      }
    } else {
      output[output_index++] = *current++;
    }
  }
  output[output_index++] = '\0';
  return output_index;
}
// IzK: Function adapted to use DebugVsnprintf()
extern "C" void DebugLog(const char* format, va_list args) { 
  static constexpr int kMaxLogLen = 256;
  char log_buffer[kMaxLogLen];
  DebugVsnprintf(log_buffer, kMaxLogLen, format, args);
  DEBUG_SERIAL_OBJECT.print(log_buffer);
  //DEBUG_SERIAL_OBJECT.print("\r\n");
}

#else

extern "C" void DebugLog(const char* format, va_list args) { DEBUG_SERIAL_OBJECT.print(format); }

#endif // !defined(TF_LITE_STRIP_ERROR_STRINGS)

namespace tflite {

constexpr ulong kSerialMaxInitWait = 4000;  // milliseconds

void InitializeTarget(const int baud) {
  DEBUG_SERIAL_OBJECT.begin(baud);
  ulong start_time = millis();
  while (!DEBUG_SERIAL_OBJECT) {
    // allow for Arduino IDE Serial Monitor synchronization
    if (millis() - start_time > kSerialMaxInitWait) {
      break;
    }
  }
}

}  // namespace tflite

namespace test_over_serial {

// Change baud rate on default serial port
void SerialChangeBaudRate(const int baud) {
  DEBUG_SERIAL_OBJECT.begin(baud);
  ulong start_time = millis();
  while (!DEBUG_SERIAL_OBJECT) {
    // allow for Arduino IDE Serial Monitor synchronization
    if (millis() - start_time > tflite::kSerialMaxInitWait) {
      break;
    }
  }
}
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
