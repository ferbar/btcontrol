#ifndef UTILS_ESP32_H
#define UTILS_ESP32_H
#define EEPROM_SOUND_VOLUME 0
#define EEPROM_SOUND_SET 1
#define EEPROM_WIFI_AP_CLIENT 2
#define EEPROM_SOUND_VOLUME_MOTOR 3
#define EEPROM_SOUND_VOLUME_HORN 4
// anzahl eeprom Werte
#define EEPROM_VALUES 5

uint8_t readEEPROM(int addr);
void writeEEPROM(int addr1, uint8_t data1, int addr2 = -1, uint8_t data2 = 0);

// gibts am ESP32 nicht (update: im IDF 4.4 aufgetaucht)
#ifndef memmem
extern "C" {
void *memmem(const void *haystack, size_t haystacklen,
                    const void *needle, size_t needlelen);
}
#endif

void printDirectory();

void initOTA(void (*onStartCallback)());

extern const char* device_name;

#endif
