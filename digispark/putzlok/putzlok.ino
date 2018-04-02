/**
 * Digispark | attiny85 | arduino PWM erzeug-code
 * 
 * hints:
 *   wenns nach code änderung nix mehr tut sind vielleicht zu viele strings in verwendung. dann mit F() die strings kapseln!
 *   siehe: http://playground.arduino.cc/Learning/Memory
 *   
 * defines:
 *  CABLIGHT
 *    für 2 leds an P0 und P3
 *  PUTZLOK
 *    wenn man ein 2. dir bit braucht !!!broken mit Timer0_Fast_PWM_OCR!!!
 *  
 *  DEBUG_FLICKER
 *    das ist nur zum debuggen!!!
 *    
 *  DEBUG_FLICKER_IN_STOP
 *    flackert zwischen 0 und 1/255 im stillstand
 *
 * test für einen 2. PWM Ausgang
 * http://www.technoblogy.com/show?LE0
 * internet sagt dann mit interrupt machen
 *
 */

// #define USB_DEBUG_CABLIGHT

#define CABLIGHT
// scheint mir als könnte man nicht 2 PWM mit 20kHz rennen lassen - timer0 geht mit 16kHz nur auf PB1 und timer1 geht nur auf PB1 oder PB3
//#define PUTZLOK
// pwm led flackert bei jedem loop durchgang
//#define DEBUG_FLICKER
// pwm led flackert wenn speed==0
#define DEBUG_FLICKER_IN_STOP


#include <DigiUSB.h>
#include <core_timers.h>
#include <ToneTimer.h>


char line[20];
unsigned int pos=0;
// test - start pwm:
int target_pwm=0;
int pwm_hw=0;
int dir=0;

int debugloop=0;

#ifdef PUTZLOK
#define PWM_PUTZ_PIN 0
#define DIR_PUTZ_PIN 2
char putz=0;     // ein oder aus
int putz_pwm_hw=0; // aktueller pwm wert (+/-)
int putz_target_pwm;
#endif

#ifdef CABLIGHT
#define CABLIGHT_FRONT 0
#define CABLIGHT_REAR  2
#endif

extern uchar usbDeviceAddr;
void(*resetFunc)(void) = 0; // software reset
int usbError=0;

// the setup routine runs once when you press reset:
void setup() {
  // 8kHz Prescaler
  ToneTimer_ClockSelect(Timer0_Prescale_Value_8);
  // auf 16kHz setzen, PWM 128 => immer ein
  // Timer0_SetWaveformGenerationMode(Timer0_Fast_PWM_FF);
  // TOP=>PWM0 value: OCRA=128=> 16kHz
  Timer0_SetWaveformGenerationMode(Timer0_Fast_PWM_OCR);
  Timer0_SetOutputCompareMatchAndClear(128);

  // initialize the digital pin as an output.
  pinMode(1, OUTPUT); //LED on Model A // PWM motor
  pinMode(5, OUTPUT); // scheint weniger strom zu liefern als der 1er dir
#ifdef PUTZLOK
  pinMode(PWM_PUTZ_PIN, OUTPUT); //LED on Model B // PWM putzmotor
  pinMode(DIR_PUTZ_PIN, OUTPUT); //LED on Model A // dir putzmotor
  // for(int i=0; i < 22; i++) { analogWrite(0,i*10); delay(1000); }
#endif
#ifdef CABLIGHT
  pinMode(CABLIGHT_FRONT, OUTPUT);
  pinMode(CABLIGHT_REAR,  OUTPUT);
#endif
  analogWrite(1,10);
  DigiUSB.begin();
  digitalWrite(5, HIGH);
// weniger delay, das usb macht ja ping  
  for(int i=0; i < 10 ; i++) { DigiUSB.refresh(); DigiUSB.delay(50); }
  digitalWrite(5, LOW);
  analogWrite(1,0);
  // test:
  // for(int i=0; i < 10 ; i++) { DigiUSB.refresh(); DigiUSB.delay(50); }
}

int hex2val(char c) {
  if(c >= '0' && c <= '9') {
    return c-'0';
  } else if(c >= 'a' && c <= 'f') {
    return c-'a'+10;
  }
  return -1;
}

bool processCmd() {
  if(pos == 4 && line[0]=='M') {
    char c1=hex2val(line[1]);
    if(c1 < 0) return false;
    char c2=hex2val(line[2]);
    if(c2 < 0) return false;
    DigiUSB.print("m");
    // DigiUSB.print(c1,DEC); DigiUSB.print(":");DigiUSB.print(c2,DEC);DigiUSB.print(":");
    target_pwm=(c1<<4) + c2;
    DigiUSB.println(target_pwm, HEX);
    // analogWrite(1,target_pwm);
    return true;
  } else if(pos==3 && line[0]=='D') {
    if(line[1] == '0') {
      digitalWrite(5, LOW);
      DigiUSB.println("d0");
      dir=0;
      return true;
    } else if(line[1] == '1') {
      digitalWrite(5, HIGH);
      DigiUSB.println("d1");
      dir=1;
      return true;
    }
#ifdef PUTZLOK
  } else if(pos==3 && line[0]=='P') { // query putzlok debug info:
    putz=hex2val(line[1]);
    DigiUSB.print(F("p")); DigiUSB.println(putz,DEC);
    return true;
#endif
  } else if(pos==2 && line[0]=='Q') { // query debug info:
    DigiUSB.print(F("usb addr=")); DigiUSB.println(usbDeviceAddr,DEC);
    DigiUSB.print(F("usberrors=")); DigiUSB.println(usbError,DEC);
    DigiUSB.print(F("speed=")); DigiUSB.println(pwm_hw,DEC);
    DigiUSB.print(F("dir=")); DigiUSB.println(dir,DEC);
#ifdef PUTZLOK
    DigiUSB.print(F("putz=")); DigiUSB.println(putz, DEC);
    DigiUSB.print(F("putztarget=")); DigiUSB.println(putz_target_pwm, DEC);
#endif
    return true;
  } else if(pos==2 && line[0]=='V') { // version ausgeben:
    DigiUSB.println(F("Version 1.1"));
#ifdef PUTZLOK
    DigiUSB.println(F("PUTZLOK"));
#endif
#ifdef DEBUG_FLICKER
    DigiUSB.println(F("DEBUG_FLICKER"));
#endif
#ifdef DEBUG_FLICKER_IN_STOP
    DigiUSB.println(F("DEBUG_FLICKER_IN_STOP"));
#endif
#ifdef CABLIGHT
    DigiUSB.println(F("CABLIGHT"));
#endif
    return true;
  }
  // invalid command:
  DigiUSB.print(F("EP=")); DigiUSB.println(pos,DEC);
  return false;
}

