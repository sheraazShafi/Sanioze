//CURRENT VERSION
#include<Arduino.h>


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

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


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

void setup() {
  // put your setup code here, to run once:
init_gpio();//INITIALIZE GPIO Pins
init_lcd(); //INITIALIZE LCD DISPLAY
init_ble();//INITIALIZE BLE SERVER with Notify and Read/Write Characteristics
init_sd();//INITIALIZE SD Card
init_dht();//INITIALIZE DHT Sensor
initOled();
Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
 u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
 u8g2.print("Ready");   // requires enableUTF8Print()        


}

/****************************************************************************************************
****************************************************************************************************/
void loop() {
  
  
  unsigned long currentMillis = millis();
  
  now = rtc.now();//GET RTC DATA
  
  temp = dht.readTemperature();//Read Temperature
  humid = dht.readHumidity();//Read Humidity
  
  BAPercent = getBatt();//Get Battery %
  O3Value = getO3ppm();//GET Ozone PPM (PPM);
  Serial.print("OZONE PPM");
  Serial.print(O3Value);
  Serial.print("HUMID:");
  Serial.print(humid);
  Serial.print("TEMP:");
  Serial.print(temp);
  Serial.print("ContactTime:");
  Serial.println(contactTime);
  if(contactTime >= offsetContactTime)
  { 
    status_ = _UPLOAD;

    digitalWrite(Mosfet,LOW);
   

    
    }

    /*
if(digitalRead(filterFan)==HIGH && O3Value<=1)
{
  digitalWrite(filterFan,LOW);
  status_ = _UPLOAD;
}
    */
  //Serial.println(O3Value);
  //"O3(ppm), Temperature(C), Humidity(%), BatteryVoltage(V), MotorStatus, YYYY/MM/DD,HH:MM:SS\r\n"
//  String writeToFile =String(O3Value)+ ":" + String(temp)+ ":" + String(humid)+ ":" + String(BAPercent)+ "," + String(motorStatus)+ ","+String(now.year(), DEC) + "/" + String(now.month(), DEC) + "/" + String(now.day(), DEC)+ "," + String(now.hour(), DEC) + ":" + String(now.minute(), DEC)+ ":" + 
//                String(now.second(), DEC)+','+String(deviceConnected)+"\r\n";
  String writeToFile =String(dataPointIndex)+":"+String(O3Value)+ ":" + String(temp)+ ":" + String(humid)+":"+String(contactTime)+":"+String(now.minute())+":"+String(now.second())+"\r\n";
  
  Serial.println(writeToFile);
  Serial.print("DATAPOINT");
  Serial.println(dataPointIndex);
  
  if(deviceConnected )
  {    
    Serial.println("Notify Change");
  }
  
if (!deviceConnected && oldDeviceConnected) 
{
  ACK =false;
  _uponConnect =false;
  pServer->startAdvertising();
oldDeviceConnected = deviceConnected;
}

if (deviceConnected && !oldDeviceConnected) {
      
      
      //STEPS TO FOLLOW UPON CONNECT
    //SEND STS PACKET -> STS:[STATUS]:[BOOKING ID]
    //RCV  STRT -> STRT:[BOOKING ID]
    //SEND ACK  PACKET -> STRTACK
    
    
    if(ACK == true)
    notify();//NOTIFY

    if(_uponConnect==false)
    uponConnect();
    
    sendStatusPacket();

    if(ACK==true);
    uploadData();//Check for Upload Req and Upload data to APP
    checkUpload();

   


      
      }


  if(currentMillis - prevMillis >=interval)
  {
  prevMillis = currentMillis;
  appendFile(SD, path.c_str(),writeToFile.c_str());
  updateUploadStatus();
  }

lcdDisplay();
control();//check for recived data and execute required operation
printOled(O3Value);

  
}

/****************************************************************************************************
****************************************************************************************************/



class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
       // ACK =false;
  //_uponConnect =false;
      Serial.println("Device Disconnected..");

      
    }
};

class ackCallBacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *ackCharacteristic) {
      std::string rxdata = ackCharacteristic->getValue();
      int i=0;  
            
            if (rxdata.length() > 0) {
        for (i = 0; i < rxdata.length(); i++)
          rxData[i] = rxdata[i];
        rxData[i] = '\0';//STRING CONSTRUCTION
      }
      Serial.print("RXD DATA:");
      Serial.println(rxData);
    }
};





