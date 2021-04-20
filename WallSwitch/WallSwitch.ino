#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

struct struct_conf {
	String  ssid = "";
	String  pwd = "";
	String  deviceID = "";
	bool    mqtt = true;
	String  mqttIP = "";
	int     mqttPort = 0;
	String  mqttUser;
	String  mqttPwd;
	String  IPmode;
	String  staticIP;
	String  gateway;
	String  subnet;
	String  dns;
	String  mac;
	bool    syslog;
	String  syslogServer;
	int     syslogPort;
	bool    upnp;
	bool    api = false;
	bool    ssl;
	String  certPath;
};
struct struct_conf conf;

#define SERIAL_BAUDRATE	19200
bool    	debug        = true; //Affiche sur la console si True

int     	serverPort   = 80;
int     	Button       = D3;
int         Relay        = D6;
int     	Led          = D7;

const char* APssid   	= "WallSwitch";
IPAddress 	ip;

IPAddress 	local_IP(192,168,4,1);
IPAddress 	gateway(192,168,4,255);
IPAddress 	subnet(255,255,255,0);
ESP8266WebServer server(serverPort); //Server on port

char    message_buff[100];
int     nb_subscribe_topics;
int     pubDelay = 90;
String  subscribe_topics[10];
bool    lightState;
bool    ledFlash  = false;
bool    ledStatus = true;

bool    modeAccessPoint = false;
String  mqtt_status = "Not connected";
unsigned long lastMsg   = 0;   //Horodatage du dernier message publié sur MQTT
unsigned long lastMsg2  = 0;   //Horodatage du dernier message publié sur MQTT

WiFiClient    espClient;  // @suppress("Abstract class cannot be instantiated")
PubSubClient  client(espClient);
MDNSResponder mdns;


void setup() {
int address = 0;
byte configlEN;

	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	delay(10);

	pinMode(Button,   INPUT);
	pinMode(Led,      OUTPUT);
	pinMode(Relay,    OUTPUT);

	digitalWrite(Led, HIGH);
	Serial.println('\n');

	EEPROM.begin(512);
	int newStrLen = EEPROM.read(0);

	if (newStrLen == 0) {
		mdns.begin("ESP8266-Setup",local_IP);
		startAccessPoint();
	} else {
		readConfigFile(newStrLen);
		setup_wifi();
		if (conf.mqtt)   {setup_mqtt();  }
	}
}



//==================================================================================
// Read Config File
//==================================================================================
void readConfigFile(int strLen) {
int eeAddress = 1;
char data[strLen + 1];

	for (int i = 0; i < strLen; i++) { data[i] = EEPROM.read(eeAddress + i); }
	data[strLen] = '\ 0';

	parseConfigJson(data);
}





//==================================================================================
// Parse JSON read from FS
//==================================================================================
void parseConfigJson(String document)  {
const char* value;

	Serial.println("Parsing config file");
	Serial.println(document);

	StaticJsonDocument<1152> doc;
	deserializeJson(doc, document);

	value             = doc["ssid"];
	conf.ssid         = value;
	value             = doc["pwd"];
	conf.pwd          = value;
	value             = doc["deviceID"];
	conf.deviceID     = value;
	value             = doc["mqttIP"];
	conf.mqttIP       = value;
	value             = doc["mqttUser"];
	conf.mqttUser     = value;
	value             = doc["mqttPwd"];
	conf.mqttPwd      = value;
	value             = doc["IPmode"];
	conf.IPmode       = value;
    //value             = doc["staticIP"];
	//conf.staticIP     = value;
	//value             = doc["gateway"];
	//conf.gateway      = value;
	//value             = doc["subnet"];
	//conf.subnet       = value;
	//value             = doc["dns"];
	//conf.dns          = value;
	//value             = doc["mac"];
	//conf.mac          = value;
	//value             = doc["syslogServer"];
	//conf.syslogServer = value;
	//value             = doc["certPath"];
	//conf.certPath     = value;
	conf.mqttPort     = atoi(doc["mqttPort"]);
	//conf.syslogPort = atoi(doc["syslogPort"]);
	conf.mqtt         = strstr( doc["mqtt"]  , "ON");
	//conf.syslog 	  = strstr( doc["syslog"], "ON");
	//conf.upnp   	  = strstr( doc["upnp"]  , "ON");
	conf.api    	  = strstr( doc["api"]   , "ON");
	//conf.ssl    	  = strstr( doc["ssl"]   , "ON");
}



