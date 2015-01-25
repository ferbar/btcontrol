#include <DigiUSB.h>
#include <core_timers.h>
#include <ToneTimer.h>

// the setup routine runs once when you press reset:
void setup() {
  // 8kHz Prescaler
  ToneTimer_ClockSelect(Timer0_Prescale_Value_8);
  // Timer0_SetWaveformGenerationMode(Timer0_Fast_PWM_FF);
  // TOP=>PWM0 value: OCRA=128=> 16kHz
  Timer0_SetWaveformGenerationMode(Timer0_Fast_PWM_OCR);
  Timer0_SetOutputCompareMatchAndClear(128);

  // initialize the digital pin as an output.
  pinMode(0, OUTPUT); //LED on Model B // debug pin
  pinMode(1, OUTPUT); //LED on Model A   
  pinMode(5, OUTPUT); // scheint weniger strom zu liefern als der 1er  
  analogWrite(1,10);
  DigiUSB.begin();
  digitalWrite(5, HIGH);
  for(int i=0; i < 5 ; i++) { DigiUSB.refresh(); delay(100); }
  digitalWrite(5, LOW);
  for(int i=0; i < 5 ; i++) { DigiUSB.refresh(); delay(100); }
  digitalWrite(5, HIGH);
  for(int i=0; i < 5 ; i++) { DigiUSB.refresh(); delay(100); }
  digitalWrite(5, LOW);
  analogWrite(1,0);
}

char line[20];
int pos=0;
int pwm=0;
int pwm_hw=0;

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
    pwm=(c1<<4) + c2;
    DigiUSB.println(pwm, HEX);
    // analogWrite(1,pwm);
    return true;
  } else if(pos==3 && line[0]=='D') {
    if(line[1] == '0') {
      digitalWrite(5, LOW);
      DigiUSB.println("d0");
      return true;
    } else if(line[1] == '1') {
      digitalWrite(5, HIGH);
      DigiUSB.println("d1");
      return true;      
    }
  }
  // invalid command:
  DigiUSB.print("EP="); DigiUSB.println(pos,DEC);
  return false;
}

// the loop routine runs over and over again forever:
void loop() {
   DigiUSB.refresh();
   if(pwm_hw<pwm) {
     analogWrite(1,++pwm_hw);
     delay(10);
   } else if(pwm_hw>pwm) {
     analogWrite(1,--pwm_hw);
     delay(10);
   }
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
       digitalWrite(0, HIGH);
       char c = DigiUSB.read();
       if(pos < sizeof(line)) {
         line[pos]=c;
         pos++;
         if(line[pos-1] == '\n') {
           line[pos-1] = '\0';
           if(processCmd()) {
             // DigiUSB.println("ok");
           } else {
             DigiUSB.print("EC: ");
             DigiUSB.println(line);
           }
           pos=0;
         }
       } else {
         DigiUSB.print("EO");
         DigiUSB.println(pos,DEC);
         pos=0;
       }
       digitalWrite(0, LOW);
  }
}
