# Micro Speech for Pico Example

This [micro_speech_pico](/examples/micro_speech_pico/) implementation is using the modifications introduced in 
[MICRO_SPEECH implementation for PICO-TFLMICRO](https://github.com/istvanzk/pico-micro_speech/)
and requires the [Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) 
by Earle F. Philhower, to be installed and used in Arduino IDE.

The main change compared to older implementations is that the `tensorflow/lite/experimental/microfrontend` is not used
and instead the new approach, with a separate TFLM model for the audio preprocessing is used. 
See the official [Micro Speech Example](https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/micro_speech) for details.

The default implementation in `command_responder.cpp` writes out the name of the recognized command
to the serial port and turns on/off the default LED for the recognized yes/no commands. 
Real applications will want to take some custom action instead, and should implement their own versions of this function.