//==================================================================================
//     Start Access point
//==================================================================================
void startAccessPoint() {
	  modeAccessPoint = true;
	  Serial.print("Setting soft-AP configuration ... ");
	  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

	  Serial.print("Setting soft-AP ... ");
	  Serial.println(WiFi.softAP("ESPsoftAP_01") ? "Ready" : "Failed!");

	  Serial.print("Soft-AP IP address = ");
	  Serial.println(WiFi.softAPIP());

	  WiFi.mode(WIFI_AP_STA);          //Both Access point and Wifi client are enabled
	  WiFi.softAP(APssid);             //Start HOTspot removing password will disable security

	  server.on("/", handleRoot);      // @suppress("Ambiguous problem")
	  server.on("/save", handlePost);  // @suppress("Ambiguous problem")
	  server.begin();                  //Start server
	  Serial.println("HTTP server started");
}




//===================================================================================
//     This routine is executed when you open its IP in browser
//===================================================================================
void handleRoot() {
	String strhtml = InitHtmlPage("Setup");
    server.send(200, "<! DOCTYPE html>", strhtml.c_str());
}



//===================================================================================
//     This routine is executed when you open its IP in browser
//===================================================================================
void handlePost() {
String jsonConfig;
String htmlPage = "";

   Serial.println("Reception of Data");

   StaticJsonDocument<512> doc;
   doc["ssid"] 			= server.arg("SSID");
   doc["pwd"] 			= server.arg("pwd");
   doc["deviceID"] 		= server.arg("mqttID");
   doc["IPmode"] 		= server.arg("DeviceIP");
   doc["mqtt"] 			= server.arg("MQTT");
   doc["mqttIP"] 		= server.arg("mqttIP");
   doc["mqttPort"] 		= server.arg("mqttPort");
   doc["mqttUser"] 		= server.arg("mqttUser");
   doc["mqttPwd"]		= server.arg("mqttPwd");
   doc["api"] 			= server.arg("API");

   serializeJson(doc, jsonConfig);

   // Save JSON configuration file
   int addr_JSON = 1;
   EEPROM.write(0, jsonConfig.length());	       // Save length of config JSON in first position
   for (int i = 0; i < jsonConfig.length(); ++i) { EEPROM.write(addr_JSON + i, jsonConfig[i]); } // Save JSON to EEPROM
   if (EEPROM.commit()) {
     Serial.println("Data successfully committed tp EEPROM");
   } else {
     Serial.println("ERROR! Data commit failed");
   }

   String strhtml = InitHtmlPage("Success");
   server.send(200, "<! DOCTYPE html>", strhtml.c_str());
   WiFi.softAPdisconnect (true);
   ESP.restart();
}



// The loop function is called in an endless loop
void loop() {

	if (modeAccessPoint or conf.api) {
		server.handleClient(); }

	if (!modeAccessPoint) {
		if (conf.mqtt){
			if (!client.connected()) { reconnect(); }
		    client.loop();
		}
		monitorSwitch();
		unsigned long now = millis();
		if (now - lastMsg > (1000 * pubDelay)) {
			lastMsg = now;
			PublishLightStatus();
		}
		if (ledFlash) {
			if (now - lastMsg2 > (1 * pubDelay)) {
				lastMsg2 = now;
				digitalWrite(Led, !digitalRead(Led));
			}
		}
	}

}




//****************************************************************************
// Publishing Light Status
//****************************************************************************
void monitorSwitch() {

	if (!digitalRead(Button)) {
		  int i = 0;
		  while (!digitalRead(Button))  {
			  i++;
		      delay(10);
		      if (i > 800) { digitalWrite(Led, HIGH); }
		  }
		  if (i < 800) {
		      digitalWrite(Relay, !digitalRead(Relay));
		      PublishLightStatus();
		  } else {
			  Serial.println("\nReset");
			  EEPROM.begin(512);
			  for (int i = 0; i < 512; i++) { EEPROM.write(i, 0); }
			  EEPROM.commit();
			  ESP.restart();;
		  }
	}
}




