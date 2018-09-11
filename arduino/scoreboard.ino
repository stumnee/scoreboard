#include "Tlc5940.h"
#include <TimeLib.h>  
#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68

int photocellPin = 0;
int tempPin = 1;

int dBrightness = 0;
enum Mode {CLOCK, SECONDS, TEMP, SCORE, TEST};
Mode mode = CLOCK;

#define IRpin_PIN      PIND
#define IRpin          2

#define MAXPULSE 65000
#define NUMPULSES 50

#define RESOLUTION 20 
#define FUZZINESS 20

uint16_t pulses[NUMPULSES][2];  // pair is high and low pulse 
uint8_t currentpulse = 0; // index for pulses we're storing


unsigned long eventTime = 0;

int buttonMode = 7;
int buttonA = 6;
int buttonB = 5;

unsigned long buttonEventTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};

byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void readDS3231time(byte *second,
                    byte *minute,
  byte *hour,
  byte *dayOfWeek,
  byte *dayOfMonth,
  byte *month,
  byte *year) {
    
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void setup()
{
//  Wire.begin();
//
//  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
//  // retrieve data from DS3231
//  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
//
//  setTime(hour, minute, second, month, dayOfMonth, year);
//  
  pinMode(tempPin, INPUT);
  pinMode(photocellPin, INPUT);
  
  Tlc.init();
  Serial.begin(9600); 

   pinMode(buttonMode, INPUT);
   pinMode(buttonA, INPUT);
   pinMode(buttonB, INPUT);

}

/*       5             A
 *      ---           ---
 *   6 | 7 | 4      F| G |B
 *      ---           ---
 *   0 |   | 2      E|   |C
 *      ---    o      ---
 *       1     3       D     
 */

 /*
  *    TOP TLC 5940 Pinout
  *    
  *     12         8        4       0
  *    ----       ----     ---     ---    
  * 15| 14 |13 11| 10 |9 7| 6 |5 3| 2 |1   
  *    ----       ----     ---     ---    
  *    
  *    BOTTOM TLC 5490 Pinout
  *    
  * 14| 13 |12 10| 9  |8 6| 5 |4 2| 1 |0   
  *    ----  o    ----  o  ---  o  ---  o 
  *         15         11       7       3
  *   
  *   
  */

int digits[10][8] = {
{0, 1, 2, 4, 5, 6, -1},
{ 2, 4, -1},
{5, 4, 7, 0, 1, -1},
{5, 4, 7, 2, 1, -1},
{6, 7, 4, 2, -1},
{5, 6, 7, 2, 1, -1},
{5, 6, 7, 2, 1, 0, -1},
{5, 4, 2, -1},
{0, 1, 2, 4, 5, 6, 7, -1},
{7, 1, 2, 4, 5, 6, -1} 
};

int letterF[4] = {5, 6, 0, 7};
void tlcSet(int segment, int p) {
  //int b = map(analogRead(photocellPin), 50, 2000, 1, 250);
  //b += dBrightness;
  int b = 20;
  if (segment == 3) {
    b = constrain(int(b / 2), 1, 150);
  }
  Tlc.set(segment + p * 8, constrain(b, 1, 300));
}
void printDigit(int digit, int p) {
  for (int i = 0; i < 8; i ++) {
    int segment = digits[digit][i];
    if (segment == -1) {
      break;
    }
    tlcSet(segment, p);
  }
}

void printDot(int p) {
  tlcSet(3, p);
}

unsigned long serialEventTime = 0;
String content = "";
void processSerialCommand() {
  while (Serial.available() > 0) {
    char ch = Serial.read();
    content.concat(ch);
    serialEventTime = millis() + 10;
  }

   if (content != "" && serialEventTime < millis()) {  
    
    content.toLowerCase();
    content.trim();

    if (content.substring(0,5)  == "mode ") {
      content = content.substring(5);
      if (content == "clock") {
        mode = CLOCK;
      }
      if (content == "seconds") {
        mode = SECONDS;
      }
      if (content == "temp") {
        mode = TEMP;
      }

      if (content == "test") {
        mode = TEST;
      }
      
    } else if (content.substring(0, 11)  == "brightness ") {
      int i = content.substring(11).toInt();
      dBrightness = i;
      
    } else if (content.substring(0, 5)  == "time ") {
       content = content.substring(5);
       Serial.println("Set time:" + content);
       //1223
       if (content.length() == 4) {
         int hr = 10 * (content.charAt(0) - '0') + (content.charAt(1) - '0');
         int mn = 10 * (content.charAt(2) - '0') + (content.charAt(3) - '0');
         setTime(hr, mn, 0, 1, 1, 2015);
       }
       
    } else {
      Serial.println("Serial read: " + content);    
    }
    
    content = "";
  }
  
}

unsigned long printClockEventTime = 0;
void printClock() {

  if (printClockEventTime < millis()) {
    
    Tlc.clear();
  
    printDigit(floor(hour() / 10), 0);
    printDigit(hour() % 10, 1);
    printDigit(floor(minute() / 10), 2);
    printDigit(minute() % 10, 3);

    if (second()% 2 == 0) {
      printDot(1);
    }
    
    Tlc.update();
    printClockEventTime = millis() + 1000;  
  }
}

unsigned long printTempEventTime = 0;
void printTemp() {
  
  if (printTempEventTime < millis()) {
    float t = analogRead(tempPin) * 5000 /1024.0 / 10  * 9 / 5 + 32;
    //float t = analogRead(tempPin) * 900 /1024.0 + 32;
    Tlc.clear();

    if (t >= 100) {
      printDigit(int(t / 100), 0);
      printDigit(int(int(t)  % 100 / 10), 1);
      printDigit(int(t) % 10, 2);
    } else {
      printDigit(int(t / 10), 0);
      printDigit(floor(int(t) % 10), 1);
      printDot(1);
      printDigit(int(t * 10) % 10 , 2);  
    }
    // print F
    for (int i = 0; i < 4; i ++) {
      tlcSet(letterF[i], 3);
    }
    
    Serial.println(t);
    
    Tlc.update();
    printTempEventTime = millis() + 1000;  
  }
}

int scoreA = 0;
int scoreB = 0;
unsigned long printScoreEventTime = 0;
void printScore() {
  if (printScoreEventTime < millis()) {
    Tlc.clear();
  
    if (scoreA < 10) {
      printDigit(scoreA, 0);  
    } else {
      printDigit(int(scoreA / 10), 0);
      printDigit(int(scoreA % 10), 1);
    }
  
    printDigit(scoreB % 10, 3);  
    if (scoreB >= 10) {
      printDigit(int(scoreB / 10), 2);
    }  
    Tlc.update();
    printScoreEventTime = millis() + 1000;  
  }
}

int digi[4] = {0, 1, 2, 3};
int doti = 0;
void printTest() {
  if (eventTime + 1000 < millis()) {
    Tlc.clear();

     for (int i = 0; i < 4; i++) {
      Serial.print(digi[i]);
      printDigit(digi[i], i);
      digi[i]++;
      
      if (digi[i] > 9) {
        digi[i] = 0;
      }
      
     }
     Serial.println(' ');

     printDot(doti++);
     if (doti > 3) {
      doti = 0;
     }
     
     Tlc.update();
     eventTime = millis();
  }
}
void printSeconds() {

  if (printClockEventTime < millis()) {
    Serial.println(second());
    
    Tlc.clear();
  
    printDigit(floor(second() / 10), 2);
    printDigit(second() % 10, 3);

    Tlc.update();
    printClockEventTime = millis() + 1000;  
  }
}

unsigned long buttonModeEventTime = 0;
unsigned long buttonAEventTime = 0;
unsigned long buttonBEventTime = 0;

void buttonEvent(int btn, int hold) {
  switch (mode) {
      case CLOCK:   if (btn == buttonMode) {
                      mode = SECONDS;
                    }
                    if (btn == buttonA) {
                      setTime(hour() + 1 % 24, minute(), 0, 1, 1, 2015);
                    }
                    if (btn == buttonB) {
                      setTime(hour(), (minute() + 5) % 60, 0, 1, 1, 2015);
                    }
                    break;
      case SECONDS: if (btn == buttonMode) {
                      mode = TEMP;
                    }
                    if (btn == buttonA) {
                      dBrightness += 10;
                    }
                    if (btn == buttonB) {
                      dBrightness -= 10;
                    }
                    break;
      case TEMP:    if (btn == buttonMode) {
                      mode = SCORE;
                    }
                    break;
      case SCORE:   if (btn == buttonMode) {
                      mode = CLOCK;
                    }
                    if (btn == buttonA) {
                      if (hold > 2000) {
                        scoreA = 0;
                      } else if (hold > 500) {
                        scoreA--;
                      } else {
                        scoreA++; 
                      }
                    }
                    if (btn == buttonB) {
                      if (hold > 2000) {
                        scoreB = 0;
                      } else if (hold > 500) {
                        scoreB--;
                      } else {
                         scoreB++; 
                      }
                    }
                    if (scoreA > 99 || scoreA < 0) {
                      scoreA = 0;
                    }
                    if (scoreB > 99 || scoreB < 0) {
                      scoreB = 0;
                    }
                    break;
    }
}
void loop()
{
  int numberpulses;
  
 // numberpulses = listenForIR();

  //Serial.print(millis());
  //Serial.print(IRpin_PIN & _BV(IRpin) ? " high " : " low ");
  //Serial.println(IRpin_PIN);

  for (int btn = 5; btn <= 7; btn++) {
    if (digitalRead(btn) == HIGH) {
      if (buttonEventTime[btn] == 0) {
        buttonEventTime[btn] = millis();
      }   
    } else {
      if (buttonEventTime[btn] != 0) {
          unsigned long duration = millis()-buttonEventTime[btn];
          if (duration > 50) {
            buttonEvent(btn, duration);  
          }
          
          //Serial.println(millis()-buttonEventTime[btn]);
       }
       buttonEventTime[btn] = 0;
    }  
  }
    
  processSerialCommand();

  switch (mode) {
    case CLOCK:   printClock();
                  break;
    case SECONDS:     printSeconds();
                  break;
    case TEMP:     printTemp();
                  break;
    case SCORE:     printScore();
                  break;

    case TEST:    printTest();
                  break;
      
  }
  
}
int listenForIR(void) {
  currentpulse = 0;
  
  while (1) {
    uint16_t highpulse, lowpulse;  // temporary storage timing
    highpulse = lowpulse = 0; // start out with no pulse length
  
    while (IRpin_PIN & (1 << IRpin)) {
       // pin is still HIGH

       highpulse++;
       delayMicroseconds(RESOLUTION);

       if (((highpulse >= MAXPULSE) && (currentpulse != 0))|| currentpulse == NUMPULSES) {
         return currentpulse;
       }
    }
    pulses[currentpulse][0] = highpulse;
  
    while (! (IRpin_PIN & _BV(IRpin))) {
       lowpulse++;
       delayMicroseconds(RESOLUTION);
        if (((lowpulse >= MAXPULSE)  && (currentpulse != 0))|| currentpulse == NUMPULSES) {
         return currentpulse;
       }
    }
    pulses[currentpulse][1] = lowpulse;

    currentpulse++;
  }
}


