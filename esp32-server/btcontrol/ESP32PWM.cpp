#include <Arduino.h>
#include "ESP32PWM.h"
#include "utils.h"
#include "config.h"
#include "lokdef.h"
#include "utils_esp32.h"
#include "ESP32_MAS_Speed.h"

static const char *TAG="ESP32PWM";

// Pins for bridge1 and bridge2
#ifdef CHINA_MONSTER_SHIELD
// -============================================================================================================================= VNH2SP30 China Monster Shield
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
// =============================================================================================================================== KEYES VNH5019 shield
#define MOTOR_OUTPUTS 2
int inApin[MOTOR_OUTPUTS] = {26, 14}; // INA: Clockwise Direction Motor0 and Motor1 (Check:"1.2 Hardware Monster Motor Shield").
int inBpin[MOTOR_OUTPUTS] = {17, 12}; // INB: Counterlockwise Direction Motor0 and Motor1 (Check: "1.2 Hardware Monster Motor Shield").
int pwmpin[MOTOR_OUTPUTS] = {13, 05}; // PWM's input
int cspin[MOTOR_OUTPUTS] = {02, 04};  // Current's sensor input !!! ADC2 nicht mit Wlan verwendbar !!!

int enpin[MOTOR_OUTPUTS] = {27, 23}; // open drain output vom VNH5019, board verbindet en/diag A+B,
// The DIAGA/ENA or DIAGB/ENB, when connected to an external pull-up resistor, enable one leg of the bridge. They also provide a feedback digital diagnostic signal.


#define MOTOR_NR 0
#define HAVE_CS
#define HAVE_PWM_PIN
#define MOTOR_IN_BRAKE LOW

#define MOTOR_FREQUENCY 16000

// ============================================================================================================================= ESP32 wemos DRV8871 motor shield
#elif defined DRV8871
// ************* drv8871 kein extra pwm pin. pwm geht über INA + INB, je nach richtung
#define MOTOR_OUTPUTS 1
int inApin[MOTOR_OUTPUTS] = {13}; // INA + PWM
int inBpin[MOTOR_OUTPUTS] = {12}; // INB + PWM

#define MOTOR_FREQUENCY 16000
#define MOTOR_NR 0
#define MOTOR_IN_BRAKE HIGH

// ============================================================================================================================= I2C Monster Shield
#elif defined LOLIN_I2C_MOTOR_SHIELD
// motor shield v2.0.0
// https://github.com/wemos/LOLIN_I2C_MOTOR_Library
#include <LOLIN_I2C_MOTOR.h>

#define MOTOR_FREQUENCY 16000
#define MOTOR_NR 0
LOLIN_I2C_MOTOR motor; //I2C address 0x30
#error untested

#elif defined WEMOS_I2C_MOTOR_SHIELD
// motor shield v1.0.0
// https://github.com/wemos/WEMOS_Motor_Shield_Arduino_Library
#include <WEMOS_Motor.h>
#define MOTOR_NR 0
//Motor shiled I2C Address: 0x30
//PWM frequency: default: 1000Hz(1kHz)
// WEMOS_Motor.cpp    Wire.write(((byte)(freq >> 24)) & (byte)0x0f); !!!!!!!!!! richten !!!!!!!!!!
// https://github.com/pbugalski/wemos_motor_shield/issues/9 => 10kHz geht ned mit der default firmware .........
/* Pathches an der wemos lib: WEMOS_Motor.cpp
 void Motor::setfreq(uint32_t freq)
 {
        Wire.beginTransmission(_address);
-       Wire.write(((byte)(freq >> 16)) & (byte)0x0f);
+// [chris]
+//     Wire.write(((byte)(freq >> 16)) & (byte)0x0f);
+       Wire.write(((byte)(freq >> 24)) & (byte)0x0f);
        Wire.write((byte)(freq >> 16));
        Wire.write((byte)(freq >> 8));
        Wire.write((byte)freq);
@@ -111,10 +113,11 @@ void Motor::setmotor(uint8_t dir, float pwm_val)
        Wire.endTransmission();     // stop transmitting
 
 
-       delay(100);
+#warning [chris] rauskommentiert
+//     delay(100);
 }
*/
Motor motor(0x30,_MOTOR_A, 1000);//Motor A