//********************************************************************************
//Connexion au réseau WiFi
//********************************************************************************
void setup_wifi() {
const char* wifi_ssid = conf.ssid.c_str();
const char* wifi_password = conf.pwd.c_str();

  WiFi.disconnect();
  delay(10);
  Serial.println();
  Serial.print("Connecting to :");
  Serial.print(conf.ssid);
  Serial.print(" ");
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
	monitorSwitch();  // In case of network failure we must be able to reset device
    delay(500);
    Serial.print(".");
  }
  Serial.print(" ");
  Serial.println("\nConnexion WiFi etablie ");
  Serial.print("Addresse IP : ");
  Serial.println(WiFi.localIP());
  ip = WiFi.localIP();
  if(mdns.begin(conf.deviceID.c_str()),WiFi.localIP())
  	  Serial.println("\nmDNS Responder Started on http://" + String(conf.deviceID));
}




//********************************************************************************
// Configuration de la connexion MQTT
//********************************************************************************
void setup_mqtt() {
  client.setServer(conf.mqttIP.c_str(), conf.mqttPort); //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);                         //La fonction de callback qui est executée à chaque réception de message
  sub_Topics();
}




//********************************************************************************
//Connexion or Reconnexion
//********************************************************************************
void reconnect() { //Boucle jusqu'à obtenur une reconnexion

  digitalWrite(Led, HIGH);
  while (!client.connected()) {
    Serial.print("MQTT Connecxion of " + String(conf.deviceID) + "...");

    if (client.connect(conf.deviceID.c_str())) {
      Serial.println("OK");
      mqtt_status = "Connected";
  	  digitalWrite(Led, LOW);
      for (int i=0; i<nb_subscribe_topics; i++) {
        client.subscribe(subscribe_topics[i].c_str());
        if (debug) { Serial.println("Subscribing to topic: " + subscribe_topics[i]); }
      }
      Serial.println("");
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println(" Wait 5 sec. and retry");
      delay(5000);
    }
  }
}



//********************************************************************************
// Initialise Subscribed topc
//********************************************************************************
void sub_Topics(){
	 nb_subscribe_topics = 5;
	 subscribe_topics[0] = String(conf.deviceID) + "/Config" ;
	 subscribe_topics[1] = String(conf.deviceID) + "/Reset"  ;
	 subscribe_topics[2] = String(conf.deviceID) + "/State"  ;
	 subscribe_topics[3] = String(conf.deviceID) + "/Restart";
	 subscribe_topics[4] = String(conf.deviceID) + "/Led";
}




//********************************************************************************
// Check for incomming messages
//********************************************************************************
void callback(char* topic, byte* payload, unsigned int length) {
std::size_t i;
  for(i=0; i<length; i++) { message_buff[i] = payload[i]; }
  message_buff[i] = '\0';
  String msgString = String(message_buff);

  TraiterMessageRecu(topic, msgString);
}




//********************************************************************************
// Traiter message recu
//********************************************************************************
void TraiterMessageRecu(String topic, String payload) {

 if (debug) { Serial.println("Topic recu => " + topic + " Payload: " + payload); }
 if (topic == subscribe_topics[0]) {  DisplayConfig(); }
 if (topic == subscribe_topics[1]) {
	EEPROM.begin(512);
    for (int i = 0; i < 512; i++) { EEPROM.write(i, 0); }
    EEPROM.commit();
    ESP.restart(); }
 if (topic == subscribe_topics[2]) { SetState(payload);}
 if (topic == subscribe_topics[3]) { ESP.restart();     }
 if (topic == subscribe_topics[4]) { LedState(payload); }
}




//********************************************************************************
// Set State of Peripheral (On/Off)
//********************************************************************************
void LedState(String payload) {

  if (payload == "On") {
      digitalWrite(Led, LOW);
      ledFlash = false;
	  PublishLightStatus();
      return;
  }
  if (payload == "Off") {
      ledFlash = false;
	  digitalWrite(Led, HIGH);
      PublishLightStatus();
     return;
  }
  if (payload == "Enabled") {
	  ledStatus = true;
      PublishLightStatus();
     return;
  }
  if (payload == "Disabled") {
	  ledStatus = false;
      ledFlash = false;
      PublishLightStatus();
     return;
  }
  if (payload == "Blink") {
      ledFlash = true;
      PublishLightStatus();
     return;
  }
}




