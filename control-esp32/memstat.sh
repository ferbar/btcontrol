#!/bin/bash

~/.platformio/packages/toolchain-xtensa32/bin/xtensa-esp32-elf-nm -SC --size-sort ./.pio/build/wemos_d1_mini32/firmware.elf