void init_ble()
{
 BLEDevice::init(DEVICENAME);//BLE NAME
 pServer = BLEDevice::createServer(); //Create BLE Server
 pServer->setCallbacks(new MyServerCallbacks());//Create Callback Functions
 pService = pServer->createService(SERVICE_UUID);// Create the BLE Service
 pCharacteristic = pService->createCharacteristic(
                            NOTIFY_CH_UUID,
                            BLECharacteristic::PROPERTY_NOTIFY);//CREATE BLE CHARACTERISTIC Type:NOTIFY
 pCharacteristic->addDescriptor(new BLE2902());//ADD DESCRIPTOR
 
 ackCharacteristic =            pService->createCharacteristic(
                                         ACK_CH_UUID,
                                         BLECharacteristic::PROPERTY_WRITE|
                                         BLECharacteristic::PROPERTY_READ);
                                         //BLECharacteristic::PROPERTY_WRITE_NR);

 // ackCharacteristic->addDescriptor(new BLE2902());//ADD DESCRIPTOR  
                      
  //pCharacteristic->setCallbacks(new notifyCallBacks()); //NOTIFY CALL BACKS
  ackCharacteristic->setCallbacks(new ackCallBacks());//ACK CALL BACKS
  pService->start();
  pServer->getAdvertising()->start();// Start advertising
  Serial.println("Waiting a client connection to notify...");
  
  }


//*************************************************



void printOled(float ppm)
{
  
  u8g2.firstPage();
  do {    
 u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
 u8g2.print(String(ppm));   // requires enableUTF8Print()        

        /*  
          u8g2.setFont(u8g2_font_open_iconic_app_2x_t);
          u8g2.drawGlyph(30, 20, 71); conic_thing_2x_t);
          
      
          
   
          u8g2.setCursor(65-10,20);
          u8g2.setFont(u8g2_font_squirrel_tu);
          u8g2.print(0.02);
  
          u8g2.setFont(u8g2_font_open_i
          u8g2.drawGlyph(2, 40, 78); 
          u8g2.setFont(u8g2_font_squirrel_tu);
          u8g2.setCursor(18,40);
          u8g2.print(":19 C");   // requires enableUTF8Print() 
          
          u8g2.setFont(u8g2_font_open_iconic_thing_2x_t);
          u8g2.drawGlyph(75, 40, 72); 
          u8g2.setCursor(105-10,40);
          u8g2.setFont(u8g2_font_squirrel_tu);
          u8g2.print(":88%");   // requires enableUTF8Print() 
           u8g2.setCursor(6,60);
          u8g2.setFont(u8g2_font_squirrel_tu);
          u8g2.print("STATUS:RUNNING");   // requires enableUTF8Print() 

  */
  } while ( u8g2.nextPage() );
  
  }

#include"prototype.h"
void initOled()
{
    u8g2.begin();  
  u8g2.enableUTF8Print();
  }


void checkUpload()
{   
    String RxString = rxData;
    if(RxString.indexOf("UPCK:S")!=-1)
    {
    
    status_ = _IDLE;  
    }
 
  }

void uploadData()
{ 
LineCounter = 0;
 
  String RxString = rxData;
  if(RxString.indexOf("UPLD:")!=-1)//IF UPLD PACKET IS RXD
  {  
    
    
    Serial.println("UPLOADING DATA....");
    int CommaIndex= RxString.indexOf(":");
    indexPoint = (RxString.substring(CommaIndex+1)).toInt();
    Serial.print("IndexPoint:");
    Serial.println(indexPoint);
    //READ FROM [PATH] File 



File myFile = SD.open(path.c_str());

if (myFile) {
    Serial.println(path);

    // read from the file until there's nothing else in it:
    while (myFile.available() && deviceConnected==true) {
      
      status_ = _UPLOAD;
      if(deviceConnected==true)
      {
      status_ = _UPLOAD;
        String Line = "D:";
        Line += myFile.readStringUntil('\n');
        
        Serial.println(Line);
    
      if(LineCounter>=indexPoint)
      {
          //if(indexPoint==0)
          ackCharacteristic->setValue(Line.c_str());//Send Data Points to APP
     
          String ACKString = "DACK:";
          ACKString+=String(LineCounter);
          String RxString = rxData;

          Serial.println(ACKString);        
          Serial.println("Wait For DataACK..");
        while(!(RxString.indexOf(ACKString)!=-1) && deviceConnected==true)//WAIT TILL DATA ACK IS RXD
        {  
           ackCharacteristic->setValue(Line.c_str());//Send Data Points to APP
           Serial.print("TXD:");
           Serial.println(Line);
           RxString = rxData;
           Serial.println(RxString);
           delay(1000);
          }
        
      }

      LineCounter+=1;
      }
      else
      status_ = _UPLOAD;


        
      }
     if(deviceConnected==true)
     {
     ackCharacteristic->setValue("D:END");//SEND END OF FILE PACKET
     Serial.print("TXD:");
     Serial.println("D:END");
     }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }


ackCharacteristic->setValue("D:END");//Send Data Points to APP
RxString  = rxData;

while(!((RxString.indexOf("UPACK"))!=-1))//WAIT for UPCK Packet
{ RxString  = rxData;
  Serial.println("ReTransmit TXD: D:END");//Resending END packet 
  ackCharacteristic->setValue("D:END");//Send Data Points to APP
  delay(1000);
  if(deviceConnected==false)
  break;
  }

if(deviceConnected==true)
{
if(RxString.indexOf("UPACK:S")!=-1)
    status_ = _IDLE; 
if(RxString.indexOf("UPACK:F")!=-1)
    status_ = _UPLOAD; 

  
 }

}
  memset(rxData, 0, sizeof rxData);
}

