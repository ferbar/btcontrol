#include "ESP32PWM.h"
#include "utils.h"
#include "config.h"
#include "lokdef.h"
#include "utils_esp32.h"

static const char *TAG="ESP32PWM";

// Pins for bridge1 and bridge2
#ifdef CHINA_MONSTER_SHIELD
// ************* pins für china arduino monster shield !!!! nicht für sparkfun !!!!!
#define MOTOR_OUTPUTS 2
int inApin[MOTOR_OUTPUTS] = {14, 17}; // INA: Clockwise Direction Motor0 and Motor1 (Check:"1.2 Hardware Monster Motor Shield").
int inBpin[MOTOR_OUTPUTS] = {12, 13}; // INB: Counterlockwise Direction Motor0 and Motor1 (Check: "1.2 Hardware Monster Motor Shield").
int pwmpin[MOTOR_OUTPUTS] = {16, 27}; // PWM's input
int cspin[MOTOR_OUTPUTS] = {35, 34};  // Current's sensor input

int enpin[MOTOR_OUTPUTS] = {2, 4}; // open drain output vom VNH2SP30, 2 hat einen 1k PullDown R

// Motor 0 geht beim monster shield nicht: EN1 wird über einen R am ESP32 board auf 0 gezogen
#define MOTOR_NR 1
#define HAVE_CS
#define HAVE_PWM_PIN
#define MOTOR_FREQUENCY 16000
#define MOTOR_IN_BRAKE LOW

#elif defined KEYES_SHIELD
// ************* KEYES VNH5019 shield:
#define MOTOR_OUTPUTS 2
int inApin[MOTOR_OUTPUTS] = {26, 14}; // INA: Clockwise Direction Motor0 and Motor1 (Check:"1.2 Hardware Monster Motor Shield").
int inBpin[MOTOR_OUTPUTS] = {17, 12}; // INB: Counterlockwise Direction Motor0 and Motor1 (Check: "1.2 Hardware Monster Motor Shield").
int pwmpin[MOTOR_OUTPUTS] = {13, 05}; // PWM's input
int cspin[MOTOR_OUTPUTS] = {02, 04};  // Current's sensor input

int enpin[MOTOR_OUTPUTS] = {27, 19}; // open drain output vom VNH2SP30, 2 hat einen 1k PullDown R

#define MOTOR_NR 1
#define HAVE_CS
#define HAVE_PWM_PIN
#define MOTOR_IN_BRAKE LOW

#define MOTOR_FREQUENCY 16000

#elif defined DRV8871
// ************* drv8871 kein extra pwm pin. pwm geht über INA + INB, je nach richtung
#define MOTOR_OUTPUTS 1
int inApin[MOTOR_OUTPUTS] = {13}; // INA + PWM
int inBpin[MOTOR_OUTPUTS] = {12}; // INB + PWM

#define MOTOR_FREQUENCY 16000
#define MOTOR_NR 0
#define MOTOR_IN_BRAKE HIGH

#else
#error No motor board defined!
#endif
// ************* motor shield defs done





#ifdef HAVE_SOUND
void ESP32_MAS_Speed::openFile(uint8_t channel, File &f) {
    if(channel == 2) {
      return ESP32_MAS::openFile(channel, f);
    } else if(channel == 1) { // bis jetzt nur horn
      if(this->Channel[channel]==2) {
        printf("play %s\n", this->Audio_File[1].c_str());
        f = SPIFFS.open(this->Audio_File[1], "r");
        if(!f) {
          printf("open failed\n");
        }
        this->Channel[channel]=4;
      } else {
        printf("disable F1\n");
        f.close();
        this->stopChan(channel);
        lokdef[0].func[1].ison=false;
      }
      return;
    }
    printf("openFile [%d] fahrstufe=%d\n", channel, this->curr_fahrstufe);
    String filename;
    if(this->curr_fahrstufe == -1 && this->target_fahrstufe == -1) {
      f.close();
      this->stopChan(channel);
      this->stopDAC();
      printf("DAC stopped\n");
    } else if(this->curr_fahrstufe == this->target_fahrstufe) {
      filename=String("/F") + this->curr_fahrstufe + ".raw";
    } else {
      if(this->curr_fahrstufe > this->target_fahrstufe) {
        filename=String("/F")+this->curr_fahrstufe+"-F"+(this->curr_fahrstufe-1)+".raw";
        this->curr_fahrstufe--;
      } else {
        filename=String("/F")+this->curr_fahrstufe+"-F"+(this->curr_fahrstufe+1)+".raw";        
        this->curr_fahrstufe++;
      }
    }
    if(filename != f.name()) {
      if(filename) {
        printf("f.name=%s\n", f.name() ? f.name() : "null");
        printf("open file [%s]\n", filename.c_str());
        f = SPIFFS.open(filename, "r");
        if(!f) {
          printf("open failed\n");
        }
      } else {
        // stille
      }
    } else {
      printf("seek 0 file=%s, fahrstufe=%d\n", filename.c_str(), this->curr_fahrstufe);
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
      Serial.println("SPIFFS Mount Failed");
    } else {
      Audio.setDAC(true); // use internal DAC
    }
    this->initialized=true;
  }
  if(!this->isRunning()) {
    Audio.startDAC();
    Serial.println("DAC init done");
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
  printf(":startPlayFuncSound() func1 = %d\n", lokdef[0].func[1].ison);
  if(lokdef[0].nFunc > 1 && lokdef[0].func[1].ison && this->Channel[1] == 0 ) {
    printf("play!\n");
    Audio.playFile(1,"/horn.raw");
  }
}


