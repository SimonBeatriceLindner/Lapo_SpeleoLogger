#ifndef CLASS_H
#define CLASS_H

class Configurations {
  public:
  short sensorCalibrationOffset;
  short sensorWharmUpTime;
  short readingPeriod;
  
  // Costruttore della classe
  Configurations(short sco, short swt, short rp) {
    sensorCalibrationOffset = sco;
    sensorWharmUpTime = swt;
    readingPeriod = rp;
  }

  Configurations() {
    sensorCalibrationOffset = 0;
    sensorWharmUpTime = 30;
    readingPeriod = 30;
  }
  
  
  String toJSON() { // Metodo per convertire l'oggetto in JSON
    // Crea un oggetto JSON
    StaticJsonDocument<200> doc;
    
    // Aggiungi i dati dell'oggetto alla struttura JSON
    doc["sco"] = sensorCalibrationOffset;
    doc["swt"] = sensorWharmUpTime;
    doc["rp"] = readingPeriod;
    
    // Serializza l'oggetto JSON in una stringa e restituiscila
    String output;
    serializeJson(doc, output);
    return output;
  }

  void fromJSON(String json) { // Metodo JSON in oggetto
    // Crea un documento JSON
    StaticJsonDocument<200> doc;
    
    // Deserializza il JSON nella struttura doc
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
      Serial.print("Errore di deserializzazione: ");
      Serial.println(error.f_str());
      return;
    }
    
    // Assegna i valori del JSON alle variabili dell'oggetto
    sensorCalibrationOffset = doc["sco"].as<short>();  // Ottieni il nome
    sensorWharmUpTime = doc["swt"].as<short>();       // Ottieni l'età
    readingPeriod = doc["rp"].as<short>(); 
  }
};

class DataPoint{
  public:
  DateTime dateTime;
  short gas1;
  short gas2;
  short temperature;

  DataPoint(DateTime dt, short g1, short g2, short te){
    dateTime = dt;
    gas1 = g1;
    gas2 = g2; 
    temperature = te;
  }

  DataPoint(String compactNotification){
    String dateTimeStr = compactNotification.substring(0, 8);
    int year = dateTimeStr.substring(0, 4).toInt();
    int month = dateTimeStr.substring(4, 6).toInt();
    int day = dateTimeStr.substring(6, 8).toInt();
    String timeStr = compactNotification.substring(8, 14);
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(2, 4).toInt();
    int second = timeStr.substring(4, 6).toInt();
    DateTime dt = {year, month, day, hour, minute, second};
        
    gas1 = compactNotification.substring(14, 17).toInt();
    gas2 = compactNotification.substring(17, 20).toInt();; 
    temperature = compactNotification.substring(20, 23).toInt();;
  }

  DataPoint(){}

  String toCompactNotification(){
    String output = "";
    char dateTimeStr[9];
    sprintf(dateTimeStr, "%04d%02d%02d%02d%02d%02d", dateTime.year(), dateTime.month(), dateTime.day(), dateTime.hour(), dateTime.minute(), dateTime.second());
    char g1Str[5];
    sprintf(g1Str, "%03d", gas1);
    char g2Str[5];
    sprintf(g2Str, "%03d", gas2);
    char teStr[5];
    sprintf(teStr, "%03d", temperature);
    output = String(dateTimeStr) + g1Str + g2Str + teStr;
    return output;
  }

  void fromCompactNotification(String compactNotification){
    String dateTimeStr = compactNotification.substring(0, 8);
    int year = dateTimeStr.substring(0, 4).toInt();
    int month = dateTimeStr.substring(4, 6).toInt();
    int day = dateTimeStr.substring(6, 8).toInt();
    String timeStr = compactNotification.substring(8, 14);
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(2, 4).toInt();
    int second = timeStr.substring(4, 6).toInt();
    DateTime dt = {year, month, day, hour, minute, second};
        
    gas1 = compactNotification.substring(14, 17).toInt();
    gas2 = compactNotification.substring(17, 20).toInt();; 
    temperature = compactNotification.substring(20, 23).toInt();;
  }

  void reSet(DateTime dt, short g1, short g2, short te){
    dateTime = dt;
    gas1 = g1;
    gas2 = g2; 
    temperature = te;
  }
};

#endif
