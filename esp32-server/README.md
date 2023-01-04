# Akku Lok Steuerung / Elektronik mit ESP32

## LGB Putzlok

LGB Putzlok mit Ikea Akkus und ESP32 (ESPDuino und Dual-VNH5019 Motor Shield) als Elektronik

Beschreibung: [LGB Putzlok](Setup-Putzlok.md)

<img src="img_putz_done2.jpg" alt="LGB Akku Putzlok ESP32" width="300"/>

## Playmobil Lok
[Playmobil Lok](Setup-Playmobillok.md)

<img src="img_playmobil_inside.jpg" alt="Playmobil Lok ESP32" width="300"/>

## Stainz
[Stainz](Setup-Stainz.md)

<img src="img_stainz_platine.jpg" alt="Playmobil Lok ESP32" width="300"/>



## CVs:
| CV | Beschreibung |
|---|---|
| 266 | Gesamtlautstärke (0..255) => besser das auf 255 und Spannungsteiler beim Verstärker. Vorsicht: wenn Motor + Horn + gesamt auf 255 sind wird der Sound übersteuern / knacksen !!! |
| 267 | Lautstärke Motor (0..255) |
| 268 | Lautstärke Horn (0..255) |
| 500 | BAT Value |
| 501 | Raw BAT Value |
| 510 | wifi AP=0 / client=1 |
