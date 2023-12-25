#ifdef HAVE_SOUND
// 20231026 ???? #include <SD.h>
#include "SPIFFS.h"
#include "ESP32_MAS.h"

// kann nicht geändert werden!
#define SOUND_ON_OFF_FUNC 3

class ESP32_MAS_Speed : public ESP32_MAS {
  virtual void openFile(uint8_t channel, File &f);
public:
  ESP32_MAS_Speed();

// -1 => stille
// 0 => stand
  void setFahrstufe(int fahrstufe) {
    assert(fahrstufe >= -1 && fahrstufe <= 5);
    this->target_fahrstufe=fahrstufe;
  }
  
  int curr_fahrstufe=-1;
  int target_fahrstufe=-1;
  void begin();
  void stop();
  bool initialized=false;
  void startPlayFuncSound();
  int setVolume(int volume);
  int setVolumeMotor(int volume);
  int setVolumeHorn(int volume);
};
extern ESP32_MAS_Speed Audio;
#endif
