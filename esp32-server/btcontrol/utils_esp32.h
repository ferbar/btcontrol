#ifndef UTILS_ESP32_H
#define UTILS_ESP32_H
#define EEPROM_SOUND_VOLUME 0
#define EEPROM_SOUND_SET 1
// anzahl eeprom Werte
#define EEPROM_VALUES 2

uint8_t readEEPROM(int addr);
void writeEEPROM(int addr1, uint8_t data1, int addr2 = -1, uint8_t data2 = 0);

// gibts am ESP32 nicht
void *memmem(const void *haystack, size_t haystacklen,
                    const void *needle, size_t needlelen);


#endif