void uponConnect()
{
delay(5000);//WAIT FOR STACK TO INIT

Serial.println("UPON CONNECT");
_uponConnect = true;
byPass = true;
sendStatusPacket();
String RxString = rxData;


while(!(RxString.indexOf("STRT:")!=-1))//CHECK IF RXD String is not having "STRT:"
{ 
  Serial.println("Wait for START PACKET");
  
  if(deviceConnected==false)
  {Serial.println("Device Disconnected..Closing Connection");
    break;
  }
  
  //sendStatusPacket();
  
  RxString = rxData;

}
byPass = false;

int CommaIndex= RxString.indexOf(":");
bookingID = (RxString.substring(CommaIndex+1)).toInt();
Serial.print("Booking ID:");
Serial.println(bookingID);
if(bookingID!=0)
{
ACK = true;
//CREATE A FILE WITH BookingID as FileName;
path+=(String(bookingID));
BookingStatusFile = path+"STATUS";

Serial.println("**********************");
Serial.println(path);
Serial.println("**********************");
delay(100);

File file = SD.open(path.c_str());
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    //writeFile(SD, path.c_str(), "O3(ppm):Temperature(C):Humidity(%):BatteryVoltage(V):MotorStatus:YYYY/MM/DD:HH/MM/SS:BleStatus\r\n");// print the headings for our data
  dataPointIndex = 0;
  }
  else {
    Serial.println("File already exists");  
    while(file.available())
    {
    file.readStringUntil('\n');//Dummy read
    dataPointIndex+=1;
    }
  }
  file.close();     

Serial.print("Number of Lines:");
Serial.println(dataPointIndex);
/***********************CREATE A FILE FOR STORING BookingID Upload STATUS***********************/
updateUploadStatus();



 Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
 Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
 uint64_t SDtotal = SD.totalBytes() / (1024 * 1024);
 uint64_t SDused = SD.usedBytes() / (1024 * 1024);
 if((SDtotal-SDused)<10){lcd.clear();lcd.setCursor(0, 0);lcd.print("SD card full"); } 




delay(250);
ackCharacteristic->setValue("STRTACK");//START ACK MSG : Device To APP

     Serial.print("TXD:");
     Serial.println("STRTACK");
//delay(250);
}

//memset(rxData, 0, sizeof rxData);
 
}

void updateUploadStatus()
{
  /***********************CREATE A FILE FOR STORING BookingID Upload STATUS***********************/
  File file = SD.open(BookingStatusFile.c_str());//CREATE A FILE TO STORE BID STATUS

  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating BID STATUS file...");
    writeFile(SD, BookingStatusFile.c_str(),"1");// Write IDLE Status upon New File Creation
  }
  else {
    Serial.println("File already exists");  
    writeFile(SD, BookingStatusFile.c_str(),String(status_).c_str());
    
    //delay(250);
  }
  file.close();     
  }

void sendStatusPacket()
{
statusPacket[20];
String sendText = "STS:";
sendText+=(String)status_;
sendText+=":";
sendText+=(String)bookingID;
sendText+=":";
sendText+=String(globalCounter);

Serial.println(sendText);



if(prevStatus!=status_ || byPass==true)//SEND STATUS PACKET WHEN THERE IS CHANGE IN STATUS
{  
  Serial.println("Change in Status");
//sendText.toCharArray(statusPacket,20);
//ackCharacteristic->setValue(statusPacket);//SEND STS:[VALUE]:BOOKINGID]
globalCounter++;
    pCharacteristic->setValue(sendText.c_str());
    
     Serial.print("NOTIFY:");
     Serial.println(sendText);
    pCharacteristic->notify();

}
prevStatus=status_;
  }


  
