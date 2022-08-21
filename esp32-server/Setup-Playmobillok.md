# Setup Playmobillok

## Partlist
* Eine Playmobillok (z.b. die Rangierlok 4027 / 4050)
* ESP32 D1 Mini
* ESP8266 power shield, 100µF Tantal / möglichst grosser Keramikkondensator noch bei 3,3V dazu
* ESP8266 motor shield wenn das WEMOS Motor shield verwendet wird muss folgender PR mit verwendet werden: https://github.com/wemos/WEMOS_Motor_Shield_Arduino_Library/pull/5/files#diff-d3c42de538f1ac16f0098750d07f327a203c4c1581bea97083639e64551639e2
* Lautsprecher
* PAM8302 Board (mono, vorzugsweise mit Poti) oder PAM8403 Board (die ham 2 Kanäle, wir bräuchten nur einen, Spannungsteiler 1-2k:10k löten, 2. Kanal auf Masse) zusätzlich noch einen 100µF Tantal C
* LiPo Akkus mit Schutzschaltung, 803450 *3 geht sich im Anbau hinten gut aus, in einen Schrumpfschlauch verpacken
* Ein/Ausschalter für den Rauchfang
* Kabel mit verpolungssicherem Stecker
* 2 Helle warmweisse LED + R (GPIO 0 + 4)
* für die Lampe am Führerstand einen duchsichtigen Deckel von einer Sprayflasche
* Spannungsteiler + C zum Bat Spannung messen (47k Ohm / 10k Ohm)


<img src="img_playmobil_inside.jpg" alt="Playmobillok Innenleben" width="300"/>

