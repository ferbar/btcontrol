#include "config.h"
#include "utils.h"
#include "lokdef.h"
#include "utils_esp32.h"

#ifdef HAVE_SOUND

#include "ESP32_MAS_Speed.h"
static const char *TAG="ESP32PWM";
ESP32_MAS_Speed::ESP32_MAS_Speed() : ESP32_MAS() {
  
}

void ESP32_MAS_Speed::openFile(uint8_t channel, File &f) {
    if(channel == 2) {
      return ESP32_MAS::openFile(channel, f);
    } else if(channel == 1) { // bis jetzt nur horn
      if(this->Channel[channel]==2) {
        DEBUGF("play %s", this->Audio_File[1].c_str());
        f = SPIFFS.open(this->Audio_File[1], "r");
        if(!f) {
          DEBUGF("open failed");
        }
        this->Channel[channel]=4;
      } else {
        DEBUGF("disable F1");
        f.close();
        this->stopChan(channel);
        lokdef[0].func[1].ison=false;
      }
      return;
    }
    DEBUGF("openFile [%d] fahrstufe=%d", channel, this->curr_fahrstufe);
    String filename;
    if(this->curr_fahrstufe == -1 && this->target_fahrstufe == -1) {
      f.close();
      this->stopChan(channel);
      this->stopDAC();
      DEBUGF("DAC stopped");
    } else if(this->curr_fahrstufe == this->target_fahrstufe) {
      filename=String("/sounds/F") + this->curr_fahrstufe + ".raw";
    } else {
      if(this->curr_fahrstufe > this->target_fahrstufe) {
        filename=String("/sounds/F")+this->curr_fahrstufe+"-F"+(this->curr_fahrstufe-1)+".raw";
        this->curr_fahrstufe--;
      } else {
        filename=String("/sounds/F")+this->curr_fahrstufe+"-F"+(this->curr_fahrstufe+1)+".raw";
        this->curr_fahrstufe++;
      }
    }
    if(filename != f.name()) {
      if(filename) {
        DEBUGF("f.name=%s", f.name() ? f.name() : "null");
        DEBUGF("open file [%s]", filename.c_str());
        f = SPIFFS.open(filename, "r");
        if(!f) {
          printf("open failed\n");
        }
      } else {
        // stille
      }
    } else {
      DEBUGF("seek 0 file=%s, fahrstufe=%d", filename.c_str(), this->curr_fahrstufe);
      f.seek(0); // seek0 is nicht viel schneller als neues open !!!!
    }
    switch (this->Channel[channel]) {
      case 2:
        this->Channel[channel] = 5;
        break;
      case 3:
        this->Channel[channel] = 4;
        break;
      case 4:
        this->Channel[channel] = 4;
        break;
    }
  };

void ESP32_MAS_Speed::begin() {
  if(!this->initialized) {
    SPIFFS.begin();
    delay(500);
    if (!SPIFFS.begin()) {
      DEBUGF("SPIFFS Mount Failed");
    } else {
      Audio.setDAC(true); // use internal DAC
    }
    this->initialized=true;
  }
  if(!this->isRunning()) {
    Audio.startDAC();
    DEBUGF("DAC init done");
    Audio.setFahrstufe(0);
    Audio.setGain(0,20);
    Audio.loopFile(0,""); // openFile handles the filenames
    if(readEEPROM(EEPROM_SOUND_SET))
      Audio.setVolume(readEEPROM(EEPROM_SOUND_VOLUME)); // 255=max
  }
}

void ESP32_MAS_Speed::stop() {
  Audio.setFahrstufe(-1);
}

void ESP32_MAS_Speed::startPlayFuncSound() {
  // printf("ESP32_MAS_Speed::startPlayFuncSound() %d %d %d \n",lokdef[0].nFunc, lokdef[0].func[1].ison , this->Channel[1] );
  DEBUGF(":startPlayFuncSound() func1 = %d", lokdef[0].func[1].ison);
  if(lokdef[0].nFunc > 1 && lokdef[0].func[1].ison && this->Channel[1] == 0 ) {
    DEBUGF("play!");
    Audio.playFile(1,"/sounds/horn.raw");
  }
}


void ESP32_MAS_Speed::setVolume(uint8_t volume) {
  ESP32_MAS::setVolume(volume);
  writeEEPROM(EEPROM_SOUND_VOLUME, volume, EEPROM_SOUND_SET, 1);
}

ESP32_MAS_Speed Audio;
#endif