//********************************************************************************
// Set State of Peripheral (On/Off)
//********************************************************************************
void SetState(String payload) {

  if (payload == "On" or payload == "1") {
      digitalWrite(Relay, HIGH);
      lightState = true;
	  PublishLightStatus();
      return;
  }
  if (payload == "Off" or payload == "0") {
	  digitalWrite(Relay, LOW);
      lightState = false;
      PublishLightStatus();
     return;
  }
}






//****************************************************************************
// Publishing Light Status
//****************************************************************************
void PublishLightStatus() {
String jsonLightStatus;
String topic = conf.deviceID  + "/Light";
String dspLedStatus = "N/A";

    if (ledFlash) { dspLedStatus = "Blink"; }
    else { 	if (ledStatus) {
        	dspLedStatus = "Enabled";
     		} else { dspLedStatus = "Disabled";	}
    }

	StaticJsonDocument<128> doc;
	if (digitalRead(Relay) == false) {
		doc["Status"]    = "Off";
		doc["State"]     = 0;
		doc["Led"]       = int(!digitalRead(Led));
		doc["LedStatus"] = dspLedStatus;
	} else {
		doc["Status"]    = "On";
		doc["State"]     = 1;
		doc["Led"]       = int(!digitalRead(Led));
		doc["LedStatus"] = dspLedStatus;
	}
	serializeJson(doc, jsonLightStatus);
	client.publish(topic.c_str(), jsonLightStatus.c_str());

	Serial.print("Publishing light status to : ");
	Serial.print(topic.c_str());
	Serial.print("   ");
	Serial.println(jsonLightStatus.c_str());
}



//********************************************************************************
// Display controler status
//************************************************	********************************
void DisplayConfig() {
String jsonStatus;
	jsonStatus = CreateJsonStatus();
	Serial.println(jsonStatus);
	client.publish(conf.deviceID.c_str(),jsonStatus.c_str());
}




//********************************************************************************
// Create JSON Status
//********************************************************************************
String CreateJsonStatus() {
String jsonStatus;
 	 StaticJsonDocument<2048> doc;
 	 doc["SSID"]             = conf.ssid;
 	 doc["Wifi Connection"]  = "Established";
 	 doc["Local IP"]         = ip.toString();
 	 doc["AccessPoint SSID"] = APssid;
 	 doc["AccessPoint IP"]   = local_IP.toString();
 	 doc["Device ID"]        = conf.deviceID;
 	 doc["mqtt"]             = conf.mqtt; ;
 	 doc["mqtt IP"]          = conf.mqttIP;
 	 doc["mqtt Port"]        = conf.mqttPort;
 	 doc["mqtt User"]        = conf.mqttUser;
 	 doc["mqtt Password"]    = "***************";
 	 doc["mqtt status"]      = mqtt_status;
 	 doc["IP Mode"]          = conf.IPmode;
 	 doc["Static IP"]        = conf.staticIP;
 	 doc["gateway"]          = conf.gateway;
 	 doc["subnet"]           = conf.subnet;
 	 doc["dns"]              = conf.dns;
 	 doc["mac"]              = conf.mac;
 	 doc["syslog"]           = conf.syslog;
 	 doc["syslogServer"]     = conf.syslogServer;
 	 doc["syslogPort"]       = conf.syslogPort;
 	 doc["certPath"]         = conf.certPath;
 	 doc["upnp"]             = conf.upnp;
 	 doc["api"]              = conf.api;
 	 doc["ssl"]              = conf.ssl;
 	 doc["debug"]            = debug;
 	 doc["pool delay"]       = pubDelay;

 	 serializeJson(doc, jsonStatus);
 	 return jsonStatus;
}