#else
#error No motor board defined!
#endif
// ************* motor shield defs done





void motorGo(uint8_t motor, uint8_t direct);

/*
void i2c_scan_bus(TwoWire &i2c) {
  Serial.println("Scanning I2C Addresses Channel 1");
  uint8_t cnt=0;
  for(uint8_t i=0;i<128;i++){
    i2c.beginTransmission(i);
    uint8_t ec=i2c.endTransmission(true);
    if(ec==0){
      if(i<16)Serial.print('0');
      Serial.print(i,HEX);
      cnt++;
    }
    else Serial.print("..");
    Serial.print(' ');
    if ((i&0x0f)==0x0f)Serial.println();
  }
  Serial.print("Scan Completed, ");
  Serial.print(cnt);
  Serial.println(" I2C Devices found.");
}


void i2c_scan() {
  // D2
#define SDA1 21
  // D1
#define SCL1 22

#define SDA2 17
#define SCL2 16

  TwoWire I2Cone = TwoWire(0);
  I2Cone.begin(SDA1,SCL1,400000); // SDA pin 21, SCL pin 22 TTGO TQ
  while(true) {
    i2c_scan_bus(I2Cone);
    Serial.println();
    delay(100);
  }
  / *
  TwoWire I2Ctwo = TwoWire(1);
I2Ctwo.begin(SDA2,SCL2,400000); // SDA2 pin 17, SCL2 pin 16 
  i2c_scan_bus(I2Cone);
Serial.println();
delay(100);
* /
}
*/


/**
 * wird mit new im btcontrol::setup() gestartet
 */
ESP32PWM::ESP32PWM() : USBPlatine(false), dir(0), pwm(0), motorStart(MOTOR_START), motorFullSpeed(255), ledToggle(0) {
#ifdef LOLIN_I2C_MOTOR_SHIELD
//   DEBUGF("start i2c scan");
//   i2c_scan();
  DEBUGF("waiting for LOLIN I2C motor shield");
  while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR) //wait motor shield ready.
  {
    unsigned char result=motor.getInfo();
    DEBUGF("product_id: %ud version: %ud, result: %ud", motor.PRODUCT_ID, motor.VERSION, result);
  }
  DEBUGF("setting frequency to %d", MOTOR_FREQUENCY);
  motor.changeFreq(MOTOR_CH_A /*MOTOR_CH_BOTH*/, MOTOR_FREQUENCY);
#elif defined WEMOS_I2C_MOTOR_SHIELD
  // default WEMOS firmware never has any return checks ...
  DEBUGF("WEMOS I2C motor shield");
