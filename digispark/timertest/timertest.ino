#include <core_timers.h>
#include <ToneTimer.h>

void setup() {
    // timer 0 kann auf pin 1(OC0B)+ pin 0 (OC0A) schreiben, es geht nur pin1 wenn ein counter_max definiert wird
    // timer 1 kann auf pin 0|1 + pin 3|4 schreiben. 3+4 ist von usb belegt
    
    // ToneTimer_ClockSelect(Timer0_Prescale_Value_8); das ist das beim attiny85 das selbe wie:
    Timer0_ClockSelect(Timer0_Prescale_Value_8);
  // auf 16kHz setzen, PWM 128 => immer ein
  // Timer0_SetWaveformGenerationMode(Timer0_Fast_PWM_FF);
  // TOP=>PWM0 value: OCR0A-Register=128=> 16kHz
    Timer0_SetWaveformGenerationMode(Timer0_Fast_PWM_OCR);
  //  ToneTimer_SetWaveformGenerationMode( ToneTimer_(Fast_PWM_FF) );
    Timer0_SetOutputCompareMatchAndClear(128);
  // Pb0 pwm aus:
  // Timer0_SetCompareOutputModeA(Timer0_Disconnected); // TCCR0A = ( TCCR0A & ~MASK2(COM0A1, COM0A0) );
  Timer0_SetCompareOutputModeA(Timer0_Set); // TCCR0A = ( TCCR0A & ~MASK2(COM0A1, COM0A0) );
  // digitalWrite(0, HIGH);
  
  /*
 // Put Timer 1 in PWM mode, Set Timer 1 Prescaler to 64
  TCCR1 = (1<<PWM1A) | (1<<CS12) | (1<<CS11) | (0<<CS10);
  Timer1_SetCompareOutputModeA(Timer1_Set); // TCCR1=blah
  // when to switch OC1A off
  OCR1A =  56;
  OCR1B =  20;
  // when to reset counter
  OCR1C =  56*2;
  */
    //P0, P1, and P4 are capable of hardware PWM (analogWrite).
    pinMode(0, OUTPUT); //0 is P0, 1 is P1, 4 is P4 - unlike the analog inputs, 
                        //for analog (PWM) outputs the pin number matches the port number.
}

void loop() {
    for(int i =0; i < 20; i++) {
      OCR1A =  i*10;
      analogWrite(1,i*10);
      delay(100);
    }
  /*
    analogWrite(0,255); //Turn the pin on full (100%)
    delay(1000);
    analogWrite(0,192); //Turn the pin on half (50%)
    delay(1000);
    analogWrite(0,128); //Turn the pin on half (50%)
    delay(1000);
    analogWrite(0,64); //Turn the pin on half (50%)
    delay(1000);
    analogWrite(0,0);   //Turn the pin off (0%)
    delay(1000);
    */
}
