#ifdef HAVE_SOUND
#include <SD.h>
#include "SPIFFS.h"
#include "ESP32_MAS.h"

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
  void setVolume(uint8_t volume);
};
extern ESP32_MAS_Speed Audio;
#endif
