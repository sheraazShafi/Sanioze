#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"  //  RTC library
#include <Wire.h>   // i2c libary
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#define PowerPIN 13 //manual turn ON & OFF pin
#define SD_CS 5
#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DEVICENAME "SEMPER002"
#define SW 14
#define _IDLE 1
#define _BUSY 2
#define _UPLOAD 3
#define RXD2 34
#define TXD2 17
//Debug Data Points
long int globalCounter = 0;
long int LineCounter = 0;
const float offsetContactTime = 50;//320 Ct Default
const int UploadTimeSec  = 30;//30sec Default 
int lcdColumns = 16;// set the LCD number of columns and rows
int lcdRows = 2;
// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;  //  rtc class
DateTime now;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

float temp,humid;//temperature and humidity

const int O3Pin = 34;// Ozone sensor is connected to GPIO 34 (Analog ADC1_CH6) 
const int BAPin = 35;// Battery Monitoring is connected to GPIO 35 (Analog ADC1_CH7) 
float O3Value,O3adcValue,BAVoltage,BAPercent;// variables for storing the adc value
int O3numReadings = 50;    
int O3readings[50];// Number Samples to take

const int GreenLED = 15;
const int OrangeLED = 16;
const int BlueLED = 17;// the number of the LED pin

const int Relay = 26;
const int Mosfet = 25;//33
//25 = AUTO
//33 = HOTEL
const int filterFan = 25;
const int Serv = 27;
long int indexPoint;
float contactTime = 0;
float prevContactTime = 0;
float curContactTime = 0;

unsigned long prevMillis=0;
unsigned long interval=UploadTimeSec*1000;


int flag=1;
int Switchflag=0;
int Generatorflag=0;
bool motorStatus=false;
char rxData[50];
char statusPacket[15];
long int dataPointIndex;
int bookingID;
int status_=1;//1=IDLE:2=BUSY:3=UPLOAD PENDING
bool _uponConnect  = false;
bool ACK = false;
bool byPass = false;
int prevStatus = 1;
String path = "/BID";//DEF PATH PREFIX
String BookingStatusFile="";

//*************BLE CONFIG************************
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" //SERVICE UUID

#define NOTIFY_CH_UUID         "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" //NOTIFY CHARACTERISTIC

#define ACK_CH_UUID            "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" //ACK CHARACTERISTIC
//************************************************


bool deviceConnected = false;
bool oldDeviceConnected = false;
BLEServer *pServer;
BLEService *pService ;
BLECharacteristic *pCharacteristic;
BLECharacteristic *ackCharacteristic;



//SETUP FUNCTION PROTOTYPE
void init_gpio();
void init_lcd();
void init_sd();
void init_dht();
void initOled();
void init_ble();
//LOOP FUNCTION PROTOTYPE
void checkUpload();
void uploadData();
void uponConnect();
void updateUploadStatus();
void lcdDisplay();
void notify();
void control();
float getO3ppm();
float getBatt();
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
void emergencyPress();
void sendStatusPacket();
void printOled(float ppm);