#ifdef CABLIGHT
void setCablight() {
   if(pwm_hw == 0) {
     if(dir==0) {
       digitalWrite(CABLIGHT_FRONT,0);
     } else {
       digitalWrite(CABLIGHT_REAR,0);
     }
   } else {
     digitalWrite(CABLIGHT_FRONT,1);
     digitalWrite(CABLIGHT_REAR,1);
   }
}
#endif

// the loop routine runs over and over again forever:
void loop() {
  DigiUSB.refresh();

// usb nicht verbunden ?? -> reset
  if(usbDeviceAddr==0) {
    if(usbError>20) {
      // led kurz aufleuchten lassen (lok ruckt dann bissl)
      for(int x=0; x < 4 ; x++) {
        /*
        for(int i=0; i < 5 ; i++) { analogWrite(1,i*19); delay(50); }
        for(int i=5; i > 0 ; i--) { analogWrite(1,i*19); delay(50); }
        */
        analogWrite(1,1);
        DigiUSB.delay(300);
        analogWrite(1,0);
        DigiUSB.delay(300);
      }
      resetFunc();
    } else {
      usbError++;
      DigiUSB.delay(100);
      analogWrite(1,usbError%1);
    }
  } else {
    usbError=0;
  }

#if defined DEBUG_FLICKER || defined DEBUG_FLICKER_IN_STOP
#ifdef DEBUG_FLICKER_IN_STOP
  if(pwm_hw==0) {
#endif
    if((++debugloop % 10) == 0) {
      analogWrite(1,1);
      DigiUSB.delay(10);
    } else {
      analogWrite(1,0);
      DigiUSB.delay(10);
    }
#ifdef DEBUG_FLICKER_IN_STOP
  }
#endif
#endif
  
// test:analogWrite(1,20);
   if(pwm_hw<target_pwm) {
     analogWrite(1,++pwm_hw);
     DigiUSB.delay(10);
     #ifdef CABLIGHT
       setCablight();
     #endif
   } else if(pwm_hw>target_pwm) {
     analogWrite(1,--pwm_hw);
     DigiUSB.delay(10);
     #ifdef CABLIGHT
       setCablight();
     #endif
   }


#ifdef PUTZLOK
   if(putz) {
     putz_target_pwm=min(target_pwm,255); // TODO: formel
     putz_target_pwm*=-1;
   } else {
     putz_target_pwm=min(target_pwm/2,255);
   }
   if(dir==0)
     putz_target_pwm*=-1;
   
   int putz_changed=0;
   if(putz_target_pwm<putz_pwm_hw) {
     ++putz_pwm_hw;
     putz_changed=1;
   } else if(pwm_hw>target_pwm) {
     --putz_pwm_hw;
     putz_changed=1;     
   }
   if(putz_changed) {
     //  DigiUSB.print("p"); // DigiUSB.println(putz_pwm_hw, DEC);
     // digitalWrite(DIR_PUTZ_PIN, (putz_pwm_hw > 0) ? 1 : 0);
     analogWrite(PWM_PUTZ_PIN,abs(putz_pwm_hw));
   }
  
#endif

//  digitalWrite(0, HIGH);   // turn the LED on (HIGH is the voltage level)
     // digitalWrite(1, HIGH);
//  DigiUSB.print("1");
//  for(int i=0; i < 10 ; i++) { DigiUSB.refresh(); delay(100); }
//  delay(1000);               // wait for a second
//  digitalWrite(0, LOW);    // turn the LED off by making the voltage LOW
     // digitalWrite(1, LOW); 
//  DigiUSB.print("0");
//  for(int i=0; i < 10 ; i++) { DigiUSB.refresh(); delay(100); }
// delay(1000);               // wait for a second
  while(DigiUSB.available() > 0) {
#ifdef USB_DEBUG_CABLIGHT
       digitalWrite(CABLIGHT_FRONT, HIGH);
#endif
       char c = DigiUSB.read();
       if(pos < sizeof(line)) {
         line[pos]=c;
         pos++;
         if(line[pos-1] == '\n') {
           line[pos-1] = '\0';
           if(processCmd()) {
             // DigiUSB.println("ok");
           } else {
             DigiUSB.print(F("EC: "));
             DigiUSB.println(line);
           }
           pos=0;
         }
       } else {
         DigiUSB.print(F("EO"));
         DigiUSB.println(pos,DEC);
         pos=0;
       }
#ifdef USB_DEBUG_CABLIGHT
       digitalWrite(CABLIGHT_FRONT, LOW);
#endif
  }
}
