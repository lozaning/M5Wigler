#include <TinyGPS++.h>
#include <SPI.h>
#include <Wire.h>
#include <M5Stack.h>
#include "WiFi.h"
#include <SD.h>
#include <string>
#include <map>
#include <cstring>
#define ARDUINO_USD_CS 4
#define LOG_FILE_PREFIX "/gpslog"
#define MAX_LOG_FILES 100
#define LOG_FILE_SUFFIX "csv"
#define LOG_COLUMN_COUNT 11
#define LOG_RATE 500
#define NOTE_DH2 661 
using namespace std;

char logFileName[13];
int totalNetworks = 0;
unsigned long lastLog = 0;

const String wigleHeaderFileFormat = "WigleWifi-1.4,appRelease=2.26,model=Feather,release=0.0.0,device=arduinoWardriving,display=3fea5e7,board=esp8266,brand=Adafruit";
const char * log_col_names[LOG_COLUMN_COUNT] = {
  "MAC", "SSID", "AuthMode", "FirstSeen", "Channel", "RSSI", "Latitude", "Longitude", "AltitudeMeters", "AccuracyMeters", "Type"
};

TinyGPSPlus tinyGPS;
HardwareSerial ss(2);
File root;

std::map<uint8_t, string> my_map = {
    { 2, "[WPA-PSK-CCMP+TKIP][ESS]" },
    { 4, "[WEP][ESS]" },
    { 5, "[WPA2-PSK-CCMP+TKIP][ESS]" },
    { 7, "[ESS]" },
    { 8, "[WPA-PSK-CCMP+TKIP][WPA2-PSK-CCMP+TKIP][ESS]" }
};

uint8_t GetByte(uint8_t network){
    byte encryption = WiFi.encryptionType(network);
    return encryption;
} 

string GetDictionaryString(uint8_t value){
    try {
      return my_map.at(value);
    }
catch (const std::exception& e) {
        return "Unidentified";
    }

}

void setup() {
  M5.begin();
  Serial.begin(115200);
  ss.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (!SD.begin(ARDUINO_USD_CS)) {
    
    M5.Lcd.println("SD card error");
    M5.Lcd.println("Error initializing SD card.");
    while(true)
      delay(100);
  }
  
  M5.Lcd.println("SD card OK!");
  delay(500);
  updateFileName();
  printHeader();
}

void loop() {
      if (tinyGPS.location.isValid()) {
        lookForNetworks();
        screenWipe();
        M5.Lcd.print("Nets: ");
        M5.Lcd.println(totalNetworks);
        M5.Lcd.print("Sats: ");
        M5.Lcd.println(tinyGPS.satellites.value());
        M5.Lcd.print("Lat: ");
        M5.Lcd.println(tinyGPS.location.lat(), 3);
        M5.Lcd.print("Lng: ");
        M5.Lcd.println(tinyGPS.location.lng(), 3);
        
       } else {
        Serial.print("Location invalid. Satellite count: ");
        Serial.println(tinyGPS.satellites.value());
        screenWipe();
        M5.Lcd.println("GPS not locked");
        M5.Lcd.print("Sats: ");
        M5.Lcd.println(tinyGPS.satellites.value());
        M5.Lcd.print("Characters Processsed: ");
        M5.Lcd.println(tinyGPS.charsProcessed());
        M5.Lcd.print("Sentences With Fix: ");
        M5.Lcd.println(tinyGPS.sentencesWithFix());
        M5.Lcd.print("Failed Checksum: ");
        M5.Lcd.println(tinyGPS.failedChecksum());
      }
    smartDelay(LOG_RATE);
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      tinyGPS.encode(ss.read());
  } while (millis() - start < ms);
}