#else
	for (int i=0; i<MOTOR_OUTPUTS; i++) {
		pinMode(inApin[i], OUTPUT);
		pinMode(inBpin[i], OUTPUT);
#ifdef HAVE_PWM_PIN
		pinMode(pwmpin[i], OUTPUT);
    pinMode(enpin[i], INPUT_PULLUP);
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
#endif // else LOLIN_I2C_MOTOR_SHIELD
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
  long start=millis();
  if(f_speed > 0) {
    this->pwm = f_speed*((double)this->motorFullSpeed - this->motorStart)/255 + this->motorStart;
  } else {
    this->pwm=0;
  }


#if MOTOR_IN_BRAKE==HIGH
  this->pwm=255-this->pwm;
#endif

  DEBUGF("ESP32PWM::setPWM set speed=%d => pwm=%d", f_speed, this->pwm);
#ifdef LOLIN_I2C_MOTOR_SHIELD
  motor.changeDuty(MOTOR_CH_A, this->pwm*100/256);
#elif defined WEMOS_I2C_MOTOR_SHIELD
  motor.setmotor(this->dir==0 ? _CW : _CCW, this->pwm*100/256);
#elif defined HAVE_PWM_PIN
  // analogWrite(pwmpin[motor], f_speed);
	ledcWrite(MOTOR_NR, this->pwm);
#ifdef PUTZLOK
#if MOTOR_OUTPUTS == 2
   if(this->doPutz)
     ledcWrite(1, this->pwm);
   else
     ledcWrite(1, this->pwm/3);
#else
   #warning Puttzlok braucht 2 Motor outputs
#endif // outputs
#endif // Putzlok
#else
  if(this->dir) {
    ledcWrite(0, this->pwm);
  } else {
    ledcWrite(1,  f_speed/3*((double)this->motorFullSpeed - this->motorStart)/255 + this->motorStart );
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
  DEBUGF("ESP32PWM::setPWM done in %ldms",millis()-start);
}

void ESP32PWM::setDir(unsigned char dir) {
  DEBUGF("ESP32PWM::setDir dir=%d", dir);
  long start=millis();
  // abrubpter richtungs wechsel??
  if(this->dir != dir) {
    this->fullstop(true, true);
  }
  this->dir=dir;
#ifdef LOLIN_I2C_MOTOR_SHIELD
  motor.changeStatus(MOTOR_CH_A, dir ? MOTOR_STATUS_CW : MOTOR_STATUS_CCW);
#else
#ifdef HAVE_PWM_PIN
	motorGo(MOTOR_NR, dir ? CW : CCW);
#ifdef PUTZLOK
  if(this->doPutz)
    motorGo(1, dir ? CW : CCW);
  else
    motorGo(1, dir ? CCW : CW);
#endif // PUTZLOK
#endif // HAVE_PWM_PIN
#endif
  DEBUGF("ESP32PWM::setDir done in %ldms",millis()-start);
}

void ESP32PWM::fullstop(bool stopAll, bool emergencyStop) {
  DEBUGF("ESP32PWM::fullstop %d %d", stopAll, emergencyStop);
#ifdef LOLIN_I2C_MOTOR_SHIELD
  motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_SHORT_BRAKE);
#elif defined WEMOS_I2C_MOTOR_SHIELD
  motor.setmotor(_STOP);
#else
#ifdef HAVE_PWM_PIN
	ledcWrite(MOTOR_NR, 0);
#ifdef PUTZLOK
  ledcWrite(1, 0);
#endif // PUTZLOK
#else
  ledcWrite(0,255);
  ledcWrite(1,255);
#endif
#endif
  lokdef[0].currspeed=0;
}

void ESP32PWM::setFunction(int nFunc, bool *func) {
// Print func values:
    Serial.print("=================setFunction ");
    for(int i=0; i < nFunc; i++) {
      Serial.printf("[%d]=%d, ",i, func[i]);
    }
    Serial.println("~~~~~~~~~~~~~~~~~~~~~");

#ifdef HAVE_SOUND
    Audio.startPlayFuncSound();
#endif
  if(nFunc > 0 ) {
#ifdef HEADLIGHT_PIN
    if(this->headlight != func[0]) {
      printf("set LEDC headlight %d\n", func[0]);
      ledcWrite(2, func[0] ? HEADLIGHT_PWM_DUTY : 0);
      this->headlight = func[0];
    }
#endif
#ifdef PUTZLOK
  // Putzlok ein/aus ist immer auf F3
   if(nFunc > 3 ) {
     if(this->doPutz != func[3]) {
      DEBUGF("setting doPutz to %d, curr=%d", this->doPutz, func[3]);
       if(this->pwm != 0) {
         ERRORF("won't switch putz flag if loco is not stopped");
         // set lokdef - func to old value
         lokdef[0].func[3].ison=this->doPutz;
       } else {
         this->doPutz = func[3];
       }
     }
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

#ifdef MOTOR_OUTPUTS
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
#endif
