// D1 = Pin-D3                   *
// D2 = Pin-D4, Pin-D14          *
// D3 = Pin-D8
// D4 = Pin-D9                   *
// D5 = Pin-D5, Pin-D13          *
// D6 = Pin-D6, Pin-D12          *
// D7 = Pin-D7, Pin-D11          *
// D8 = Pin-D10                  *

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <DHT.h>


struct struct_conf {
	String  ssid;
	String  pwd;
	String  deviceID;
	bool    mqtt;
	String  mqttIP;
	int     mqttPort;
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
	bool    api;
	bool    ssl;
	String  certPath;
};
struct struct_conf conf;

// D1 pin definition
int pin03=D1;
int pin04=D2;
int pin06=D6;
int pin07=D7;
int pin08=D3;
int pin09=D4;
int pin12=D6;
int pin13=D5;

#define SERIAL_BAUDRATE	19200
bool    	debug       = true;
boolean 	Motion 		= false;
boolean 	LastMotion 	= false;
boolean 	frontDoor 	= false;
boolean 	backDoor 	= false;
long duration;  // variable for the duration of sound wave travel
int  distance;  // variable for the distance measurement

int     	serverPort   = 80;
int     	HC_SR04_Trig = pin03;
int     	HC_SR04_Echo = pin04;
int     	reed2        = pin06;
int     	pirPin       = pin07;
int     	button       = pin08;
int     	reed1        = pin13;

const char* APssid   	= "Garage";
IPAddress 	ip;

IPAddress 	local_IP(192,168,4,1);
IPAddress 	gateway(192,168,4,255);
IPAddress 	subnet(255,255,255,0);
ESP8266WebServer server(serverPort); //Server on port

char    message_buff[100];
int     nb_subscribe_topics;
int     pubDelay = 90;
String  subscribe_topics[10];

bool    modeAccessPoint = false;
String  mqtt_status = "Not connected";
unsigned long lastMsg   = 0;    //Horodatage du dernier message publié sur MQTT

// DHT Definition
#define DHTPIN  pin09
#define DHTTYPE DHT22

WiFiClient    espClient;  // @suppress("Abstract class cannot be instantiated")
PubSubClient  client(espClient);
MDNSResponder mdns;
DHT dht(DHTPIN, DHTTYPE);


//===================================================================================
// Setup procedure
//===================================================================================
void setup() {
	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	delay(10);

	pinMode(pirPin,  INPUT);
	pinMode(DHTPIN,  INPUT);
	pinMode(reed1,   INPUT_PULLUP);
	pinMode(reed2,   INPUT_PULLUP);
	pinMode(button,  OUTPUT);
    pinMode(HC_SR04_Trig, OUTPUT); // Sets the trigPin as an OUTPUT
	pinMode(HC_SR04_Echo, INPUT);  // Sets the echoPin as an INPUT

    digitalWrite(button, LOW);
	Serial.println('\n');

	SPIFFS.begin();
	if (!SPIFFS.exists("/config.json")) {
		mdns.begin("ESP8266-Setup",local_IP);
		startAccessPoint();
	} else {
		readConfigFile();
		setup_wifi();
		if (conf.ssl)    {setup_ssl();   }
		if (conf.mqtt)   {setup_mqtt();  }
		if (conf.syslog) {setup_syslog();}
		if (conf.api)    {setup_api();   }
	}
}


//==================================================================================
// Read Config File
//==================================================================================
void readConfigFile() {
bool fsMounted = SPIFFS.begin();

 if (fsMounted) {Serial.println("File system mounted with success");}
 else {Serial.println("Error mounting the file system"); return; }

 File file = SPIFFS.open("/config.json", "r"); // @suppress("Abstract class cannot be instantiated")
 if (!file) { Serial.println("Error opening file"); }
 Serial.println("Reading config file");
 parseConfigJson(file.readStringUntil('\n'));
}



