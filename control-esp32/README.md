# ESP32 Control Pad

<img src="img_control_pad.jpg" alt="Control Pad" width="300"/>

Wlan / Bluetooth Steuerung für die Eisenbahn mit haptischem Feedback. Wlan / Lok Auswahl übers Display, Geschwindigkeit und Funktionen über das Schiebepotentiometer bzw die Taster.

202203: die ESP32 Bluetooth lib braucht leider extrem viel RAM/Flash, funktioniert auch nicht mit Blueooth 2.0 USB Dongles, braucht mein gepatchtes ESP32-Arduino-SDK

## Partlist

* Lilygo ESP32 T-Display
* Schiebepotentiometer 60mm - 70mm Gehäuse, 45mm Schiebeweg, linear (B10K -> SC4521N)
* Stiftleiste (eventuell beim ESP32 dabei)
* Ein paar Taster (3 für mein .svg)
* Ein paar Schalter (3 für mein .svg)
* Lochrasterplatine oder gefräste Plexiglas Front
* Akku mit passendem Stecker für das T-Display (JST-PH 1,25mm), oder 14500 AA Lipo Akku + Bat halter
* Visual Studio Code + USB C Kabel

## Plexiglas Frontplatte

* Plexiglas 100mm x 90mm
* CNC Fräse ;-)
* 1mm Fräser + 1,5mm Fräser
* 3 Taster
* 3 Schalter
* Power Schiebeschalter
* bCNC
* 2 abgezwickte Schrauben für die TTGO - Display - Taster - 2,8mm Durchmesser, 6mm lang ohne Gewinde, Ende abgeschliffen
* 3 Messing Nagerl zum Befestigen vom TTGO - Display

howto bCNC: TODO **************************

## 3D Print Boden
<img src="img_control_pad_3d_boden.jpg" alt="Control Pad - Boden" width="300"/>

* 4* 1,6x8 versenk Schrauben

howto: TODO **************************

## Kompilieren

getestet mit platformio/framework-arduinoespressif32@^3.20007.0

### esp32 Arduino SDK kompilieren

Optional: das IDF wird im Arduino SDK mit komplett aktivierten BT + BLE + A2DP + HFP kompiliert was extrem viel RAM + Flash benötigt. Images können sonst über eine BT Verbindung nicht angezeigt werden.

Arduino sdk ohne A2DP und BLE selber kompilieren -> spart 300kB flash und 50kB ram:

* git clone esp32-idf -> IDF 4.4.4 branch
* git clone https://github.com/espressif/esp32-arduino-lib-builder.git -> IDF 4.4 branch + setup + git submodule init
* git clone arduino-esp32 -> irgendeinen release branch welcher IDF 4.4.4 verwendet
* unter components/arduino softlink auf arduino-esp32 anlegen
* die ./arduino-lib-builder/defaultconf.esp32 nach esp32-arduino-lib-builder/configs/ verlinken
* Build reduzieren: in configs/builds.json unter "target": "esp32" nur "bootloaders":[ ["dio","40m"] ] über lassen, "mem_variants":[ ["dio","80m"] ]
* die ./arduino-lib-builder/build-chris.sh nach esp32-arduino-lib-builder/ verlinken
* build-chris.sh im esp32-arduino-lib-builder/ aufrufen, kopiert die object files nach .platformio/packages/framework-arduinoespressif32
* tools/sdk/esp32/sdkconfig checken ob BLE abgedreht ist (CONFIG_BTDM_CTRL_MODE_BR_EDR_ONLY=y)


### ESP32 Create Memory map file
-https://everythingesp.com/esp32-arduino-creating-a-memory-map-file/-

mit platformio gibts unter .pio/build/wemos_d1_mini32/firmware.map immer ein .map file

Analyse z.b. mit fpvgcc --sar .pio/build/wemos_d1_mini32/firmware.map

Zeigt Grösse von Funktionen und statischen Variablen an:

memstat.sh

### Call-graphs: (von wo wird welche funktion aufgerufen)

Anleitung für radare2: https://reverseengineering.stackexchange.com/questions/16081/how-to-generate-the-call-graph-of-a-binary-file

```
afl | grep BTAddress
s 0x400eb128
agf
```

output.dot mit less anschaun und nach funktionsname suchen, dann nach `-> "func-adresse"` dann sieht man wo die funktion aufgerufen wurde.

xdot.py ist nur bedingt hilfreich

## Hardware:

5V Stromanschulss: keine Strom-Rund Buchse montieren, GND -> GND, +5V -> 1Ohm R -> Kondensator am TTGO-T-Display Board (siehe Bild)

## Workarounds

### ttgo-display-esp32 startet nach power on nicht
=> Kondensator entfernen (bei neueren Versionen vom Board nicht mehr notwendig, dort fehlt der C schon)

<img src="ttgo-t-display-gpio0-capacitor.jpg" alt="gpio0 capacitor" width="300"/>
