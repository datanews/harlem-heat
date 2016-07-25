// 
// Harlem Heat Sensor Project
//
// Timesetter Program
// 
// Compile and upload to Feather to set the time!
//
// John Keefe
// johnkeefe.net
// for WNYC.org


// RTC libarary from Adafruit
// https://github.com/adafruit/RTClib


// LIBRARIES
#include <Wire.h>
#include "RTClib.h"

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
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));  
  Serial.println("Time set to sketch compile time");
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

  Serial.println(dataString);
  
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
