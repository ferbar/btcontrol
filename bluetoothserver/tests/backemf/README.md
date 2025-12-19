# BEMF (BACK EMF)

Schaltet man die Stromzufuhr zum Motor ab, dreht er sich natürlich noch weiter. Nach 200µs produziert der Motor im Generatorbetrieb eine Gleichspannung (NICHT Strom) die den U/min entspricht. Die Polung ist die selbe wie zuvor. Wird der Motor blockiert ist der gelesene Wert negativ (vermutlich wegen Spulen im Motor)

Testprogramme für die Platinenversion 2.1 und den ADS1015

I2C auf 400kHz stellen, sonst dauert die messung lange

Eventuell gibts ein Problem mit blockiertem Motor + TLE9201: scheint wie wenn der bei blockiertem Motor + DIS=1 den Motor kurz invertiert einschaltet.