void lcdDisplay()
{ lcd.clear();
  lcd.print("O3 PPM:");
  lcd.print(O3Value);
  delay(1000);
  lcd.clear();
  lcd.print("Temperature:");
  lcd.print(temp);
  lcd.setCursor(0,1);
  lcd.print("Humidity:");
  lcd.print(humid);    
  delay(500);
  lcd.clear(); 
  lcd.setCursor(3,0);
  lcd.print("Battery:");
  lcd.print(BAPercent);
  delay(1000);
  lcd.clear();
}



void notify()
{
char notifyMsg[50];
    sprintf (notifyMsg, "%2.2f,%3.2f,%3.2f,%d,%2.2f\n", O3Value, temp, humid,motorStatus,BAVoltage);
    pCharacteristic->setValue(notifyMsg);
    pCharacteristic->notify();
    Serial.print("NOTIFY:");
    Serial.println(notifyMsg);    
}

void control()
{
  
  String rxDataString  = rxData;
  if(rxDataString.indexOf('a')!=-1 && status_ ==_IDLE)
  {
  Serial.println("GENERATOR ON!");
  digitalWrite(Mosfet, HIGH);
  delay(2500);
  motorStatus=true;
  status_ = _BUSY;
  digitalWrite(GreenLED,HIGH);
  digitalWrite(OrangeLED,HIGH);
  digitalWrite(BlueLED,HIGH);
  
  
  delay(100);
  }
  else if(rxDataString.indexOf('b')!=-1)
  {
    
    status_ = _IDLE;
  Serial.println("GENERATOR OFF!");
  digitalWrite(Mosfet, LOW);
  delay(1000);
  motorStatus=false;
  digitalWrite(GreenLED,HIGH);
  delay(100);
      digitalWrite(GreenLED,LOW);
  digitalWrite(OrangeLED,LOW);
  digitalWrite(BlueLED,LOW);

    }
  else if(rxDataString.indexOf('c')!=-1 && status_ ==_IDLE)
  {
    status_ = _BUSY;
    Serial.println("FILTER FAN ON!");
    digitalWrite(Relay, HIGH);
   }
  else if(rxDataString.indexOf('d')!=-1)
  {
    status_ = _IDLE;
    Serial.println("FILTER FAN OFF!");
    digitalWrite(Relay, LOW);
    }
   //memset(rxData, 0, sizeof rxData);
  }

void init_dht()
{
  dht.begin();  
  if (! rtc.begin()) {      //   initialize RTC
    //Serial.println("Couldn't find RTC");
    lcd.clear();// clears the display to print new message
    lcd.setCursor(0, 0);// set cursor to first column, first row
    lcd.print("Failure in RTC");
    lcd.setCursor(0, 1);// set cursor to first column, 2nd row
    lcd.print("REPLACE RTC/CELL");
    //while (1);
  }
  
  }



float getO3ppm()
{   
  

for (int k = 0; k < O3numReadings; k++) {
        O3readings[k] = analogRead(O3Pin);// take the reading, increment the cursor
        delay(2);}  // add a delay so you don't take all measurements at once.
   int O3sum = 0.0; // will hold the sum of all sensor readings.
   for (int l = 0; l < O3numReadings; l++) {// add up all the measurements...
            O3sum = O3sum + O3readings[l];}
        O3adcValue = O3sum / O3numReadings;// and divide by the number of measurements to get the average
  
  if(O3adcValue<300){O3Value=0.0;}//if(O3adcValue<496){O3Value=0.0;}
  else if(O3adcValue>2481){O3Value=10.0;} //y=mx+c ==>y is ppm & x is adc; m=2/397=0.00503778,c=-2.4987.
  else if(300 < O3adcValue < 2481){ O3Value = 10.0/2181.0*float(O3adcValue);
          O3Value = O3Value-(1000.0/727.0);}// O3 sensor gives data from 0.4(496adc) to 2v(2481adc) equivalent to 0-10ppm. 
  if(O3Value<=0.26)
  O3Value = 0;
  
  
  return O3Value;
  }