void ESP32_MAS_Speed::setVolume(uint8_t volume) {
  ESP32_MAS::setVolume(volume);
  writeEEPROM(EEPROM_SOUND_VOLUME, volume, EEPROM_SOUND_SET, 1);
}

ESP32_MAS_Speed Audio;
#endif

/**
 * wird mit new im btcontrol::setup() gestartet
 */
ESP32PWM::ESP32PWM() : USBPlatine(false), dir(0), pwm(0), motorStart(40), motorFullSpeed(255),ledToggle(0) {

	for (int i=0; i<MOTOR_OUTPUTS; i++) {
		pinMode(inApin[i], OUTPUT);
		pinMode(inBpin[i], OUTPUT);
#ifdef HAVE_PWM_PIN   
		pinMode(pwmpin[i], OUTPUT);
    pinMode(enpin[i], INPUT);
#endif
	}
	for (int i=0; i<MOTOR_OUTPUTS; i++) {
		digitalWrite(inApin[i], MOTOR_IN_BRAKE);
		digitalWrite(inBpin[i], MOTOR_IN_BRAKE);

#ifdef HAVE_PWM_PIN
		ledcSetup(i, MOTOR_FREQUENCY, 8);
		ledcAttachPin(pwmpin[i], i);
#elif MOTOR_OUTPUTS == 1
    // dev8871 verwendet ina + inb als pwm ports
    ledcSetup(0, MOTOR_FREQUENCY, 8);
    ledcAttachPin(inApin[0], 0);
    ledcSetup(1, MOTOR_FREQUENCY, 8);
    ledcAttachPin(inBpin[0], 1);
#else
    #error something went wrong
#endif
	}
	for(int i=0; i<MOTOR_OUTPUTS; i++) {
		DEBUGF("Motor %d", i);
		DEBUGF("inA[%d]=%d", i, digitalRead(inApin[i]));
		DEBUGF("inB[%d]=%d", i, digitalRead(inBpin[i]));
#ifdef HAVE_PWM_PIN
		DEBUGF("en[%d]=%d", i, digitalRead(enpin[i]));
#endif
	}
#ifdef INFO_LED_PIN
  pinMode(INFO_LED_PIN, OUTPUT);
#endif
#ifdef HEADLIGHT_PIN
  ledcSetup(2, 5000, 8);
  ledcAttachPin(HEADLIGHT_PIN, 2);
#endif
}

ESP32PWM::~ESP32PWM() {
	fullstop(true, true);
}

void ESP32PWM::setPWM(int f_speed) {
  unsigned char pwm = 0;
  if(f_speed > 0) {
    pwm = f_speed*((double)this->motorFullSpeed - this->motorStart)/255 + this->motorStart;
  }

#if MOTOR_IN_BRAKE==HIGH
  pwm=255-pwm;
#endif

  DEBUGF("ESP32PWM::setPWM set pwm=%d", pwm);
#ifdef HAVE_PWM_PIN
  // analogWrite(pwmpin[motor], f_speed);
	ledcWrite(MOTOR_NR, pwm);
#else
  if(this->dir) {
    ledcWrite(0, pwm);
  } else {
    ledcWrite(1, pwm);
  }
#endif
#ifdef HAVE_SOUND
  Audio.setFahrstufe(ceil(f_speed/(255./5.)));
#endif
#ifdef INFO_LED_PIN
  if(this->ledToggle)
    digitalWrite(INFO_LED_PIN, this->ledToggle & 0x1);
  this->ledToggle++;
#endif
}

void ESP32PWM::setDir(unsigned char dir) {
  this->dir=dir;
  this->fullstop(true, true);
#ifdef HAVE_PWM_PIN
	motorGo(MOTOR_NR, dir ? CW : CCW);
#endif
}

void ESP32PWM::fullstop(bool stopAll, bool emergencyStop) {
#ifdef HAVE_PWM_PIN
	ledcWrite(MOTOR_NR, 0);
#else
  ledcWrite(0,255);
  ledcWrite(1,255);
#endif
}

void ESP32PWM::setFunction(int nFunc, bool *func) {
#ifdef HAVE_SOUND
    Audio.startPlayFuncSound();
#endif
  if(nFunc > 0 ) {
#ifdef HEADLIGHT_PIN
    if(this->headlight != func[0]) {
      printf("set LEDC headlight %d\n", func[0]);
      ledcWrite(2, func[0] ? 20 : 0);
      this->headlight = func[0];
    }
#endif
  }
}

int ESP32PWM::sendPOM(int addr, int cv, int value) {

        #ifdef HAVE_SOUND
          if(cv==266) {
              Audio.setVolume(value);
            return 1;
          } else {
        #endif
            return 0;
        #ifdef HAVE_SOUND
          }
        #endif
}

/**
 * Function that controls the variables:
 * @param motor(0 ou 1)
 * @param direction (BRAKEVCC CW CCW BRAKEGND)
 * @param pwm (0 255);
 */
void motorGo(uint8_t motor, uint8_t direct)
{
if (motor <= MOTOR_OUTPUTS)
	DEBUGF("set motor[%d] dir %d", motor, direct);
    {
    if (direct <=4)
        {
        if (direct <=1)
            digitalWrite(inApin[motor], HIGH);
        else
            digitalWrite(inApin[motor], LOW);

        if ((direct==0)||(direct==2))
            digitalWrite(inBpin[motor], HIGH);
        else
            digitalWrite(inBpin[motor], LOW);

        }
    }
}

