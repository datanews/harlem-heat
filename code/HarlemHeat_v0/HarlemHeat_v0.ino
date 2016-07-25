// 
// Harlem Heat Sensor Project
// John Keefe
// johnkeefe.net
// for WNYC.org
//
// v0 setup using RTC
// v1 add SD card writing


// RTC libarary from Adafruit
// https://github.com/adafruit/RTClib


// SD card libarary from Arduino

// LIBRARIES
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

// CONSTANTS
const int chipSelect = 10;  // for SD card

RTC_PCF8523 RTC;

void setup () {

  Serial.begin(57600);

  if (! RTC.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (! RTC.initialized()) {
    Serial.println("RTC is NOT running!");
  }
  
  // following line sets the RTC to the date & time this sketch was compiled
  // DO NOT USE here. Use HarlemHeat_TimeSetter instead.
  // Otherwise a board reset will RESET the time to the sketch-compile time!
  // RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));  
  
  //  pinMode(SDpower,OUTPUT);
  //  digitalWrite(SDpower,LOW);
  delay(1000);
   
   // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
     Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  

}

void loop () {
  
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

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
  
  delay(10000);
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
