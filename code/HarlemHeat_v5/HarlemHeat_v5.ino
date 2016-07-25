// Harlem Heat Project Sensor 
// John Keefe
// johnkeefe.net
//
// for WNYC.org and the Harlem Heat Project
// http://www.wnyc.org/series/harlem-heat-project
//
// v0 setup using RTC
// v1 add SD card writing
// v2 temp sensor and battery
// v3 slow-flash LED if card write fails
// v4 comment out all serial commands for space, added LED flashing functions
// v5 deleted serial command lines for clarity

// RTC libarary from Adafruit
// https://github.com/adafruit/RTClib

// SD card libarary from Arduino
// https://www.arduino.cc/en/Reference/SD

// Battery-monitoring code based on info from Adafruit at
// https://learn.adafruit.com/adafruit-feather-32u4-basic-proto/power-management

// library for the temperature humidity sensor
// get at https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib

// library for low-power sleep move
// get at http://jeelabs.net/projects/jeelib/wiki

// low-power sleep code from Jean-Claude Wippler
// http://jeelabs.net/pub/docs/jeelib/classSleepy.html and
// http://jeelabs.net/projects/jeelib/wiki
// Which is well-described here: http://jeelabs.org/2011/12/13/developing-a-low-power-sketch/
//
// Note that I'm using loseSomeTime() in place of delay(), and that going
// into low-power mode screws up the serial monitor feed! To debug, replace
// loseSomeTime(XXX) with delay(XXX).

// LIBRARIES
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <dht.h>
#include <JeeLib.h> 

// CONSTANTS
const int chipSelect = 10;  // for SD card
RTC_PCF8523 RTC;            // Real Time Clock
dht DHT;                    // temp sensor definitions
#define DHT22_PIN 5         // temp sensor pin
#define VBATPIN A9          // pin reading the battery
#define LEDPIN 13           // using the red LED on the feather

// VARIABLES
String unit_id = "HH099";
float temp_farenheit;
uint8_t mins_between_readings =  15;   // mins between readings, 0 to 255

// this is for the sleepy code
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

// this is for the sensor
struct
{
    uint32_t total;
    uint32_t ok;
    uint32_t crc_error;
    uint32_t time_out;
    uint32_t connect;
    uint32_t ack_l;
    uint32_t ack_h;
    uint32_t unknown;
} stat = { 0,0,0,0,0,0,0,0};

void setup () {

  //// LED SETUP ////
  pinMode(LEDPIN, OUTPUT);

  //// REAL TIME CLOCK SETUP ////
  if (! RTC.begin()) {
    // fast flash the LED to indicate a problem
    fastFlash();
  }

  if (! RTC.initialized()) {
    // fast flash the LED to indicate a problem
    fastFlash();
  }
  
  // following line sets the RTC to the date & time this sketch was compiled
  // DO NOT USE here. Use HarlemHeat_TimeSetter instead.
  // Otherwise any time you reset the board, the time will reset to the sketch-compile time!
  // RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));  

  //// SD CARD SETUP ////
  pinMode(chipSelect,OUTPUT);
  digitalWrite(chipSelect,LOW);
  delay(1000);
   
   // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    // slow flash the LED:
    slowFlash();
    return;
  }
}

void loop () {

  // Read Sensor Data

  // this code is from the original DHT library code above. it is probably
  // more involved than necessary, but broke when I tried to 
  // pare it down.
  
  // if (debug) { Serial.print("DHT22, \t"); }

  uint32_t start = micros();
  int chk = DHT.read22(DHT22_PIN);
  uint32_t stop = micros();

  stat.total++;
  switch (chk)
  {
  case DHTLIB_OK:
      stat.ok++;
      break;
  case DHTLIB_ERROR_CHECKSUM:
      stat.crc_error++;
      fastFlash();
      break;
  case DHTLIB_ERROR_TIMEOUT:
      stat.time_out++;
      fastFlash();
      break;
  case DHTLIB_ERROR_CONNECT:
      stat.connect++;
      fastFlash();
      break;
  case DHTLIB_ERROR_ACK_L:
      stat.ack_l++;
      fastFlash();
      break;
  case DHTLIB_ERROR_ACK_H:
      stat.ack_h++;
      fastFlash();
      break;
  default:
      stat.unknown++;
      fastFlash();
      break;
  }

  // convert celcius to farenheit because U.S.
  temp_farenheit = (DHT.temperature * 1.8) + 32;

  // read the battery
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // board divides voltage by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  
  //get the time
  DateTime now = RTC.now();
  
  // make a string for assembling the data to log:
  String dataString = "";

  dataString += unit_id;
  dataString += ",";
  dataString += now.unixtime();
  dataString += ",";
  dataString += now.year();
  dataString += "-";
  dataString += padInt(now.month(), 2);
  dataString += "-";
  dataString += padInt(now.day(), 2);
  dataString += " ";
  dataString += padInt(now.hour(), 2);
  dataString += ":";
  dataString += padInt(now.minute(), 2);
  dataString += ":";
  dataString += padInt(now.second(), 2);
  dataString += ",";
  dataString += temp_farenheit;
  dataString += ",";
  dataString += DHT.humidity;
  dataString += ",";
  dataString += measuredvbat;

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(unit_id + ".txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  } else {
    // if the file isn't open, indicate an error:
    slowFlash();
  }

  // Go to sleep!
  // loseSomeTime has a max of 60secs ... so looping for multiple minutes
  for (byte i = 0; i < mins_between_readings; ++i) {
    Sleepy::loseSomeTime(60000);
  } 

  // wait 5 seconds for sensor warmup
  delay(50000);
  
}


// nicely pad dates and times with leading zeros (so June is 06 instead of 6)
String padInt(int x, int pad) {
  String strInt = String(x);
  String str = "";
  
  if (strInt.length() >= pad) {
    return strInt;
  }
  
  for (int i=0; i < (pad-strInt.length()); i++) {
    str += "0";
  }
  
  str += strInt;
 
  return str;
}

// Flash LED fast to signify something is amiss
void slowFlash() {
       while (1 == 1) {
       // blink forever
       digitalWrite(LEDPIN, HIGH);
       delay(1000);
       digitalWrite(LEDPIN, LOW);
       delay(1000);
     }
}

// Flash LED fast to signify something is amiss
void fastFlash() {
       while (1 == 1) {
       // blink forever
       digitalWrite(LEDPIN, HIGH);
       delay(300);
       digitalWrite(LEDPIN, LOW);
       delay(300);
     }
}

