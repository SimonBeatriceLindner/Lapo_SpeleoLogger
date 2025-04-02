/*
  PINOUT:
    green led = D2
    red led   = D3  
    W25Q CS   = D10
    W25Q SCK  = D13
    W25Q MISO = D12
    W25Q MOSI = D11
    MQ2       = A7
*/

#include "SPI.h"
#include "SPIFlash.h"
#include "ArduinoJson.h" 
#include "RTClib.h"
#include "MQ2_LPG.h"
#include "RTClib.h"

#include "class.h"


//Define Pins
#define GREEN_LED 2
#define RED_LED 3
#define MQ2_PIN A7 
#define FLASH_CS_PIN 10

// Inizializzazione dei componenti 
SPIFlash flash(FLASH_CS_PIN);
MQ2Sensor mq2(MQ2_PIN);
RTC_DS1307 rtc;

//Definizione costanti
const int W25Q128_SectorNumber=4096;
const uint32_t W25Q128_FirstDataAddress=0x1000;
const char SeparatorSimbol = '#';

//MQ-2 settings
#define RL_Value 100 // 100K ohm
#define x1_Value 199.150007852152
#define x2_Value 797.3322752256328
#define y1_Value 1.664988323698715
#define y2_Value 0.8990240080541785
#define x_Value 497.4177875376839
#define y_Value 1.0876679972710004
#define Ro_Value 6.02
#define bitADC_Value 1023.0 // development board adc resolution
#define Voltage_Value 5.0 //????

//definizione oggetti e variabili globali
DataPoint dataPoint;
Configurations configs;
uint32_t  firstFreeAdress;

//#########################  SETUP  #########################

void setup() {
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);

  Serial.begin(9600);
  while (!Serial);

  /*DateTime dt = {2025, 3, 24, 14, 30, 0};
  DataPoint dataPoint(dt, 300, 299, 8);
  String test= dataPoint.toCompactNotification();
  Serial.println(test);
  dataPoint.fromCompactNotification(test);
  Serial.println(dataPoint.temperature);*/ 

  if (!flash.begin()) {
    Serial.println("ERROR! Flash memory inizialization error");
    while (1);
  }

  mq2.begin();

  if (!rtc.begin()) {
    Serial.println("[ERROR] rtc inizialization error");  
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
  }
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
}

//#########################  LOOP  #########################

void loop() {
  Serial.println((uint32_t)firstFreeAdress);

  calibration();
  double gasPPM = mq2.readGas()-configs.sensorCalibrationOffset;
  
  DateTime time = rtc.now();
  
  Serial.println(time.timestamp(DateTime::TIMESTAMP_FULL)+" - "+gasPPM);


  DateTime dt = {2025, 3, 24, 14, 30, 0};
  dataPoint.reSet(dt, 300, 299, 8);
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
  Serial.println("erase begin");
  delay(1000);
  flash.eraseChip();
  Serial.println("erase done");
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

//#########################  MQ-2  #########################

void calibration(){
  mq2.RL(RL_Value); // resistance load setting
  mq2.Ro(Ro_Value); // reverse osmosis setting
  mq2.Volt(Voltage_Value); // voltage sensor setting
  mq2.BitADC(bitADC_Value); // development board adc resolution setting
  mq2.mCurve(x1_Value, x2_Value, y1_Value, y2_Value); // mCurve setting
  mq2.bCurve(x_Value, y_Value); // bCurve setting
  mq2.getCalibrationData(); // get data calibration
}


