partlist:
esp32 cam
drv8871 H brücke board
pam 8302A board (+diode + 100µF tantal)
stepdown 5V
streifen leiterplatine - abmessungen siehe sketch
Licht hinten + Führerhaus: 2* FET + 4* 100Ohm + 2* LEDs
Licht vorne LED + 100Ohm


* esp32 cam
   1. flashen
   2. plastik von den pinleisten runterziehen und pins einzeln rauslöten -> power + rx + tx + gpio0 + gnd daneben eventuell noch drauf lassen

   3. flash led runterlöten / rauskratzen
   4. antennen R umlöten

* platine schneiden + bohrungen machen + leiterbahnen unterbrechen

* schaltregler auf der unterseite drauflöten
   checken ob die 5V dort sind wo sie hingehören

* pam auf der unterseite drauflöten

* bridge: drahtln in die base platine löten (nicht das h brücke board)

* esp32: nur + und gnd anlöten, verbindung zur bridge, motor testen, verbindung zum pam

* Lautsprecher im Führerstand: kleiner Schiebeschalter, dort kann auch gleich eine polyswitch fuse montiert werden

LEDs:
GPIO 0  LED Führerstand
GPIO 4  LED vorn
GPIO 16 LED hinten
GPIO 33 esp32 cam onboard led

ADC:
GPIO 35

