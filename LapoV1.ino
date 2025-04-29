/*
  PINOUT:
    green led = D2
    red led   = D3  
    W25Q CS   = D10
    W25Q SCK  = D13
    W25Q MISO = D12
    W25Q MOSI = D11
    SENSOR1   = A7
    SENSOR2   = A6
  
  ERROR blick number:
    1 - memory inizialization error
    2 - RTC inizialization error
*/

#include "SPI.h"
#include "SPIFlash.h"
#include "ArduinoJson.h" 
#include "RTClib.h"
#include "RTClib.h"

#include "class.h"

//Define Pins
#define GREEN_LED 2
#define RED_LED 3
#define MQ2_PIN A7 
#define FLASH_CS_PIN 10

// Inizializzazione dei componenti 
SPIFlash flash(FLASH_CS_PIN);
RTC_DS1307 rtc;

//Definizione costanti
const int W25Q128_SectorNumber=4096;
const uint32_t W25Q128_FirstDataAddress=0x1000;
const char SeparatorSimbol = '#';

//definizione oggetti e variabili globali
DataPoint dataPoint;
Configurations configs;
uint32_t  firstFreeAdress;

//#########################  SETUP  #########################

void setup() {
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(MQ2_PIN, INPUT);

  digitalWrite(RED_LED, HIGH);

  Serial.begin(9600);
  
  /*DateTime dt = {2025, 3, 24, 14, 30, 0};
  DataPoint dataPoint(dt, 300, 299, 8);
  String test= dataPoint.toCompactNotification();
  Serial.println(test);
  dataPoint.fromCompactNotification(test);
  Serial.println(dataPoint.temperature);*/ 

  if (!flash.begin()) {
    Serial.println("ERROR! Flash memory inizialization error");
    blinkError(1);
  }

  if (!rtc.begin()) {
    blinkError(2); 
  }
  if (! rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  //resetMemory();
  //writeConfigs();

  //Leggi le configurazioni dal primo settore della memoria
  String configsString=readConfigs();
  Serial.println(configsString);
  configs.fromJSON(configsString);
  Serial.println(configs.toJSON());
  //Ottieni la prima cella di memoria libera nell'area riservata ai dati
  firstFreeAdress=getFirstDataFreeAdress();

  while(configs.sensorWharmUpTime*1000 > millis()){
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    delay(500);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    delay(500);

    if(Serial && Serial.available()){
      commandElaboration();
    }

  }
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
}

//#########################  LOOP  #########################

void loop() {
  //Serial.println((uint32_t)firstFreeAdress);

  short gasRead = analogRead(MQ2_PIN);
  DateTime dt = rtc.now();
  
  dataPoint.reSet(dt, gasRead, 0, 0);
  String toWrite=dataPoint.toCompactNotification();
  Serial.println(toWrite); 
  firstFreeAdress+=writeDataPoint((uint32_t)firstFreeAdress, toWrite);

  //printData(); 
  delay(10000);
}

//#########################  READ  #########################
String readConfigs(){
  uint32_t address = 0x0000;
  String result = "";
  bool flag=0;
  for(int i=0 ;flag==0; i++){
    char byteRead = flash.readByte(address + i);
    Serial.print(byteRead);
    if(byteRead!='#' && byteRead!=0xFF && (byteRead >= 0x20 && byteRead <= 0x7E)) result += byteRead;
    else return result;
  }
  return result;
}

//#########################  WRITE  #########################
// Funzione per scrivere una stringa sulla memoria
int writeDataPoint(uint32_t address, String data) {
  data+='#';
  int writtenBytes=0;
  for (int i = 0; i < data.length(); i++) {
    int response=flash.writeByte(address + i, data[i]);
    if(response == 1) writtenBytes+=1;
  }
  return writtenBytes;
}

void writeConfigs(){
  uint32_t address = 0x0000;
  Configurations app(6,30,30);
  String jsonConf=app.toJSON();
  jsonConf+='#';
  for (int i = 0; i < jsonConf.length(); i++) {
    flash.writeByte(address + i, jsonConf[i]);
  }
}

//#########################  RESET  #########################

void resetMemory(){
  Serial.println("reset begin");
  delay(1000);
  flash.eraseChip();
  Serial.println("reset done");
}

//#########################  OTHER  #########################

uint32_t getFirstDataFreeAdress(){
  for (int i = 0; true; i++) {
    byte byteRead = flash.readByte(W25Q128_FirstDataAddress + i);
    if(byteRead==0xFF){
      return (uint32_t) W25Q128_FirstDataAddress + i;
    }
  }  
}

void printData(){
  for (int i = 0; true ; i++) {
    byte byteRead = flash.readByte(W25Q128_FirstDataAddress + i);
    if(byteRead!=0xFF){
      Serial.print((char)byteRead);
    }
    else{
      Serial.println("");
      return 0;
    }
  }  
}

//#########################  ERROR  #########################

void blinkError(int errorNumber){
  digitalWrite(GREEN_LED, LOW);
  while(1){
    for(int i=0;i<errorNumber;i++){
      digitalWrite(RED_LED, HIGH);
      delay(500);
      digitalWrite(RED_LED, LOW);
      delay(500);
    }
    delay(1500);
  }
}

//#########################  COMMAND LINE  #########################

int commandElaboration(){
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  Serial.println("command line...");

  for(int out=0; out==0;){
    if(Serial.available()){
      String data=Serial.readString();
      data.trim();
      int spaceIndex = data.indexOf(" ");
      String command = data.substring(0, spaceIndex);
      //Serial.println(command);
      if(command=="help"){
        Serial.println("commands:");
        Serial.println("  reset-memory            - reset configs and data from memory");
        Serial.println("  get-configs             - get configs from memory");
        Serial.println("  print-data              - print data from memory");
        Serial.println("  exit                    - exit from command line mode");
      }
      else if(command=="reset-memory"){
        resetMemory();
      }
      else if(command=="get-configs"){
        Serial.println(configs.toJSON());
      }
      else if(command=="print-data"){
        printData();
      }
      else if(command=="exit"){
        Serial.println("exit from command line...");
        return 0;
      }
      else{
        Serial.println(command+" it is not a command");
      }
      Serial.println("");
    }
  }  
}
