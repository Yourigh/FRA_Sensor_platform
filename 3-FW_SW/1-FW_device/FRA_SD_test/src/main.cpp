// A simple data logger for the Arduino analog pins with optional DS1307
// uses RTClib from https://github.com/adafruit/RTClib
#include <SPI.h>
#include "SdFat.h"
#include "sdios.h"
#include "FreeStack.h"

#define SD_CHIP_SELECT  17  // SD chip select pin
#define USE_DS1307       0  // set nonzero to use DS1307 RTC
#define LOG_INTERVAL    10  // mills between entries
#define SENSOR_COUNT     3  // number of analog pins to log   // JR, not working now, edited with maua; commands
#define ECHO_TO_SERIAL   0  // echo data to serial port if nonzero
#define WAIT_TO_START    0  // Wait for serial input in setup()
#define ADC_DELAY        0  // ADC delay for high impedence sensors

// file system object
SdFat sd;

// text file for logging
ofstream logfile;

// Serial print stream
ArduinoOutStream cout(Serial);

// buffer to format data - makes it eaiser to echo to Serial
char buf[80];
//------------------------------------------------------------------------------
#if SENSOR_COUNT > 6
#error SENSOR_COUNT too large
#endif  // SENSOR_COUNT
//------------------------------------------------------------------------------
// store error strings in flash to save RAM
#define error(s) sd.errorHalt(F(s))
//------------------------------------------------------------------------------
#if USE_DS1307
// use RTClib from Adafruit
// https://github.com/adafruit/RTClib

// The Arduino IDE has a bug that causes Wire and RTClib to be loaded even
// if USE_DS1307 is false.

#error remove this line and uncomment the next two lines.
//#include <Wire.h>
//#include <RTClib.h>
RTC_DS1307 RTC;  // define the Real Time Clock object
//------------------------------------------------------------------------------
// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = RTC.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
//------------------------------------------------------------------------------
// format date/time
ostream& operator << (ostream& os, DateTime& dt) {
  os << dt.year() << '/' << int(dt.month()) << '/' << int(dt.day()) << ',';
  os << int(dt.hour()) << ':' << setfill('0') << setw(2) << int(dt.minute());
  os << ':' << setw(2) << int(dt.second()) << setfill(' ');
  return os;
}
#endif  // USE_DS1307
//------------------------------------------------------------------------------
void setup() {
  #define ETH_CS 23
  #define ETH_EN 4
  pinMode(ETH_CS,OUTPUT);
  digitalWrite(ETH_CS,1);
  pinMode(ETH_EN,OUTPUT);
  digitalWrite(ETH_EN,1);
  pinMode(2,OUTPUT);

  Serial.begin(115200);
  
  // Wait for USB Serial.
  while (!Serial) {
    SysCall::yield();
  }
  // F() stores strings in flash to save RAM
  cout << endl << F("FreeStack: ") << FreeStack() << endl;

#if WAIT_TO_START
  cout << F("Type any character to start\n");
  while (!Serial.available()) {
    SysCall::yield();
  }
  // Discard input.
  do {
    delay(10);
  } while(Serial.available() && Serial.read() >= 0);
#endif  // WAIT_TO_START

#if USE_DS1307
  // connect to RTC
  Wire.begin();
  if (!RTC.begin()) {
    error("RTC failed");
  }

  // set date time callback function
  SdFile::dateTimeCallback(dateTime);
  DateTime now = RTC.now();
  cout  << now << endl;
#endif  // USE_DS1307

  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(SD_CHIP_SELECT, SD_SCK_MHZ(20))) {  //TO INCREASE ON FINAL PCB
    sd.initErrorHalt();
  }

  // create a new file in root, the current working directory
  char name[] = "logger00.csv";

  for (uint8_t i = 0; i < 100; i++) {
    name[6] = i/10 + '0';
    name[7] = i%10 + '0';
    if (sd.exists(name)) {
      continue;
    }
    logfile.open(name);
    break;
  }
  if (!logfile.is_open()) {
    error("file.open");
  }

  cout << F("Logging to: ") << name << endl;
  cout << F("Type any character to stop\n\n");

  // format header in buffer
  obufstream bout(buf, sizeof(buf));
  bout << F("w_duration  ,");
  bout << F("millis");

#if USE_DS1307
  bout << F(",date,time");
#endif  // USE_DS1307

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    bout << F(",sens") << int(i);
  }
  logfile << buf << endl;

#if ECHO_TO_SERIAL
  cout << buf << endl;
#endif  // ECHO_TO_SERIAL
}
//------------------------------------------------------------------------------
unsigned long a;

void loop() {
  uint32_t m;

  // wait for time to be a multiple of interval
  do {
    m = millis();
  } while (m % LOG_INTERVAL);

  // use buffer stream to format line
  obufstream bout(buf, sizeof(buf));

  //last row write time was:
  bout << a << ","; //log write time

  // start with time in millis
  bout << m;

#if USE_DS1307
  DateTime now = RTC.now();
  bout << ',' << now;
#endif  // USE_DS1307

  // read analog pins and format data 
/*
  for (uint8_t ia = 0; ia < SENSOR_COUNT; ia++) {
#if ADC_DELAY
    analogRead(ia);
    delay(ADC_DELAY);
#endif  // ADC_DELAY
    bout << ',' << analogRead(ia);
  }
*/
//MANUAL for ESP32 JR
bout << ',' << analogRead(A2);
bout << ',' << analogRead(A3);
bout << ',' << analogRead(A4);
  bout << endl;
digitalWrite(2,~digitalRead(2));
  // log data and flush to SD
  
  a = micros(); //measure write time
  logfile << buf << flush;
  a = micros() - a;
  
  // check for error
  if (!logfile) {
    error("write data failed");
  }

#if ECHO_TO_SERIAL
  cout << buf;
#endif  // ECHO_TO_SERIAL

  // don't log two points in the same millis
  if (m == millis()) {
    delay(1);
  }

  if (!Serial.available()) {
    return;
  }
  logfile.close();
  cout << F("Done!");
  digitalWrite(2,0);
  SysCall::halt();
}