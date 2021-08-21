# Setup Putzlok

Kurze Docu zum Umbau einer LGB Putzlok in eine Akku Lok mit einem ESP32

## Partlist

* Die LGB Putzlok ;-)
* ESPDuino -> ESP32 Board in Arduino - Form - auf Aliexpress nach "ESP32 R32" suchen.
* VNH5019 Motor Shield
* 2* Ikea Braunit Akku Packs
* Pins für die Akkus (aliexpress: https://de.aliexpress.com/item/4000332107587.html?spm=a2g0s.9042311.0.0.27424c4dFLLeEG 10 Pairs Pitch 2.5 mm 4 Positions Female Blade Receptacle 5.8 mm Male Header Battery Connector Right Angle Through Holes 7A /pin)
* Polymorph Plastik kugerln
* Kleinzeug für die Beleuchtung
  * Lochrasterplatine
  * FETs
  * Widerstände
  * LEDs

## Zusammenbau

### Akkus

Die 2 Akkus müssen in Serie geschalten werden damit wir 14V - 16V bekommen. Kabel anlöten, auf die Polung achten, und dann kommt die Patzerei:
Stecker in die Akkus, obern Teil, dort wo der Stecker von den Akkus ist mit Klebeband abkleben (eventuell mit Vaseline noch einfetten???), das Polymorph Plastik mit kochendem Wasser erhitzen und drauf patzen damit der Stecker nicht mehr rausfallen kann.
10 Minuten warten, die Akkus werden etwas mühsamer rausgehen da das Polymorph Plastik etwas geschrumpft ist.

### ESP32 + Motor Shield

Zusammenstecken ist soweit eh klar. Wichtig: vom Motor Shield den +5V pin *NICHT* einstecken!!! Es funktioniert sonst das WLAN so gut wie nicht !!!

Optional: grössere WLAN Antenne anlöten.

