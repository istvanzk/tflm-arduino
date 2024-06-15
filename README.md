# TFLM implementation for Arduino

![Exp](https://img.shields.io/badge/Fork&Copy-experimental-orange.svg)
[![Lic](https://img.shields.io/badge/License-Apache2.0-green)](http://www.apache.org/licenses/LICENSE-2.0)
![Py](https://img.shields.io/badge/Python-3.9+-green)
[![TensorFlow 2.15](https://img.shields.io/badge/TensorFlow-2.15-FF6F00?logo=tensorflow)](https://github.com/tensorflow/tensorflow/releases/tag/v2.15.0)
![Ver](https://img.shields.io/badge/Version-0.1-lightgrey)
![HWD](https://img.shields.io/badge/HWD_tests-Ongoing-lightgreen)

This experimental fork is based on the old official [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples) with modifications to work with the newer (2024+) official 
[TensorFlow Lite for Microcontrollers](https://github.com/tensorflow/tflite-micro/tree/main) implementation.

To avoid compatibility issues with the old official [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples), this Arduino library should be installed as `TFLM_Arduino` and the ino code uses `#include <TensorFlowLiteMicro.h>`. 
Its version number has been reseted to indicate a new fork of the library.

The included [micro_speech_pico](/examples/micro_speech_pico/) implementation is using the modifications introduced in 
[MICRO_SPEECH implementation for PICO-TFLMICRO](https://github.com/istvanzk/pico-micro_speech/)
and requires the [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) 
by Earle F. Philhower, to be installed and used in Arduino IDE instead of the `Arduino Mbed OS RP2040 Boards`.


## Table of contents
<!--ts-->
* [How to Install](#how-to-install)
  * [GitHub](#github)
  * [Checking your Installation](#checking-your-installation)
* [Updates](#updates)
* [Compatibility](#compatibility)
* [License](#license)
* [Contributing](#contributing)
<!--te-->

## How to Install

### GitHub

To avoid compatibility issues with the old official [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples), this Arduino library should be installed as `TFLM_Arduino` and the ino code uses `#include <TensorFlowLiteMicro.h>`. 

To install this experimental version of the library, requires you to clone the
repo into the folder that holds libraries for the Arduino IDE. The location for
this folder varies by operating system, but typically it's in
`~/Arduino/libraries` on Linux, `~/Documents/Arduino/libraries/` on MacOS, and
`My Documents\Arduino\Libraries` on Windows.

Once you're in that folder in the terminal, you can then grab the code using the
git command line tool:

```
git clone https://github.com/istvanzk/tflm-arduino TFLM_Arduino
```

To update your clone of the repository to the latest code, use the following terminal commands:
```
cd TFLM_Arduino
git pull
```

Please note that the [micro_speech_pico](/examples/micro_speech_pico/) example requires the [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) by Earle F. Philhower, to be installed and used (follow therin provided installation guidelines) instead of the `Arduino Mbed OS RP2040 Boards`.

### Checking your Installation

Once the library has been installed, you should then start the Arduino IDE.
You will now see an `TFLM_Arduino` entry in the `File -> Examples` menu of the Arduino IDE. 
This submenu contains the `hello_world` and `micro_speech_pico` sample projects you can try out.

## Updates

The code here is created through a _manual_ project generation process
and may differ from the source of truth, since it's cross-platform and needs to be modified to work within the Arduino IDE.
For now, the code cannot be synced to official [TensorFlow Lite for Microcontrollers](https://github.com/tensorflow/tflite-micro/tree/main) nor to the old [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples).

The follwing code provided in the old official [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples), **has been removed in this fork**:
 * The `scripts` folder and its content
 * The `docs` folder and its content
 * The `src/tensorflow/lite/experimental/microfrontend/lib` folder and its content (not used by the included [micro_speech_pico](/examples/micro_speech_pico/) example)
 * The `examples/magic_wand`, `examples/person_detection` examples

The follwing code provided in the old official [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples), **has been modified in this fork**:
 * The `src/tensorflow/lite/micro/system_setup.cpp` (see below for details)
 * The `src/peripherals` (see below for details)
 * The [hello_world.ino](/examples/hello_world/hello_world.ino) example is modified to pull in only the operation implementations needed, instead of all available TFLM operators
 * The included [micro_speech_pico](/examples/micro_speech_pico/) example is modified to work specifically with `Raspberry Pi Pico` (RP2040) compatible boards, hence its new name


## Compatibility

This TFLM framework code for running machine learning models should be compatible with most Arm Cortex M-based boards, such as the `Raspberry Pi Pico` or `Nano 33 BLE Sense`. 

However, please note that the original code in `src/peripherals` from [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples), meant to be used to access peripherals like microphones, cameras, and accelerometers, **has been modified in this fork**, as described below.

The modified [src/peripherals](/src/peripherals/) includes a bare minimum implementation, for only three peripherals: 
* LED - which should work with all the Arduino compatible boards
* Analog microphone - which is meant to be used with RP2040 based boards (using [src/peripherals/analog_microphone](src/peripherals/analog_microphone.cpp))
* PDM microphone - which can be used with RP2040 based boards (using [src/peripherals/pdm_microphone](src/peripherals/pdm_microphone.cpp)) or any other Arduino board with on-board PDM microphone (using the [PDM Arduino library](https://docs.arduino.cc/learn/built-in-libraries/pdm/))

See the [/src/peripherals/README](/src/peripherals/README.md) for details.
These adaptations were required to make the library code work with the latest official Arduino and Tensorflow Lite Micro libraries.

The included [micro_speech_pico](/examples/micro_speech_pico/) example code has been modified to work specifically with `Raspberry Pi Pico` (RP2040) compatible boards.

The [peripherals.h](/src/peripherals/peripherals.h) implementation uses:
```
#if defined(ARDUINO_SPARKFUN_MICROMOD_RP2040)
#define RP2040_BOARD 
...
```

and the [system_setup.cpp](/src/tensorflow/lite/micro/system_setup.cpp) implementation uses:
```
#if defined(ARDUINO) && !defined(ARDUINO_SPARKFUN_MICROMOD_RP2040)
#define ARDUINO_EXCLUDE_CODE
#endif  // defined(ARDUINO) && !defined(ARDUINO_SPARKFUN_MICROMOD_RP2040)

#ifndef ARDUINO_EXCLUDE_CODE
...
```

Therefore, if you want to experiment with and adapt the code for other RP2040 based boards, you'll need to edit these two files accordingly by changing the `ARDUINO_SPARKFUN_MICROMOD_RP2040` to another board preprocessing definition name. The board preprocessor definition format in Arduino IDE is: `ARDUINO_<PROCESSOR-DESCRIPTOR>_<BOARDNAME>`. The `<PROCESSOR-DESCRIPTOR>_<BOARDNAME>` strings for the installed boards can be listed by running in the base Arduino directory, the following command:
```
grep board= `find . -name boards.txt` | cut -f2 -d= | sort -u
```

**NOTE**: The [system_setup.cpp](/src/tensorflow/lite/micro/system_setup.cpp) implementation fixes at least two issues encoutered with the older version of the library:
1. The error of missing/wrong definition for `DebugLog()` function, see e.g. [undefined reference to 'DebugLog' in micro_error_reporter.cpp](https://github.com/arduino/ArduinoTensorFlowLiteTutorials/issues/35). The `tensorflow/lite/micro/micro_error_reporter.h` is not used anymore, and is replaced by the use of `tensorflow/lite/micro/micro_log.h`: `MicroPrintf()` uses `DebugLog()` and `DebugLog()` uses `DebugVsnprintf()`, etc.

2. The missing correct `DebugVsnprintf()` function, which allows the use of the `MicroPrintf()` to output printf like formated strings to the serial output/console. 
This code is adapted from the old (up to 2023) version of the `MicroVsnprintf()` in `micro_string.cpp`.

3. Most of the new [system_setup.cpp](/src/tensorflow/lite/micro/system_setup.cpp) implementation is pulled in only if `!defined(TF_LITE_STRIP_ERROR_STRINGS)` and `TF_LITE_STRIP_ERROR_STRINGS` can be undefined e.g. in `micro_speech_pico.ino`. Else, a simple `DebugLog()` function without printf like formating is compiled, to reduce the code size.

## License

This code is made available under the Apache 2 license.

## Contributing

Forks of this library are welcome and encouraged.
