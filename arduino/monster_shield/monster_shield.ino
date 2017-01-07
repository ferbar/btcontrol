/**
 * arduino - uno prog für ein monster shield (dual H Brücke)
 */
// http://garagelab.com/profiles/blogs/tutorial-how-to-use-the-monster-moto-shield
//

#define BRAKEVCC 0
#define CW 1
#define CCW 2
#define BRAKEGND 3
#define CS_THRESHOLD 30   // Definition of safety current (Check: "1.3 Monster Shield Example").
// 5V/1023 * CS_THRESHOLD = I/10000 * 1,6kOhm => CS_THRESHOLD = I/10000 * 1,6kOhm * 1023 / 5V

int target_pwm=0;

int inApin[2] = {7, 4}; // INA: Clockwise Direction Motor0 and Motor1 (Check:"1.2 Hardware Monster Motor Shield").
int inBpin[2] = {8, 9}; // INB: Counterlockwise Direction Motor0 and Motor1 (Check: "1.2 Hardware Monster Motor Shield").
int pwmpin[2] = {5, 6};            // PWM's input
int cspin[2] = {2, 3};              // Current's sensor input

int statpin = 13;

int sleep=8;

int dir=CW;

int hex2val(char c) {
  if(c >= '0' && c <= '9') {
    return c-'0';
  } else if(c >= 'a' && c <= 'f') {
    return c-'a'+10;
  }
  return -1;
}

void setup()                         
{
    Serial.begin(115200);              // Initiates the serial to do the monitoring 
    pinMode(statpin, OUTPUT);
    for (int i=0; i<2; i++)
        {
        pinMode(inApin[i], OUTPUT);
        pinMode(inBpin[i], OUTPUT);
        pinMode(pwmpin[i], OUTPUT);
        }
    for (int i=0; i<2; i++)
        {
        digitalWrite(inApin[i], LOW);
        digitalWrite(inBpin[i], LOW);
        }

    // base frequency = 62,5kHz 2=>durch 8
    TCCR0B = (TCCR0B & 0b11111000) | 2;
 
}


String line;

bool processCmd() {
    int len=line.length();
    if(len > 0 && (line.charAt(len-1) == '\r')) {
       line=line.substring(0,len-1);
    }
    if(line.startsWith("M")) {
        Serial.print("m");
        if(line.length() != 3) {
            Serial.print(F("-invalid length:")); Serial.print(line.length()); return false;
        }
        char c1=hex2val(line[1]);
        if(c1 < 0) {
            Serial.print(F("-invalid hex1:")); Serial.print(line[1]); return false;
        }
        char c2=hex2val(line[2]);
        if(c2 < 0) {
            Serial.print(F("-invalied hex2:")); Serial.print(line[2]); return false;
        }
        target_pwm=(c1<<4) + c2;
        Serial.println(target_pwm, HEX);
        motorGo(0, dir, target_pwm);
        return true;
    } else if(line.startsWith("D")) {
        if(line[1] == '0') {
            dir=CW;
            Serial.println(F("d0"));
        } else {
            dir=CCW;
            Serial.println(F("d1"));
        }
        return true;
    } else if(line == F("Q")) { // query debug info:
        //Serial.print(F("usb addr=")); DigiUSB.println(usbDeviceAddr,DEC);
        //Serial.print(F("usberrors=")); DigiUSB.println(usbError,DEC);
        Serial.print(F("speed=")); Serial.println(target_pwm,DEC);
        Serial.print(F("dir=")); Serial.println(dir,DEC);
        int cs = analogRead(cspin[0]);
        Serial.print(F("cs0=")); Serial.print(cs); Serial.print(F("\n"));
        cs = analogRead(cspin[1]);
        Serial.print(F("cs1=")); Serial.print(cs); Serial.print(F("\n"));
        return true;
    } else if(line == F("V")) { // version ausgeben:
        Serial.println(F("Version 1.1"));
#ifdef PUTZLOK
        DigiUSB.println(F("PUTZLOK"));
#endif
#ifdef CABLIGHT
    DigiUSB.println(F("CABLIGHT"));
#endif
        return true;
    }
    // invalid command:
    Serial.print(F("error:[")); Serial.print(line); Serial.print(F("]"));
    return false;
}

void loop()
{

// Routine to increase the speedo of the motor
    if (Serial.available()) {
        char c = Serial.read();
        if(c == '\n') {
            if(processCmd()) {
             // DigiUSB.println("ok");
            } else {
                Serial.print(F("EC: "));
                Serial.println(line);
            }
            line="";
        } else {
            line+=c;
        }
    }

/*
    delay(100*sleep);
    int cs = analogRead(cspin[0]);                             
    Serial.print("PWM:"); Serial.print(target_pwm, HEX);
    Serial.print(" CS:");
    Serial.print(cs);
    Serial.println();
*/

    return;
    
    int i =0;
    while(i<255) {
        motorGo(0, CW, i);   // Increase the speed of the motor, according to the value of i is increasing       
        delay(50*sleep);                          
        i++;
        int cs = analogRead(cspin[0]);                             
        Serial.print("PWM:");
        Serial.print(i);
        Serial.print(" CS:");
        Serial.print(cs);
        Serial.println();
      
        if (cs > CS_THRESHOLD) // If the motor locks, it will shutdown and...  
        {                                                                     // ...Resets the process of increasing the PWM
            motorOff(0);  
      
        }
        digitalWrite(statpin, LOW);
    }
    i=1;

//=========

//Keep the acceleration while the motor not locks

    while(i!=0)
        {                                      
        motorGo(0, CW, 255);       // Keep the PWM in 255 (Max Speed) 
        delay(50*sleep);
        int cs = analogRead(cspin[0]);                             
        Serial.print("PWM: max");
        Serial.print(" CS:");
        Serial.print(cs);
        Serial.println();
    
        if (cs > CS_THRESHOLD)     // If the motor locks, it will shutdown and....         
    
            {                                                                      // ...Resets the process of increasing the PWM
             motorOff(0); 
    
            }
    }

//========


}
void motorOff(int motor)     //Function to shut the motor down case it locks
{
    for (int i=0; i<2; i++) {
        digitalWrite(inApin[i], LOW);
        digitalWrite(inBpin[i], LOW);
    }
    analogWrite(pwmpin[motor], 0);
    digitalWrite(13, HIGH);
    Serial.println("Motor Locked (threshold reached)");
    delay(1000*sleep);
}

void motorGo(uint8_t motor, uint8_t direct, uint8_t pwm)         //Function that controls the variables: motor(0 ou 1), direction (cw ou ccw) e pwm (entra 0 e 255);
{
if (motor <= 1)
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

        analogWrite(pwmpin[motor], pwm);
        }
    }
}


