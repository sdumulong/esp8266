#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2020-06-17 11:30:56

#include "Arduino.h"
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>

void setup() ;
void readConfigFile() ;
void parseConfigJson(String document)  ;
void handleRoot() ;
void handlePost() ;
void startAccessPoint() ;
void setup_wifi() ;
void setup_ssl() ;
void setup_mqtt() ;
void sub_Topics();
void setup_syslog() ;
void setup_api() ;
void handleRootDevice() ;
void handleConfig() ;
void handleReset() ;
void handleState() ;
void handleColor() ;
void handleRestart() ;
void callback(char* topic, byte* payload, unsigned int length) ;
void reconnect() ;
void loop() ;
void PublishMotionSensor() ;
void monitorResetSwitch() ;
void PublishLightStatus() ;
void TraiterMessageRecu(String topic, String payload) ;
void DisplayConfig() ;
String CreateJsonStatus() ;
void SetState(String payload) ;
void SetColor(String payload) ;
String ReadSavedColor() ;
void SaveColorValue(String color) ;
void colorRGB(String HexValue);
void ReadDeviceLastState() ;
void SetRainbowColor() ;
void SetChristmasColor() ;

#include "Bedlight.ino"


#endif
