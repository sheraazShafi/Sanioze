//CURRENT VERSION
#include<Arduino.h>
#include"drivers.h"
#include"initCalls.h"
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

