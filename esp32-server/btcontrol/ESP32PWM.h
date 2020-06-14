#include <SD.h>
#include "USBPlatine.h"
#include "config.h"


class ESP32PWM : public USBPlatine {
public:
	ESP32PWM();
	virtual ~ESP32PWM();
	/// @param f_speed 0...255
	virtual void setPWM(int f_speed);
	virtual void setDir(unsigned char dir);
	// brauch ma da ned:
	virtual void commit() {};
	virtual void fullstop(bool stopAll, bool emergencyStop);
  virtual void setFunction(int nFunc, bool *func);
  int sendPOM(int addr, int cv, int value);
  
private:
	void init(int devnr);

	int dir;
	int pwm;
	char buffer[1024];

	int motorStart;
	int motorFullSpeed;
  unsigned int ledToggle;
  bool horn=false;
  bool headlight=false;
};

#define BRAKEVCC 0
#define CW 1
#define CCW 2
#define BRAKEGND 3
void motorGo(uint8_t motor, uint8_t direct);

#ifdef HAVE_SOUND
#include "SPIFFS.h"
#include "ESP32_MAS.h"

class ESP32_MAS_Speed : public ESP32_MAS {
  virtual void openFile(uint8_t channel, File &f);
public:
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