void lookForNetworks() {
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found");
    Serial.println("No networks found");
    M5.Lcd.fillScreen(RED);
  } else {
    for (uint8_t i = 0; i <= n; ++i) {
      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      if ((isOnFile(WiFi.BSSIDstr(i)) == -1) && (WiFi.channel(i) > 0) && (WiFi.channel(i) < 15)) { //Avoid erroneous channels
        totalNetworks++;
        File logFile = SD.open(logFileName, FILE_APPEND);
        Serial.print("New network found ");
        Serial.println(WiFi.BSSIDstr(i));
        M5.Lcd.setCursor(0,110);
        M5.Lcd.println("                                                         ");//clear the part of the LCD we're going to write the next ssid to
        M5.Lcd.setCursor(0,110);
        M5.Lcd.println("New Networks:");
        String SSID = WiFi.SSID(i);
        M5.Lcd.setCursor(0,130);
        M5.Lcd.println(SSID);
        logFile.print(WiFi.BSSIDstr(i));
        logFile.print(',');
        SSID = WiFi.SSID(i);
        // Commas in SSID brokes the csv file padding
        SSID.replace(",", ".");
        if(logFile.print(SSID)){
          Serial.println("************************************************************************Message appended");
    } else {
        Serial.println("******************************************************************************Append failed");
    
        }
        //logFile.print(SSID);
        logFile.print(',');

        string abc = "";

        
        string test = GetDictionaryString(GetByte(i));
        const char *str = test.c_str();
        logFile.print(str);
        logFile.print(',');
        logFile.print(tinyGPS.date.year());
        logFile.print('-');
        logFile.print(tinyGPS.date.month());
        logFile.print('-');
        logFile.print(tinyGPS.date.day());
        logFile.print(' ');
        logFile.print(tinyGPS.time.hour());
        logFile.print(':');
        logFile.print(tinyGPS.time.minute());
        logFile.print(':');
        logFile.print(tinyGPS.time.second());
        logFile.print(',');
        logFile.print(WiFi.channel(i));
        logFile.print(',');
        logFile.print(WiFi.RSSI(i));
        logFile.print(',');
        logFile.print(tinyGPS.location.lat(), 6);
        logFile.print(',');
        logFile.print(tinyGPS.location.lng(), 6);
        logFile.print(',');
        logFile.print(tinyGPS.altitude.meters(), 1);
        logFile.print(',');
        logFile.print((tinyGPS.hdop.value(), 1));
        logFile.print(',');
        logFile.print("WIFI");
        logFile.println();
        logFile.close();
      }
    }
  }
   //M5.Speaker.tone(NOTE_DH2, 20);
}

int isOnFile(String mac) {
  
  File netFile = SD.open(logFileName);
  String currentNetwork;
  if (netFile) {
    while (netFile.available()) {
      currentNetwork = netFile.readStringUntil('\n');
      if (currentNetwork.indexOf(mac) != -1) {
        //Serial.println("The network was already found");
        //Serial.println("The network was already found");
        //Serial.println("The network was already found");
        //Serial.println("******************************************************************");
        netFile.close();
        //M5.Lcd.print("We think we already found this network debug????");
        //M5.Lcd.print("The index of the network is: ");
        //M5.Lcd.println(currentNetwork.indexOf(mac));
        return currentNetwork.indexOf(mac);
      }
    }
    netFile.close();
    Serial.println("We dont think we've seen this network before");
    //M5.Lcd.println("We dont think we've seen this network before");
    //M5.Lcd.print("The index of the network is: ");
    //M5.Lcd.println(currentNetwork.indexOf(mac));
    return currentNetwork.indexOf(mac);
  }
  Serial.println("netFile was not true");
  //M5.Lcd.print("netFile was not true");
}

void printHeader() {
  File logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    int i = 0;
    logFile.println(wigleHeaderFileFormat); // comment out to disable Wigle header
    for (; i < LOG_COLUMN_COUNT; i++) {
      logFile.print(log_col_names[i]);
      if (i < LOG_COLUMN_COUNT - 1)
        logFile.print(',');
      else
        logFile.println();
    }
    logFile.close();
  }
}

void updateFileName() {
  int i = 0;
  for (; i < MAX_LOG_FILES; i++) {
    memset(logFileName, 0, strlen(logFileName));
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!SD.exists(logFileName)) {
      Serial.println("we picked a new file name");
      Serial.println(logFileName);
      M5.Lcd.println("we picked a new file name");
      M5.Lcd.println(logFileName);
      break;
    } else {
      Serial.print(logFileName);
      Serial.println(" exists");
    }
  }
  M5.Lcd.println("File name: ");
  M5.Lcd.println(logFileName);
}

void screenWipe() {
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
}
