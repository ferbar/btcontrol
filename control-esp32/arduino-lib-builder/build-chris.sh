#!/bin/bash

# !! wichtig: idf, lib builde + sfk müssen von der verion/branch her zusammen passen!

. ../esp-idf/export.sh

# im components verzeichnis muss ein softlink auf das arduino framework git repo sein (wenn nur plattformio dann dort packages softlink machen)
./build.sh -s -c ~/.platformio/packages/framework-arduinoespressif32 -t esp32
