#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>

struct struct_config {
	const char* ssid;
	const char* pwd;
	const char* deviceID;
	bool mqtt;
	const char* mqttIP;
	int mqttPort;
	const char* mqttUser;
	const char* mqttPwd;
	const char* IPmode;
	const char* staticIP;
	const char* gateway;
	const char* subnet;
	const char* dns;
	const char* mac;
	bool syslog;
	const char* syslogServer;
	int syslogPort;
	bool upnp;
	bool api;
	bool ssl;
	const char* certPath;
};

#define SERIAL_BAUDRATE	19200
bool    	debug        = true; //Affiche sur la console si True
struct struct_config config;
int     	serverPort   = 80;
int     	pirPin       = D3;
int     	microSwitch  = D8;

boolean 	Motion 		= 0;
boolean 	LastMotion 	= 0;
const char* APssid   	= "Motion";
IPAddress 	ip;

IPAddress 	local_IP(192,168,4,3);
IPAddress 	gateway(192,168,4,255);
IPAddress 	subnet(255,255,255,0);
ESP8266WebServer server(serverPort); //Server on port

char    message_buff[100];
int     nb_subscribe_topics;
String  subscribe_topics[10];
bool    modeAccessPoint = false;
String  mqtt_status = "Not connected";
unsigned long lastMsg   = 0;    //Horodatage du dernier message publié sur MQTT

WiFiClient    espClient;  // @suppress("Abstract class cannot be instantiated")
PubSubClient  client(espClient);
MDNSResponder mdns;



//===================================================================================
// Setup procedure
//===================================================================================
void setup() {
	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	delay(10);

	pinMode(pirPin, INPUT);
	Serial.println('\n');

	SPIFFS.begin();
	if (!SPIFFS.exists("/config.json")) {
		mdns.begin("ESP8266-Setup",local_IP);
		startAccessPoint();
	} else {
		readConfigFile();
		setup_wifi();
		if (config.mqtt)   {setup_mqtt();  }
		if (config.api)    {setup_api();   }
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
	Serial.println("Parsing config file");

	StaticJsonDocument<1152> doc;
	deserializeJson(doc, document);
	config.ssid         = doc["ssid"];
	config.pwd          = doc["pwd"];
	config.deviceID     = doc["deviceID"];
	config.mqttIP       = doc["mqttIP"];
	config.mqttUser     = doc["mqttUser"];
	config.mqttPwd      = doc["mqttPwd"];
	config.IPmode       = doc["IPmode"];
	config.staticIP     = doc["staticIP"];
	config.gateway      = doc["gateway"];
	config.subnet       = doc["subnet"];
	config.dns          = doc["dns"];
	config.mac          = doc["mac"];
	config.syslogServer = doc["syslogServer"];
	config.certPath     = doc["certPath"];
	config.mqttPort     = atoi(doc["mqttPort"]);
	config.syslogPort   = atoi(doc["syslogPort"]);
	config.mqtt         = strstr( doc["mqtt"]  , "ON");
	config.syslog 		= strstr( doc["syslog"], "ON");
	config.upnp   		= strstr( doc["upnp"]  , "ON");
	config.api    		= strstr( doc["api"]   , "ON");
	config.ssl    		= strstr( doc["ssl"]   , "ON");

	Serial.println(	config.certPath);
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
   ESP.restart();
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

	  WiFi.mode(WIFI_AP_STA);         //Both Access point and Wifi client are enabled
	  WiFi.softAP(APssid);  //Start HOTspot removing password will disable security

	  server.on("/", handleRoot);      //Which routine to handle at root location
	  server.on("/save", handlePost);
	  server.begin();                  //Start server
	  Serial.println("HTTP server started");
}



//********************************************************************************
//Connexion au réseau WiFi
//********************************************************************************
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to :");
  Serial.print(config.ssid);
  Serial.print(" ");
  WiFi.begin(config.ssid, config.pwd);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(" ");
  Serial.println("\nConnexion WiFi etablie ");
  Serial.print("Addresse IP : ");
  Serial.println(WiFi.localIP());
  ip = WiFi.localIP();
  if(mdns.begin(config.deviceID),WiFi.localIP())
	  Serial.println("\nmDNS Responder Started on http://" + String(config.deviceID));
}



//********************************************************************************
// Configuration de la connexion MQTT
//********************************************************************************
void setup_mqtt() {
  client.setServer(config.mqttIP, config.mqttPort); //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);                     //La fonction de callback qui est executée à chaque réception de message
  sub_Topics();
}


//********************************************************************************
// Initialise Subscribed topc
//********************************************************************************
void sub_Topics(){
	 nb_subscribe_topics = 3;
	 subscribe_topics[0] = String(config.deviceID) + "/Config" ;
	 subscribe_topics[1] = String(config.deviceID) + "/Reset"  ;
	 subscribe_topics[2] = String(config.deviceID) + "/Restart";
}



