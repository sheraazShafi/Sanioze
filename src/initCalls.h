
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