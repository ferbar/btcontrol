#include <Arduino.h>
#include "ESP32PWM.h"
#include "utils.h"

static const char *TAG="ESP32PWM";

// Pins for bridge1 and bridge2 - see ARDUINO_SHIeLDLABEL
// ****** pins für china arduino monster shield !!!! nicht für sparkfun !!!!!
int inApin[2] = {14, 17}; // INA: Clockwise Direction Motor0 and Motor1 (Check:"1.2 Hardware Monster Motor Shield").
int inBpin[2] = {12, 13}; // INB: Counterlockwise Direction Motor0 and Motor1 (Check: "1.2 Hardware Monster Motor Shield").
int pwmpin[2] = {16, 27}; // PWM's input
int cspin[2] = {35, 34};  // Current's sensor input

int enpin[2] = {2, 4}; // open drain output vom VNH2SP30, 2 hat einen 1k PullDown R

// Motor 0 geht nicht: EN1 wird über einen R am ESP32 board auf 0 gezogen
#define MOTOR_NR 1
#define MOTOR_FREQUENCY 16000

ESP32PWM::ESP32PWM() : USBPlatine(false), dir(0), pwm(0), motorStart(70) {


	for (int i=0; i<2; i++) {
		pinMode(inApin[i], OUTPUT);
		pinMode(inBpin[i], OUTPUT);
		pinMode(pwmpin[i], OUTPUT);
		pinMode(enpin[i], INPUT);
	}
	for (int i=0; i<2; i++) {
		digitalWrite(inApin[i], LOW);
		digitalWrite(inBpin[i], LOW);

		ledcSetup(i, MOTOR_FREQUENCY, 8);
		ledcAttachPin(pwmpin[i], i);
	}
	for(int i=0; i<2; i++) {
		DEBUGF("Motor %d", i);
		DEBUGF("inA[%d]=%d", i, digitalRead(inApin[i]));
		DEBUGF("inB[%d]=%d", i, digitalRead(inBpin[i]));
		DEBUGF("en[%d]=%d", i, digitalRead(enpin[i]));
	}
}

ESP32PWM::~ESP32PWM() {
	fullstop();
}

void ESP32PWM::setPWM(int f_speed) {
	// analogWrite(pwmpin[motor], f_speed);
	ledcWrite(MOTOR_NR, f_speed);
}

void ESP32PWM::setDir(unsigned char dir) {
	motorGo(MOTOR_NR, dir ? CW : CCW);
}

void ESP32PWM::fullstop(bool stopAll, bool emergencyStop) {
	ledcWrite(MOTOR_NR, 0);
}

/**
 * Function that controls the variables:
 * @param motor(0 ou 1)
 * @param direction (BRAKEVCC CW CCW BRAKEGND)
 * @param pwm (0 255);
 */
void motorGo(uint8_t motor, uint8_t direct)
{
if (motor <= 1)
	DEBUGF("set motor[%d] dir %d\n", motor, direct);
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

