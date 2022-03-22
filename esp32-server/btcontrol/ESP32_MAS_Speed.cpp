#include "config.h"
#include "utils.h"
#include "lokdef.h"
#include "utils_esp32.h"
#include "clientthread.h"

#define TAG "ESP32_MAS_SPEED"

#ifdef HAVE_SOUND

#include "ESP32_MAS_Speed.h"

//#define DEBUG_MAS

#ifdef DEBUG_MAS
#warning !!!!!! DEBUG_MAS enabled - expect i2s buffer underruns thru Serial.printf
#endif

ESP32_MAS_Speed::ESP32_MAS_Speed() : ESP32_MAS() {
  DEBUGF("ESP32_MAS_Speed::ESP32_MAS_Speed()");
}

void ESP32_MAS_Speed::openFile(uint8_t channel, File &f) {
    if(channel == 2 || channel == 1) {
      return ESP32_MAS::openFile(channel, f);
/*
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
*/
    }
#ifdef DEBUG_MAS
    DEBUGF("openFile [%d] fahrstufe=%d", channel, this->curr_fahrstufe);
#endif
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
        DEBUGF("old filename: f.name=%s", f.name() ? f.name() : "null");
        DEBUGF("open file [%s]", filename.c_str());
        if(!SPIFFS.exists(filename)) {  // workaround 202202: open doesn't fail if file doesn't exist!
          ERRORF("file %s doesn't exist", filename.c_str());
          this->stopDAC();
        }
        f = SPIFFS.open(filename, "r");
        if(!f) {
          ERRORF("open %s failed", filename.c_str());
          this->stopDAC();
        }
      } else {
        // Stille
      }
    } else {
#ifdef DEBUG_MAS
      DEBUGF("seek 0 file=%s, fahrstufe=%d", filename.c_str(), this->curr_fahrstufe);
#endif
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
  DEBUGF("ESP32_MAS_Speed::begin()");
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
    Audio.setFahrstufe(0);
    Audio.setGain(0,80);  // Motor sound
    // Audio.setGain(1,100);  // Horn -> default 128
    Audio.loopFile(0,""); // openFile handles the filenames
    int volume=readEEPROM(EEPROM_SOUND_VOLUME);
    NOTICEF("############# Sound Volume: %d (CV:266) ###########################", volume);
    Audio.setVolume(volume); // 255=max
    Audio.startDAC();
    DEBUGF("DAC init done");
  }
}

void ESP32_MAS_Speed::stop() {
  DEBUGF("ESP32_MAS_Speed::stop()");
  Audio.setFahrstufe(-1);
}

void ESP32_MAS_Speed::startPlayFuncSound() {
  // printf("ESP32_MAS_Speed::startPlayFuncSound() %d %d %d \n",lokdef[0].nFunc, lokdef[0].func[1].ison , this->Channel[1] );
  if(TCPClient::numClients == 0) {
    DEBUGF("ESP32_MAS_Speed::startPlayFuncSound() - no clients connected, ignoring");
    return;
  }
    
  DEBUGF(":startPlayFuncSound() func1 = %d", lokdef[0].func[1].ison);
  if(lokdef[0].nFunc > 1 && lokdef[0].func[1].ison && this->Channel[1] == 0 ) {
    DEBUGF("play!");
    Audio.playFile(1,"/sounds/horn.raw", []() {
        DEBUGF("disable F1");
        lokdef[0].func[1].ison=false;
    });
  }

  if(lokdef[0].nFunc > SOUND_ON_OFF_FUNC && lokdef[0].func[SOUND_ON_OFF_FUNC].ison != this->isRunning()) {
    if(lokdef[0].func[SOUND_ON_OFF_FUNC].ison) {
      DEBUGF("ESP32_MAS_Speed::startPlayFuncSound() start");
      this->begin();
    } else {
      DEBUGF("ESP32_MAS_Speed::startPlayFuncSound() stop");
      this->stopDAC();
    }
  }
}


void ESP32_MAS_Speed::setVolume(uint8_t volume) {
  DEBUGF("ESP32_MAS_Speed::setVolume(%d)", volume);
  ESP32_MAS::setVolume(volume);
  writeEEPROM(EEPROM_SOUND_VOLUME, volume, EEPROM_SOUND_SET, 1);
}

ESP32_MAS_Speed Audio;
#endif