float getBatt()
{
    long sum = 0;                   // sum of samples taken
    BAVoltage = 0.0;            // calculated voltage
    float output = 0.0;             //output value
    const float battery_max = 13.60; //maximum voltage of battery
    const float battery_min = 12.0;  //minimum voltage of battery before shutdown
    for (int i = 0; i < 500; i++)
    {
        sum += analogRead(BAPin);
        delayMicroseconds(1000);
    }
    // calculate the voltage
    BAVoltage = sum / (float)500;
    BAVoltage = (BAVoltage * 15.95) / 4095.0;// PD is 100k&27k,multiplication factor=127/27=4.7037--->4.7037*3.3=15.52.In Vmax=14.6//error calib is 15.95
    BAVoltage = roundf(BAVoltage * 100) / 100;//round value by two precision
    //Serial.print("BAVoltage: ");
   // Serial.println(BAVoltage, 2);
    //Serial.print("BAPercent: ");Serial.println(BAPercent);
    if (BAVoltage < 12.20){digitalWrite(OrangeLED,HIGH);delay(100);}
    else if (BAVoltage > 13.60){digitalWrite(BlueLED,HIGH);delay(100);}
    else if (12.20<BAVoltage<13.60) {digitalWrite(OrangeLED,LOW);digitalWrite(BlueLED,LOW);delay(100);}  
  
    
    
    output = ((BAVoltage - battery_min) / (battery_max - battery_min)) * 100;
    if (output < 100 && output>0)
        return output;
    else if(output<0)
        return 0.00;
    else 
        return 100.0f;
  
  }

 




void init_rtc()
{
  
  dht.begin();  
  if (! rtc.begin()) {      //   initialize RTC
    //Serial.println("Couldn't find RTC");
    lcd.clear();// clears the display to print new message
    lcd.setCursor(0, 0);// set cursor to first column, first row
    lcd.print("Failure in RTC");
    lcd.setCursor(0, 1);// set cursor to first column, 2nd row
    lcd.print("REPLACE RTC/CELL");
    //while (1);
  }
}



void writeFile(fs::FS &fs, const char * path, const char * message) {// Write to the SD card (DON'T MODIFY THIS FUNCTION)
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
  
  Serial.printf("Appending to file: %s\n", path);
  Serial.println(message);
  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
  if(dataPointIndex==0)
  prevContactTime = 0;//Y1
  else
  { curContactTime = getO3ppm();//Y2
    contactTime += (prevContactTime+curContactTime)/4;//(y2-y1/2)*(x2-x1)
    prevContactTime = curContactTime;//(y1=y2)
   } 
   
 Serial.print("dataPointIndex: ");
 Serial.println(dataPointIndex);
 Serial.print("PREV CONTACT TIME: ");
 Serial.println(prevContactTime);
 Serial.print("CUURENT CONTACT TIME: ");
 Serial.println(curContactTime);
 Serial.print("CONTACT TIME: ");
 Serial.println(contactTime);
 
 dataPointIndex+=1;

}


void init_sd()
{
  
  SD.begin(SD_CS);  // Initialize SD card
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");lcd.clear();lcd.setCursor(0, 0);lcd.print("Card Mount Failed");return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");lcd.clear();lcd.setCursor(0, 0);lcd.print("No SD card attached");return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }
  File file = SD.open("/data.txt");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "YYYY/MM/DD, HH:MM:SS, O3(ppm), Temperature(C), Humidity(%), BatteryVoltage(V), MotorStatus \r\n");// print the headings for our data
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();     
 Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
 Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
 uint64_t SDtotal = SD.totalBytes() / (1024 * 1024);
 uint64_t SDused = SD.usedBytes() / (1024 * 1024);
 if((SDtotal-SDused)<10)
 {lcd.clear();
 lcd.setCursor(0, 0);
 lcd.print("SD card full"); }     
 }

void init_lcd()
{
  
  lcd.init();// initialize LCD 
  lcd.backlight();// turn on LCD backlight 
  lcd.setCursor(4, 0);// set cursor to 4th column, first row
  lcd.print("SEMPER");// print message
  lcd.setCursor(0, 1);// set cursor to first column, second row
  lcd.print("O3 Disinfector");delay(1000);
  lcd.clear();// clears the display to print new message
}

void emergencyPress()
{
 // digitalWrite(Mosfet,LOW);
  //digitalWrite(filterFan,HIGH);
  Serial.println("Emergency Press....");
  status_ = _IDLE;
    }
void init_gpio(){
  pinMode(OrangeLED, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(Mosfet, OUTPUT);
  pinMode(Serv, OUTPUT);
  pinMode(PowerPIN, INPUT);
  pinMode(SW,INPUT_PULLUP);
  pinMode(filterFan,OUTPUT);
  
  digitalWrite(Mosfet,HIGH);delay(3000);digitalWrite(Mosfet,LOW);
  digitalWrite(GreenLED,HIGH);
  digitalWrite(OrangeLED,LOW);
  digitalWrite(BlueLED,LOW);
  digitalWrite(Relay,LOW);
  digitalWrite(Mosfet,LOW);
  
attachInterrupt(SW, emergencyPress, FALLING);
  Serial.begin(9600);//DEBUG PORT Enabled
  }