//==================================================================================
// Parse JSON read from FS
//==================================================================================
void parseConfigJson(String document)  {
const char* value;

	Serial.println("Parsing config file");

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
	value             = doc["staticIP"];
	conf.staticIP     = value;
	value             = doc["gateway"];
	conf.gateway      = value;
	value             = doc["subnet"];
	conf.subnet       = value;
	value             = doc["dns"];
	conf.dns          = value;
	value             = doc["mac"];
	conf.mac          = value;
	value             = doc["syslogServer"];
	conf.syslogServer = value;
	value             = doc["certPath"];
	conf.certPath     = value;
	conf.mqttPort     = atoi(doc["mqttPort"]);
	conf.syslogPort   = atoi(doc["syslogPort"]);
	conf.mqtt         = strstr( doc["mqtt"]  , "ON");
	conf.syslog 	  = strstr( doc["syslog"], "ON");
	conf.upnp   	  = strstr( doc["upnp"]  , "ON");
	conf.api    	  = strstr( doc["api"]   , "ON");
	conf.ssl    	  = strstr( doc["ssl"]   , "ON");
}



//===================================================================================
//     This routine is executed when you open its IP in browser
//===================================================================================
void handleRoot() {
String htmlPage = "";
bool fsMounted = SPIFFS.begin();

	if (fsMounted ) {
		Serial.println("File system mounted with success");
	} else {
		Serial.println("Error mounting the file system");
		return;
	}
	if (SPIFFS.exists("/index.html")) {
		File file = SPIFFS.open("/index.html", "r"); // @suppress("Abstract class cannot be instantiated")
		if (!file) { Serial.println("Error opening file"); }
		Serial.println("\nReading access point web page");

		  while(file.available()) {
		  	htmlPage = htmlPage + file.readStringUntil('\n');
		  }
	    server.send(200, "<! DOCTYPE html>", htmlPage.c_str());
	}
}



