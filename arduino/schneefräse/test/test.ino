#include "Button2.h"

Button2 redButton;

Button2 actButton;

bool motorState=false;

#define LED_PIN 2
#define KUPPLUNG_PIN D5
#define MOTOR_FET D1

void switchOff() {
    digitalWrite(MOTOR_FET, 1); // invertiert
    digitalWrite(LED_PIN, 1);
    motorState=false;
}

void setup() {
  Serial.begin(74880);  // laut doc speed vom boodloader...
  Serial.println("");
  Serial.println("setup");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(MOTOR_FET, 1);
  pinMode(MOTOR_FET, OUTPUT);
  pinMode(KUPPLUNG_PIN, INPUT_PULLUP);
  
  switchOff();


  // put your setup code here, to run once:
  redButton.begin(D7, INPUT_PULLUP, false, true); // pullup, inverted
  redButton.setClickHandler( [](Button2& b){
    Serial.println("click");
    switchOff();
  });
  redButton.setLongClickTime(1000); // 1s
  redButton.setLongClickDetectedHandler( [](Button2& b){
    Serial.println("longclick");
    if(digitalRead(KUPPLUNG_PIN) == false) { // invertiert
      digitalWrite(MOTOR_FET, 0); // invertiert
      digitalWrite(LED_PIN, 0);
      motorState=true;
      Serial.printf("Kupplung: %d\n",digitalRead(KUPPLUNG_PIN));
    } else {
      Serial.println("Kupplung?");
      
    }
  });
  Serial.printf("Kupplung: %d\n",digitalRead(KUPPLUNG_PIN));
}

void loop() {
  // put your main code here, to run repeatedly:
  redButton.loop();
  delay(1);
  if(motorState == true && digitalRead(KUPPLUNG_PIN) == true) {
    Serial.println("kupplung offen");
    switchOff();
  }
}