//********************************************************************************
// Display controler status
//************************************************	********************************
String InitHtmlPage(String mode) {
String strhtml = "";

	if (mode == "Setup") {
	   strhtml +="<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head>" ;
	   strhtml += "<body><form action=\"save\" method=\"post\">";
	   strhtml += "<table border=\"0\" width=\"400\" cellspacing=\"0\" cellpadding=\"0\">";
	   strhtml += "<tr><td colspan=\"2\" bgcolor=\"#000000\"><p align=\"center\"><font color=\"#FFFFFF\"><b><font size=\"5\">SDLogik</font><br>Network Setup</b></font></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;</td><td width=\"188\">&nbsp;</td></tr>";
	   strhtml += "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>SSID </td><td width=\"188\"><input type=\"text\" name=\"SSID\" id=\"SSID\" required size=\"20\"></td></tr>";
	   strhtml += "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>PWD </td><td width=\"188\"><input type=\"password\" name=\"pwd\" id=\"pwd\" required></td></tr>";
	   strhtml += "<tr><td width=\"148\"><font color=\"#FF0000\">&nbsp;&nbsp;*</font>Device ID</td><td width=\"188\"><input type=\"text\" name=\"mqttID\" id=\"mqttID\" required></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Device IP</td><td width=\"188\"><select size=\"1\" name=\"DeviceIP\"><option selected>DHCP</option><option>Static</option></select></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Enabled</td><td width=\"188\"><input type=\"checkbox\" name=\"MQTT\" value=\"ON\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT IP</td><td width=\"188\"><input type=\"text\" name=\"mqttIP\" id=\"mqttIP\" size=\"12\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Port </td><td width=\"188\"><input type=\"text\" name=\"mqttPort\" id=\"mqttPort\" size=\"6\" placeholder=\"1883\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT User</td><td width=\"188\"><input type=\"text\" name=\"mqttUser\" id=\"mqttUser\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MQTT Pwd</td><td width=\"188\"><input type=\"password\" name=\"mqttPwd\" id=\"mqttPwd\" size=\"20\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Static IP</td><td width=\"188\"><input type=\"text\" name=\"staticIP\" id=\"staticIP\" size=\"12\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Gateway</td><td width=\"188\"><input type=\"text\" name=\"gateway\" id=\"gateway\" size=\"12\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Subnet</td><td width=\"188\"><input type=\"text\" name=\"subnet\" id=\"subnet\" size=\"12\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;DNS</td><td width=\"188\"><input type=\"text\" name=\"DNS\" id=\"DNS\" size=\"12\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;MAC</td><td width=\"188\"><input type=\"text\" name=\"MAC\" id=\"MAC\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Enable UPnP</td><td width=\"188\"><input type=\"checkbox\" name=\"UPnP\" value=\"ON\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Enabled</td><td width=\"188\"><input type=\"checkbox\" name=\"syslog\" value=\"ON\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Server</td><td width=\"188\"><input type=\"text\" name=\"syslogServer\" id=\"syslogServer\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Syslog Port</td><td width=\"188\"><input type=\"text\" name=\"syslogPort\" id=\"syslogPort\" size=\"6\" placeholder=\"514\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Enable API</td><td width=\"188\"><input type=\"checkbox\" name=\"API\" value=\"ON\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Enable SSL</td><td width=\"188\"><input type=\"checkbox\" name=\"SSL\" value=\"ON\"></td></tr>";
	   strhtml += "<tr><td width=\"148\">&nbsp;&nbsp;Certificate Path</td><td width=\"188\"><input type=\"text\" name=\"CertPath\" size=\"25\"></td></tr>";
	   strhtml += "<tr><td width=\"336\" colspan=\"2\"><p align=\"left\">&nbsp;</td></tr>";
	   strhtml += "<tr><td width=\"336\" colspan=\"2\"><p align=\"center\"><input type=\"submit\" name=\"save\" value=\"Save Config\">&nbsp;&nbsp;&nbsp;";
	   strhtml += "<input type=\"submit\" name=\"reset\" value=\"Reset Form\"></td></tr>";
	   strhtml += "</table><p align=\"left\"><font color=\"#FF0000\">*</font> Required</p></form></body></html>";
	}

	if (mode == "Success") {
	   strhtml="<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head>" ;
	   strhtml = strhtml + "<body><table border=\"0\" width=\"400\" cellspacing=\"0\" cellpadding=\"0\">";
	   strhtml = strhtml + "<tr><td colspan=\"2\" bgcolor=\"#000000\"><p align=\"center\"><font color=\"#FFFFFF\"><b><font size=\"5\">SDLogik</font><br>Network Setup</b></font></td></tr>";
	   strhtml = strhtml + "<tr><td width=\"148\">&nbsp;</td><td width=\"188\">&nbsp;</td></tr>";
	   strhtml = strhtml + "</table><p align=\"left\"><font color=\"#00FF00\"><br>Configuration has been successfully saved</font></p></form></body></html>";
	}

	return strhtml;
}