//===================================================================================
//     This routine is executed when you open its IP in browser
//===================================================================================
void handlePost() {
void(* resetFunc) (void) = 0; //declare reset function @ address 0
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
   doc["staticIP"] 		= server.arg("staticIP");
   doc["gateway"] 		= server.arg("gateway");
   doc["subnet"] 		= server.arg("subnet");
   doc["dns"] 			= server.arg("DNS");
   doc["mac"] 			= server.arg("MAC");
   doc["syslogServer"] 	= server.arg("syslogServer");
   doc["syslogPort"] 	= server.arg("syslogPort");
   doc["upnp"] 			= server.arg("UPnP");
   doc["syslog"] 		= server.arg("syslog");
   doc["api"] 			= server.arg("API");
   doc["ssl"] 			= server.arg("SSL");
   doc["CertPath"] 		= server.arg("CertPath");

   serializeJson(doc, jsonConfig);

   bool fsMounted = SPIFFS.begin();
   if (fsMounted ) {
    Serial.println("File system mounted with success");
   } else {
    Serial.println("Error mounting the file system");
    return;
   }

   // Save JSON configuration file
   File file = SPIFFS.open("/config.json", "w"); // open the file in write mode // @suppress("Abstract class cannot be instantiated")
   file.println(jsonConfig);
   file.close();

   Serial.println("\nconfig file saved");

   if (SPIFFS.exists("/success.html")) {
    File file = SPIFFS.open("/success.html", "r"); // @suppress("Abstract class cannot be instantiated")
    if (!file) { Serial.println("Error opening file /success.html"); }
    Serial.println("\nReading success page");
      while(file.available()) {
       htmlPage = htmlPage + file.readStringUntil('\n');
      }
       server.send(200, "<! DOCTYPE html>", htmlPage.c_str());
   }
   file.close();
   WiFi.softAPdisconnect (true);
   resetFunc();
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



//********************************************************************************
//Connexion au réseau WiFi
//********************************************************************************
void setup_wifi() {
const char* wifi_ssid = conf.ssid.c_str();
const char* wifi_password = conf.pwd.c_str();

  delay(10);
  Serial.println();
  Serial.print("Connecting to :");
  Serial.print(conf.ssid);
  Serial.print(" ");
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
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
// Configuration de la connexion SSL
//********************************************************************************
void setup_ssl() {
}


//********************************************************************************
// Configuration de la connexion MQTT
//********************************************************************************
void setup_mqtt() {
  client.setServer(conf.mqttIP.c_str(), conf.mqttPort); //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);                     //La fonction de callback qui est executée à chaque réception de message
  sub_Topics();
}


//********************************************************************************
// Initialise Subscribed topc
//********************************************************************************
void sub_Topics(){
	 nb_subscribe_topics = 4;
	 subscribe_topics[0] = String(conf.deviceID) + "/Config" ;
	 subscribe_topics[1] = String(conf.deviceID) + "/Reset"  ;
	 subscribe_topics[2] = String(conf.deviceID) + "/Restart";
	 subscribe_topics[3] = String(conf.deviceID) + "/FrontDoor";
}


//********************************************************************************
// Configuration de la connexion Syslog
//********************************************************************************
void setup_syslog() {

}



//********************************************************************************
// Configuration de la connexion API
//********************************************************************************
void setup_api() {
    Serial.print("Starting API service...");
	server.on("/"       , handleRootDevice); // @suppress("Ambiguous problem")
	server.on("/Config" , handleConfig);     // @suppress("Ambiguous problem")
	server.on("/Reset"  , handleReset);      // @suppress("Ambiguous problem")
	server.on("/Restart", handleRestart);    // @suppress("Ambiguous problem")

	server.begin(); //Start API server
	Serial.println("OK\n");
	MDNS.addService("http", "tcp", serverPort);
}



//********************************************************************************
//
//********************************************************************************
void handleRootDevice() {
    Serial.println("Root request");
    server.send(200, "<! DOCTYPE json>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Device " + conf.deviceID +" is listening</body></html>" );
}



//********************************************************************************
//
//********************************************************************************
void handleConfig() {
String jsonStatus;

	Serial.println("Config request");
	jsonStatus = CreateJsonStatus();
    server.send(200, "<! DOCTYPE json>", jsonStatus );
}


//********************************************************************************
//
//********************************************************************************
void handleReset() {
void(* resetFunc) (void) = 0; //declare reset function @ address 0

	Serial.println("Reset request");
    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Resetting Bedlight</body></html>" );
	SPIFFS.begin();
	SPIFFS.remove("/config.json");
	resetFunc();
}


//********************************************************************************
//
//********************************************************************************
void handleRestart() {
//void(* resetFunc) (void) = 0; //declare reset function @ address 0

	Serial.println("Restart request");
    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Restarting Bedlight</body></html>" );
    //resetFunc();
    ESP.restart();
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
//Connexion or Reconnexion
//********************************************************************************
void reconnect() { //Boucle jusqu'à obtenur une reconnexion

  while (!client.connected()) {
    Serial.print("MQTT Connecxion of " + String(conf.deviceID) + "...");

    if (client.connect(conf.deviceID.c_str())) {
      Serial.println("OK");
      mqtt_status = "Connected";
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



//==================================================================================
//  Main Loop
//==================================================================================
void loop() {
	if (modeAccessPoint or conf.api) {
		server.handleClient(); }

	if (!modeAccessPoint) {
		if (conf.mqtt){
			if (!client.connected()) { reconnect(); }
		    client.loop();
		}
		PublishMotionSensor();
		MonitorGarageDoor();

		unsigned long now = millis();
		if (now - lastMsg > (1000 * pubDelay)) {
			lastMsg = now;
			MonitorDistance();
			PublishGarageStatus();
		}
	}
}




//****************************************************************************
// Monitor Distance
//****************************************************************************
void MonitorDistance() {
	digitalWrite(HC_SR04_Trig, LOW);
	delayMicroseconds(2);

	digitalWrite(HC_SR04_Trig, HIGH);
	delayMicroseconds(10);
	digitalWrite(HC_SR04_Trig, LOW);

	duration = pulseIn(HC_SR04_Echo, HIGH);
	distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
}



//****************************************************************************
// Monitor Garage Door status
//****************************************************************************
void MonitorGarageDoor() {
boolean saveFrontDoor = frontDoor;
boolean saveBackDoor  = backDoor;

	frontDoor = digitalRead(reed1);
	backDoor  = digitalRead(reed2);

	if (frontDoor != saveFrontDoor or backDoor != saveBackDoor) {
		PublishGarageStatus();
		delay(500);
	}
}



//****************************************************************************
// Publish Motion Sensor
//****************************************************************************
void PublishMotionSensor() {
String jsonMotionStatus;
String topic =  String(conf.deviceID) + "/Motion";

  Motion = digitalRead(pirPin);
  if (Motion != LastMotion) {
    if (Motion == HIGH) {
		StaticJsonDocument<64> doc;
		doc["Motion"]     = "1";
		doc["MotionDesc"] = "Motion detected";
		serializeJson(doc, jsonMotionStatus);
		client.publish(topic.c_str(), jsonMotionStatus.c_str());
    } else {

		StaticJsonDocument<64> doc;
		doc["Motion"]     = "0";
		doc["MotionDesc"] = "No Motion detected";
		serializeJson(doc, jsonMotionStatus);
		client.publish(topic.c_str(), jsonMotionStatus.c_str());
    }
    LastMotion = Motion;
  }
}





//****************************************************************************
// Publishing Garage Status
//****************************************************************************
void PublishGarageStatus() {
String jsonLightStatus;
String topic       = conf.deviceID  + "/Sensors";

float  humidity    = dht.readHumidity();
delay(200);
float  temperature = dht.readTemperature();

//    Serial.println(humidity);
//    Serial.println(temperature);

	StaticJsonDocument<128> doc;

	doc["temperature"] = temperature;
	doc["humidity"]    = humidity;
	doc["frontDoor"]   = frontDoor;
	doc["backDoor"]    = backDoor;
	doc["distance"]    = distance;

	serializeJson(doc, jsonLightStatus);
	client.publish(topic.c_str(), jsonLightStatus.c_str());

	Serial.print("Publishing garage status to : ");
	Serial.print(topic.c_str());
	Serial.print("   ");
	Serial.println(jsonLightStatus.c_str());
}




//********************************************************************************
// Traiter message recu
//********************************************************************************
void TraiterMessageRecu(String topic, String payload) {

 if (debug) { Serial.println("Topic recu => " + topic + " Payload: " + payload); }
 if (topic == subscribe_topics[0]) { DisplayConfig(); }
 if (topic == subscribe_topics[1]) {
     SPIFFS.begin();
     SPIFFS.remove("/config.json");
     ESP.restart(); }
 if (topic == subscribe_topics[2]) { ESP.restart();     }
 if (topic == subscribe_topics[3]) { ToogleFrontDoor();     }
}




//********************************************************************************
// Toogle Front Door
// *******************************************************************************
void ToogleFrontDoor() {
	digitalWrite(button, HIGH);
	delay(2000);
	digitalWrite(button, LOW);
}



//********************************************************************************
// Display controler status
//********************************************************************************
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
// Read device state
//********************************************************************************
void ReadDeviceLastState() {
String htmlPage;

  Serial.println("\nconfig file saved");

  bool fsMounted = SPIFFS.begin();
  if (fsMounted) {
	 if (SPIFFS.exists("/success.html")) {
		 File file = SPIFFS.open("/success.html", "r"); // @suppress("Abstract class cannot be instantiated")
		 if (!file) { Serial.println("Error opening file /success.html"); }
		 Serial.println("\nReading success page");

		 while(file.available()) {
			 htmlPage = htmlPage + file.readStringUntil('\n');
		 }
		 server.send(200, "<! DOCTYPE html>", htmlPage.c_str());
	 }
  }
}

