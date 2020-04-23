#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
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
int         pinRed       = D1;
int         pinGreen     = D5;
int         pinBlue      = D3;
int         pinWhite     = D4;
int     	microSwitch  = D8;

const char* APssid   	= "FakeWindow";
IPAddress 	ip;

IPAddress 	local_IP(192,168,4,3);
IPAddress 	gateway(192,168,4,255);
IPAddress 	subnet(255,255,255,0);
ESP8266WebServer server(serverPort); //Server on port

char    message_buff[100];
int     nb_subscribe_topics;
String  subscribe_topics[10];
String  FriendlyName= "";
bool    modeAccessPoint = false;
String  mqtt_status = "Not connected";
unsigned long lastMsg   = 0;    //Horodatage du dernier message publié sur MQTT
String  activeColor = "#00000000";  //Light Value

WiFiClient    espClient;  // @suppress("Abstract class cannot be instantiated")
PubSubClient  client(espClient);
MDNSResponder mdns;


//===================================================================================
// Setup procedure
//===================================================================================
void setup() {
	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	delay(10);

	pinMode(pinRed,   OUTPUT);
	pinMode(pinGreen, OUTPUT);
	pinMode(pinBlue,  OUTPUT);
	pinMode(pinWhite, OUTPUT);

	analogWrite(pinRed,  0);
	analogWrite(pinGreen,0);
	analogWrite(pinBlue, 0);
	analogWrite(pinWhite,0);

	pinMode(microSwitch, INPUT);
	Serial.println('\n');

	SPIFFS.begin();
	if (!SPIFFS.exists("/config.json")) {
		mdns.begin("ESP8266-Setup",local_IP);
		startAccessPoint();
	} else {
		readConfigFile();
		setup_wifi();
		if (config.ssl)    {setup_ssl();   }
		if (config.mqtt)   {setup_mqtt();  }
		if (config.syslog) {setup_syslog();}
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
//    Serial.print(document);
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

	FriendlyName = String(config.deviceID);
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
// Configuration de la connexion SSL
//********************************************************************************
void setup_ssl() {
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
	 nb_subscribe_topics = 5;
	 subscribe_topics[0] = String(config.deviceID) + "/Config" ;
	 subscribe_topics[1] = String(config.deviceID) + "/Reset"  ;
	 subscribe_topics[2] = String(config.deviceID) + "/State"  ;
	 subscribe_topics[3] = String(config.deviceID) + "/Color"  ;
	 subscribe_topics[4] = String(config.deviceID) + "/Restart";
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
	server.on("/Config" , handleConfig); // @suppress("Ambiguous problem")
	server.on("/Reset"  , handleReset); // @suppress("Ambiguous problem")
	server.on("/State"  , handleState); // @suppress("Ambiguous problem")
	server.on("/Color"  , handleColor); // @suppress("Ambiguous problem")
	server.on("/Restart", handleRestart); // @suppress("Ambiguous problem")

	server.begin(); //Start API server
	Serial.println("OK\n");
	MDNS.addService("http", "tcp", serverPort);
}



//********************************************************************************
//
//********************************************************************************
void handleRootDevice() {
    Serial.println("Root request");
    server.send(200, "<! DOCTYPE json>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Device " + FriendlyName +" is listening</body></html>" );
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
void handleState() {
String value = "";

//	Serial.println("State request");
//	value = server.arg("value");
//    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>State changed for " + value + "</body></html>" );
}


//********************************************************************************
//
//********************************************************************************
void handleColor() {
String value = "";
//	Serial.println("Color request");
//	value = server.arg("value");
//    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Color set to " + value + "</body></html>" );
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
		unsigned long now = millis();
		if (now - lastMsg > (1000 * 30)) {
			lastMsg = now;
			PublishLightStatus();
		}
	}
}




//****************************************************************************
// Reset switch control
//****************************************************************************
void monitorResetSwitch() {
//void(* resetFunc) (void) = 0; //declare reset function @ address 0
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
    	//resetFunc();
    }
    if (buttonPressed > 20) {
	    Serial.println("Button Pressed more than 20");
    	SPIFFS.begin();
  	    SPIFFS.remove("/config.json");
    	//resetFunc();
  	    ESP.restart();
    }
}



//****************************************************************************
// Publishing Light Status
//****************************************************************************
void PublishLightStatus() {
String jsonLightStatus;
String topic = FriendlyName  + "/Light";

	StaticJsonDocument<64> doc;
	if (activeColor == "#00000000") {
		doc["Status"] = "Off";
		doc["State"]  = 0;
	} else {
		doc["Status"] = "On";
		doc["State"]  = 1;
	}
	doc["Color"]  = activeColor;
	serializeJson(doc, jsonLightStatus);
	client.publish(topic.c_str(), jsonLightStatus.c_str());

	Serial.print("Publishing light status to : ");
	Serial.println(topic.c_str());
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
 if (topic == subscribe_topics[2]) { SetState(payload); }
 if (topic == subscribe_topics[3]) { SetColor(payload); }
 if (topic == subscribe_topics[4]) { ESP.restart();     }
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



//********************************************************************************
// Set State of Peripheral (On/Off)
//********************************************************************************
void SetState(String payload) {
String SaveActiveColor = activeColor;

  if (payload == "On") {
	  colorRGB(ReadSavedColor());
	  PublishLightStatus();
      return;
  }
  if (payload == "Off") {
     colorRGB("#00000000");
     PublishLightStatus();
     return;
  }
  if (payload == "BlinkRed") {
	 if (SaveActiveColor != "#00000000") {
	     colorRGB("#00000000");
	     delay(500);
	 }
     for (int i=0; i<5; i++) {
    	 colorRGB("#FF000000");
    	 delay(500);
    	 colorRGB("#00000000");
    	 delay(500);
     }
	 colorRGB(SaveActiveColor);
     return;
  }
  if (payload == "BlinkGreen") {
	 if (SaveActiveColor != "#00000000") {
	     colorRGB("#00000000");
	     delay(500);
	 }
     for (int i=0; i<5; i++) {
    	 colorRGB("#00FF0000");
    	 delay(500);
    	 colorRGB("#00000000");
    	 delay(500);
     }
	 colorRGB(SaveActiveColor);
     return;
  }
  if (payload == "BlinkBlue") {
	 if (SaveActiveColor != "#00000000") {
	     colorRGB("#00000000");
	     delay(500);
	 }
     for (int i=0; i<5; i++) {
    	 colorRGB("#0000FF00");
    	 delay(500);
    	 colorRGB("#00000000");
    	 delay(500);
     }
	 colorRGB(SaveActiveColor);
     return;
  }
  if (payload == "BlinkWhite") {
	 if (SaveActiveColor != "#00000000") {
	     colorRGB("#00000000");
	     delay(500);
	 }
     for (int i=0; i<5; i++) {
    	 colorRGB("#FFFFFFFF");
    	 delay(500);
    	 colorRGB("#00000000");
    	 delay(500);
     }
	 colorRGB(SaveActiveColor);
     return;
  }
  if (payload == "Blink") {
	 String FSColor = ReadSavedColor();
	 if (SaveActiveColor != "#00000000") {
	     colorRGB("#00000000");
	     delay(500);
	 }
     for (int i=0; i<5; i++) {
    	 colorRGB(FSColor);
    	 delay(500);
    	 colorRGB("#00000000");
    	 delay(500);
     }
	 colorRGB(SaveActiveColor);
     return;
  }
}



//********************************************************************************
// Set Color of Light in Hexa format #FFFFFFFF (4 colors)
//********************************************************************************
void SetColor(String payload) {
	if (payload.substring(0,1) == "#") {
		activeColor = payload;
		SaveColorValue(activeColor);
	  	colorRGB(activeColor);
 	  	PublishLightStatus();
	  	return;
	}
}



//********************************************************************************
// Read saved color
//********************************************************************************
String ReadSavedColor() {
const char* color = "";

	 if (SPIFFS.exists("/savedcolor.json")) {
		  File file = SPIFFS.open("/savedcolor.json", "r"); // @suppress("Abstract class cannot be instantiated")
		  while(file.available()) {
		    String line = file.readStringUntil('\n');
			StaticJsonDocument<64> doc;
			deserializeJson(doc, line);
			color = doc["color"];
			file.close();
		    return String(color);
		  }
	 } else {
		 return "#FFFFFFFF";
	 }
	 return "#00000000";
}



//********************************************************************************
// Saved color Value
//********************************************************************************
void SaveColorValue(String color) {
String jsonStatus;

	StaticJsonDocument<128> doc;
	doc["color"] = color;
	serializeJson(doc, jsonStatus);

	File file2 = SPIFFS.open("/savedcolor.json", "w"); // @suppress("Abstract class cannot be instantiated")
	file2.print(jsonStatus);
    // int bytesWritten = file.print(deviceJson);
	file2.close();
}




//****************************************************************************
// Turn LED to Received color
//****************************************************************************
void colorRGB(String HexValue){

  activeColor = HexValue;
  unsigned long number = (unsigned long) strtoul( &HexValue[1], NULL, 16);

  int subStringR = number >> 24;
  int subStringG = number >> 16 & 0xFF;
  int subStringB = number >> 8 & 0xFF;
  int subStringW = number & 0xFF;

  analogWrite(pinRed,map(subStringR, 0, 255, 0 ,1024));
  analogWrite(pinGreen,map(subStringG, 0, 255, 0 ,1024));
  analogWrite(pinBlue,map(subStringB, 0, 255, 0 ,1024));
  analogWrite(pinWhite,map(subStringW, 0, 255, 0 ,1024));
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

