# Modified TensorFlow Lite Micro Peripherals

This directory provides drivers with generic interfaces to peripherals used with the TFLM examples.

## Table of contents
<!--ts-->
* [Modified TensorFlow Lite Micro Peripherals](#tensorflow-lite-micro-peripherals)
  * [LEDs](#leds)
  * [Analog microphone for Pico](#analog-microphone-for-pico)
  * [PDM microphone for Pico](#pdm-microphone-for-pico)

<!--te-->


### LEDs

Copy from [TensorFlow Lite Micro Library for Arduino Examples](https://github.com/tensorflow/tflite-micro-arduino-examples).

A generic LED API is provided in the [led.h](/src/peripherals/led.h) header file.

The physical default LED will vary between peripherals and platforms.
The GPIO pin that connects to the default LED is defined in the
[peripherals.h](/src/peripherals/peripherals.h) header file.

The generic LED API provides the following functionality:

* Turning the LED on/off
* Blinking the LED at an application defined duty cycle and rate

### Analog microphone for Pico

This implementation is based on the [Microphone Library for Pico](https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico), see [README_pico_microphone](/src/peripherals/README_pico_microphone.md). It requires the [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) to be installed.

The original `analog_microphone` from the [Microphone Library for Pico](https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/) has been modified and adapted to work with Arduino IDE, as follows:
* Flatten implementation to have all required files in the [peripherals](/src/peripherals/) directory
* Convert *.c to *.cpp, including the use of new/delete instead of malloc/free
* Move code within `namespace peripherals {}` and `namespace {}` to match how the overall [peripherals](/src/peripherals/) implementation approach
* Ensure the `analog_microphone_init()` and `analog_microphone_start()` functions have a `return 0` at the end (were missing from the original C implementation)

This peripheral implementation is used by the [analog_audio_provider](/examples/micro_speech_pico/analog_audio_provider.cpp), based on the [pico-wake-word](https://github.com/henriwoodcock/pico-wake-word/) implementation. Remember to enable `#define ANALOG_MIC` in 
[audio_provider.h](/examples/micro_speech_pico/audio_provider.h).

**NOTE1**: The [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) has support for [ADCInput API](https://arduino-pico.readthedocs.io/en/latest/adc.html), based on DMA buffering, with similar functionality as provided in `analog_microphone` from the [Microphone Library for Pico](https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/). This is not used in the current code, yet.

### PDM microphone for Pico

This implementation is based on the [Microphone Library for Pico](https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico), see [README_pico_microphone](/src/peripherals/README_pico_microphone.md). It requires the [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) to be installed.

The orginal `pdm_microphone` from the [Microphone Library for Pico](https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/) has been modified and adapted to work with Arduino IDE, as follows:
* Flatten implementation to have all required files in the [peripherals](/src/peripherals/) directory, including the implementation of the original `OpenDM2PCM` directory.
* Convert *.c to *.cpp, including the use of new/delete instead of malloc/free
* Move code within `namespace peripherals {}` and `namespace {}` to match how the overall [peripherals](/src/peripherals/) implementation approach
* Ensure the `pdm_microphone_init()` and `pdm_microphone_start()` functions have a `return 0` at the end (were missing from the original C implementation)
* Modify the `OpenPDMFiter` implementation:
  * Always have `uint8_t Gain;` defined in the `TPDMFilter_InitStruct` structure
  * Define the second argument of the `Open_PDM_Filter_64()` and `Open_PDM_Filter_128()` functions to be `int16_t* data_out` (instead of uinit16_t*) to match the actual implementation/use in `pdm_microphone.cpp`

**NOTE2**: The [`pdm_microphone.pio`](/src/peripherals/pdm_microphone.pio) is included in the [`pdm_microphone.cpp`](/src/peripherals/pdm_microphone.cpp) in its compiled form [`pdm_microphone.pio.h`](/src/peripherals/pdm_microphone.pio.h). The Arduino toolchain cannot perform the compilation automatically. The `pdm_microphone.pio` must be _manually_ compiled with PIOASM tool to `pdm_microphone.pio.h`. The PIOASM tool can be found pre-compiled in the Arduino IDE installation of the [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) in `.../packages/rp2040/tools/pqt-pioasm`.


This peripheral implementation is used by the [pdm_audio_provider](/examples/micro_speech_pico/pdm_audio_provider.cpp), based on the [pico-wake-word](https://github.com/henriwoodcock/pico-wake-word/) implementation. Remember to enable `#define PDM_MIC` in
[audio_provider.h](/examples/micro_speech_pico/audio_provider.h).

**NOTE3**: Alternatively, the [PDM Arduino library](https://docs.arduino.cc/learn/built-in-libraries/pdm/) can also be used in [pdm_audio_provider](/examples/micro_speech_pico/pdm_audio_provider.cpp) for any board which has an on-board PDM microphone.

**NOTE4**: The [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) has support for [I2S API](https://arduino-pico.readthedocs.io/en/latest/i2s.html), based on DMA buffering, with similar functionality as provided in `pdm_microphone` from the [Microphone Library for Pico](https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/). This is not used in the current code, yet.
