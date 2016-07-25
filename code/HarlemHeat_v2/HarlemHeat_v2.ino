// 
// Harlem Heat Sensor Project
// John Keefe
// johnkeefe.net
// for WNYC.org
//
// v0 setup using RTC
// v1 add SD card writing
// v2 temp sensor and battery

/// ---- 
/// todo:
/// add temp code
/// output temp & humidity
/// add sleep code
/// add interrupt or clock trigger for wake and read


// RTC libarary from Adafruit
// https://github.com/adafruit/RTClib

// SD card libarary from Arduino

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

// VARIABLES
bool debug = false;
float temp_farenheit;
uint8_t mins_between_readings =  20;   // mins between readings, 0 to 255

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

  if (debug) { Serial.begin(57600); }

  //// REAL TIME CLOCK SETUP ////
  if (! RTC.begin()) {
    if (debug) { Serial.println(F("Couldn't find RTC")); }
    while (1);
  }

  if (! RTC.initialized()) {
    if (debug) { Serial.println(F("RTC is NOT running!")); }
  }
  
  // following line sets the RTC to the date & time this sketch was compiled
  // DO NOT USE here. Use HarlemHeat_TimeSetter instead.
  // Otherwise a board reset will RESET the time to the sketch-compile time!
  // RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));  

  //// SD CARD SETUP ////
  
  pinMode(chipSelect,OUTPUT);
  digitalWrite(chipSelect,LOW);
  delay(1000);
   
   // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
     if (debug) { Serial.println("Card failed, or not present"); }
    // don't do anything more:
    return;
  }
  

}

void loop () {

  // Read Sensor Data

  // this code is from the original noted above. it is probably
  // more involved than necessary, but broke when I tried to 
  // pare it down.
  if (debug) { Serial.print("DHT22, \t"); }

  uint32_t start = micros();
  int chk = DHT.read22(DHT22_PIN);
  uint32_t stop = micros();

  stat.total++;
  switch (chk)
  {
  case DHTLIB_OK:
      stat.ok++;
      if (debug) { Serial.print("OK,\t"); }
      break;
  case DHTLIB_ERROR_CHECKSUM:
      stat.crc_error++;
      if (debug) { Serial.print("Checksum error,\t"); }
      break;
  case DHTLIB_ERROR_TIMEOUT:
      stat.time_out++;
      if (debug) { Serial.print("Time out error,\t"); }
      break;
  case DHTLIB_ERROR_CONNECT:
      stat.connect++;
      if (debug) { Serial.print("Connect error,\t"); }
      break;
  case DHTLIB_ERROR_ACK_L:
      stat.ack_l++;
      if (debug) { Serial.print("Ack Low error,\t"); }
      break;
  case DHTLIB_ERROR_ACK_H:
      stat.ack_h++;
      if (debug) { Serial.print("Ack High error,\t"); }
      break;
  default:
      stat.unknown++;
      if (debug) { Serial.print("Unknown error,\t"); }
      break;
  }

  // convert celcius to farenheit because U.S.
  temp_farenheit = (DHT.temperature * 1.8) + 32;
  if (debug) { Serial.print("Humidity: "); }
  if (debug) { Serial.println(DHT.humidity); }
  if (debug) { Serial.print("Temperature (F): "); }
  if (debug) { Serial.println(temp_farenheit); }

  // read the battery
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  if (debug) { Serial.print("VBat: " ); Serial.println(measuredvbat); }
  
  //get the time
  DateTime now = RTC.now();
  
  // make a string for assembling the data to log:
  String dataString = "";
  
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
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    if (debug) { Serial.print("Wrote: "); }
    if (debug) { Serial.println(dataString); }
  }
  // if the file isn't open, pop up an error:
  else {
    if (debug) { Serial.println(F("error opening data file")); }
  }

  // Go to sleep!
  if (debug) {
    delay(10000);
  } else {
    // loseSomeTime has a max of 60secs ... so looping for multiple minutes
    for (byte i = 0; i < mins_between_readings; ++i) {
      Sleepy::loseSomeTime(60000);
    }
  }

  // allow 5 seconds for sensor warmup
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