//********************************************************************************
// Configuration de la connexion API
//********************************************************************************
void setup_api() {
    Serial.print("Starting API service...");
	server.on("/"       , handleRootDevice);
	server.on("/Config" , handleConfig);
	server.on("/Reset"  , handleReset);
	server.on("/Restart", handleRestart);

	server.begin(); //Start API server
	Serial.println("OK\n");
	MDNS.addService("http", "tcp", serverPort);
}



//********************************************************************************
//
//********************************************************************************
void handleRootDevice() {
    Serial.println("Root request");
    server.send(200, "<! DOCTYPE json>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Device " + String(config.deviceID) +" is listening</body></html>" );
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

	Serial.println("Reset request");
    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Resetting Bedlight</body></html>" );
	SPIFFS.begin();
	SPIFFS.remove("/config.json");
	ESP.restart();
}



//********************************************************************************
//
//********************************************************************************
void handleRestart() {
	Serial.println("Restart request");
    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Restarting Bedlight</body></html>" );
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
	Serial.print("MQTT Connecxion of " + String(config.deviceID) + "...");

    if (client.connect(config.deviceID)) {
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
	if (modeAccessPoint or config.api) {
		server.handleClient(); }

	if (!modeAccessPoint) {
		if (config.mqtt){
			if (!client.connected()) { reconnect(); }
		    client.loop();
		}
		monitorResetSwitch();
		monitorMotion();
		unsigned long now = millis();
		if (now - lastMsg > (1000 * 30)) {
			lastMsg = now;
		}
	}
}




//****************************************************************************
// Reset switch control
//****************************************************************************
void monitorResetSwitch() {
int buttonStatus = 0;
int buttonPressed = 0;

buttonStatus = digitalRead(microSwitch);

delay(100);
	while (buttonStatus) {
		buttonPressed ++;
		delay(100);
		buttonStatus = digitalRead(microSwitch);
	}
    if (buttonPressed > 0 && buttonPressed <= 20) {
	    Serial.println("Button Pressed less than 20");
	    ESP.restart();
    }
    if (buttonPressed > 20) {
	    Serial.println("Button Pressed more than 20");
    	SPIFFS.begin();
  	    SPIFFS.remove("/config.json");
  	    ESP.restart();
    }
}




//****************************************************************************
// Reset switch control
//****************************************************************************
void monitorMotion() {

	String topic =  String(config.deviceID) + "/Motion";
	Motion = digitalRead(pirPin);

	if (Motion != LastMotion) {
		if (Motion == HIGH) {
			client.publish(topic.c_str(), "1");
	      if (debug) { Serial.println("Publishing Motion detected");}
	    } else {
	    	client.publish(topic.c_str(), "0");
	      if (debug) { Serial.println("Publishing No motion detected"); }
	    }
	    LastMotion = Motion;
	  }
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
 if (topic == subscribe_topics[2]) { ESP.restart();      }
}




//********************************************************************************
// Display controler status
//********************************************************************************
void DisplayConfig() {
String jsonStatus;
	jsonStatus = CreateJsonStatus();
	Serial.println(config.deviceID);
	Serial.println(jsonStatus);
	client.publish(config.deviceID,jsonStatus.c_str());
}




//********************************************************************************
// Create JSON Status
//********************************************************************************
String CreateJsonStatus() {
String jsonStatus;
 	 StaticJsonDocument<2048> doc;
 	 doc["SSID"]             = config.ssid;
 	 doc["Wifi Connection"]  = "Established";
 	 doc["Local IP"]         = ip.toString();
 	 doc["AccessPoint SSID"] = APssid;
 	 doc["AccessPoint IP"]   = local_IP.toString();
 	 doc["Device ID"]        = config.deviceID;
 	 doc["mqtt"]             = config.mqtt; ;
 	 doc["mqtt IP"]          = config.mqttIP;
 	 doc["mqtt Port"]        = config.mqttPort;
 	 doc["mqtt User"]        = config.mqttUser;
 	 doc["mqtt Password"]    = "***************";
 	 doc["mqtt status"]      = mqtt_status;
 	 doc["IP Mode"]          = config.IPmode;
 	 doc["Static IP"]        = config.staticIP;
 	 doc["gateway"]          = config.gateway;
 	 doc["subnet"]           = config.subnet;
 	 doc["dns"]              = config.dns;
 	 doc["mac"]              = config.mac;
 	 doc["syslog"]           = config.syslog;
 	 doc["syslogServer"]     = config.syslogServer;
 	 doc["syslogPort"]       = config.syslogPort;
 	 doc["certPath"]         = config.certPath;
 	 doc["upnp"]             = config.upnp;
 	 doc["api"]              = config.api;
 	 doc["ssl"]              = config.ssl;
 	 doc["debug"]            = debug;
 	 doc["pool delay"]       = 30;

 	 serializeJson(doc, jsonStatus);
 	 return jsonStatus;
}
