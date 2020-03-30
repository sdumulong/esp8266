#include <FS.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
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

struct device_def {
	String      frendlyName;
	const char* deviceIP;
	int         devicePort;
};

struct device_state {
	String      state;
	String      brightness;
};


#define SERIAL_BAUDRATE	19200
bool    	debug        = true; //Affiche sur la console si True
struct struct_config config;
struct device_def    devices[20];
struct device_state  deviceState;

int     	serverPort   = 80;
String      FriendlyName= "";
const char* APssid   	= "HubBelkin";
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
// UPnPDevice device;


String wemo_on="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>1</BinaryState></u:SetBinaryState></s:Body></s:Envelope>";
String wemo_off="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>0</BinaryState></u:SetBinaryState></s:Body></s:Envelope>";
String wemo_brightness_value="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:SetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"><BinaryState>1</BinaryState><brightness>~~~</brightness></u:SetBinaryState></s:Body></s:Envelope>";
String wemo_state="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:GetBinaryState xmlns:u=\"urn:Belkin:service:basicevent:1\"></u:GetBinaryState></s:Body></s:Envelope>";


//===================================================================================
// Setup procedure
//===================================================================================
void setup() {
	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	delay(10);

	devices[0].frendlyName = "Lustre";
	devices[0].deviceIP    = "192.168.0.15";
	devices[0].devicePort  = 49153;
	devices[1].frendlyName = "Exterieur";
	devices[1].deviceIP    = "192.168.0.66";
	devices[1].devicePort  = 49153;
	devices[2].frendlyName = "Office";
	devices[2].deviceIP    = "192.168.0.82";
	devices[2].devicePort  = 49153;

	Serial.println('\n');

	SPIFFS.begin();
	if (!SPIFFS.exists("/config.json")) {
		mdns.begin("ESP8266-Setup",local_IP);
		startAccessPoint();
	} else {
		WiFi.softAPdisconnect (true);
		readConfigFile();
		setup_wifi();
		if (config.mqtt)   {setup_mqtt();  }
		if (config.api)    {setup_api();   }
		Wemo_Subscribing();
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
//	Serial.println(	config.deviceID  );
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

	  WiFi.mode(WIFI_AP_STA);          //Both Access point and Wifi client are enabled
	  WiFi.softAP(APssid);             //Start HOTspot removing password will disable security

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
// Configuration de la connexion API
//********************************************************************************
void setup_api() {
    Serial.print("Starting API service...");
	server.on("/"       , handleRootDevice);
	server.on("/api/config" , handleConfig);
	server.on("/api/reset"  , handleReset);
	server.on("/api/restart", handleRestart);
	server.on("/api/setbinarystate", handleBinaryState);
	server.on("/api/getbinarystate", handleGetBinaryState);
	server.on("/wemoSubscriptions", handleSubscriptions);
	server.on("/devicemanager", handleDeviceManager);
	server.on("/adddevice", handleAddDevice);

	server.begin(); //Start API server
	Serial.println("OK");
    Serial.println("Registered API");
    Serial.println("   /");
    Serial.println("   /api/config");
    Serial.println("   /api/reset");
    Serial.println("   /api/restart");
    Serial.println("   /api/setbinarystate");
    Serial.println("   /api/getbinarystate\n");
    Serial.println("   /wemoSubscriptions\n");
	MDNS.addService("http", "tcp", serverPort);
}




//********************************************************************************
// Configuration de la connexion API
//********************************************************************************
void Wemo_Subscribing() {

	WiFiClient Wemoclient;

	Serial.println("Initializing wemo subscriptions");
	for (int i = 0; i <= 2; i++) {
		Serial.print("    Connecting to Wemo at ");
		Serial.println(devices[i].deviceIP);
		if (!Wemoclient.connect(devices[i].deviceIP, devices[i].devicePort)) {
		    Serial.println("Connection failed");
		    return;
		}
		Wemoclient.println("\"SUBSCRIBE /upnp/event/basicevent1 HTTP/1.0");
		Wemoclient.println("Content-Length: 0");
		Wemoclient.println("HOST: " + String(devices[i].deviceIP));
		Wemoclient.println("CALLBACK: <http://192.168.0.83/wemoSubscriptions>");
		Wemoclient.println("NT: upnp:event");
		Wemoclient.println("TIMEOUT: infinite");
		Wemoclient.println("Connection: close\"");
		delay(100);
	}
	Serial.println();
}



//********************************************************************************
//
//********************************************************************************
void handleDeviceManager() {
String htmlPage = "";
String device = "<tr><td width=\"100\">Lustre</td><td width=\"100\">Switch</td><td width=\"134\">192.168.0.154</td><td width=\"67\">15874</td><td width=\"28\" align=\"right\"><a href=\"/delete?device=Lustre\">del</a></td></tr>";
    Serial.println("Request for Device Manager");


    bool fsMounted = SPIFFS.begin();

    	if (fsMounted ) {
    		Serial.println("File system mounted with success");
    	} else {
    		Serial.println("Error mounting the file system");
    		return;
    	}
    	if (SPIFFS.exists("/devices.html")) {
    		File file = SPIFFS.open("/devices.html", "r"); // @suppress("Abstract class cannot be instantiated")
    		if (!file) { Serial.println("Error opening file"); }
    		Serial.println("\nReading device manager page");
    		  while(file.available()) {
    		  	htmlPage = htmlPage + file.readStringUntil('\n');
    		  }
    		htmlPage.replace(">><<",device);
    	    server.send(200, "<! DOCTYPE html>", htmlPage.c_str());
    	}
}



//********************************************************************************
//
//********************************************************************************
void handleAddDevice() {
	String jsonConfig;

	   Serial.println("Reception of Data");

	   StaticJsonDocument<512> doc;
	   doc["frendlyName"] 	= server.arg("name");
	   doc["deviceType"] 	= server.arg("type");
	   doc["deviceIp"] 		= server.arg("IP");
	   doc["devicePort"] 	= server.arg("Port");

	   serializeJson(doc, jsonConfig);
	   Serial.println(jsonConfig);
	   handleDeviceManager();
}




//********************************************************************************
//
//********************************************************************************
void handleSubscriptions() {
    Serial.println("Receive Subscriptions");
    server.send(200, "<! DOCTYPE json>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>OK</body></html>" );
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
	Serial.println("Reset request");
    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Resetting HubBelkin</body></html>" );
	SPIFFS.begin();
	SPIFFS.remove("/config.json");
    ESP.restart();
}



//********************************************************************************
//
//********************************************************************************
void handleRestart() {
	Serial.println("Restart request");
    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Restarting HubBelkin</body></html>" );
    ESP.restart();
}




//********************************************************************************
//
//********************************************************************************
void handleBinaryState() {
//const char*  IP;
//int   state  ;
	Serial.println("Set Binary State request");
    server.send(200, "<! DOCTYPE html>", "<html><head><meta http-equiv=\"Content-Language\" content=\"en-ca\"><meta name=\"viewport\" content=\"width=device-width, user-scalable=no\"head><body>Setting Binary State</body></html>" );

//    IP    = server.arg("ip");
//    state = server.arg("state");
//	wemo_control(IP,state);
}



//********************************************************************************
//
//********************************************************************************
void handleGetBinaryState() {
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
//    if (client.connect("HubBelkin")) {
      Serial.println("OK");
      mqtt_status = "Connected";
      for (int i=0; i<nb_subscribe_topics; i++) {
        client.subscribe(subscribe_topics[i].c_str());
        if (debug) { Serial.println("    Subscribing to topic: " + subscribe_topics[i]); }
        delay(10);
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
		unsigned long now = millis();
		if (now - lastMsg > (1000 * 120)) {
			lastMsg = now;
			PublishLightStatus();
		}
	}
}



//****************************************************************************
// Publishing Light Status
//****************************************************************************
void PublishLightStatus() {
String  values;

	for (int i = 0; i <= 2; i++) {
		// Serial.println(devices[i].deviceIP);
		values = wemo_getState(devices[i].deviceIP, devices[i].devicePort);

		String pub_topicState  =  FriendlyName + "/Wemo/BinaryState/" + devices[i].frendlyName;
		String pub_topicBright =  FriendlyName + "/Wemo/Brightness/" + devices[i].frendlyName;
		Serial.print("Publishing status of ");
		Serial.println(pub_topicState);

		client.publish(pub_topicState.c_str(),  deviceState.state.c_str());
	  	if (deviceState.brightness != "x")
	  		client.publish(pub_topicBright.c_str(), deviceState.brightness.c_str());

//		client.publish(pub_topicState.c_str(), values.c_str());
//		if (values.brightness != "x")
//			client.publish(pub_topicBright.c_str(), values.brightness.c_str());
	}
}



//********************************************************************************
// Initialise Subscribed topc
//********************************************************************************
void sub_Topics(){
	 nb_subscribe_topics = 6;
	 subscribe_topics[0] = String(config.deviceID) + "/Config" ;
	 subscribe_topics[1] = String(config.deviceID) + "/Reset"  ;
	 subscribe_topics[2] = String(config.deviceID) + "/Restart";
	 subscribe_topics[3] = String(config.deviceID) + "/Wemo/GetBinaryState/#";
	 subscribe_topics[4] = String(config.deviceID) + "/Wemo/SetBinaryState/#";
	 subscribe_topics[5] = String(config.deviceID) + "/Wemo/SetBrightness/#";
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
 if (topic == subscribe_topics[2]) { ESP.restart();           }
 if (topic.substring(0, 25) == subscribe_topics[3].substring(0, 25)) { GetBinaryState(topic);          }
 if (topic.substring(0, 25) == subscribe_topics[4].substring(0, 25)) { SetBinaryState(topic, payload); }
 if (topic.substring(0, 25) == subscribe_topics[5].substring(0, 25)) { SetBrightness(topic, payload);  }
}



//********************************************************************************
// Display controler status
//********************************************************************************
void DisplayConfig() {
String jsonStatus;

	jsonStatus = CreateJsonStatus();
	Serial.println(jsonStatus);
	client.publish(FriendlyName.c_str(),jsonStatus.c_str());
}



//********************************************************************************
// Get Binary State
//********************************************************************************
void GetBinaryState(String topic) {
String      	device      = "";
const char* 	deviceIP    = "";
int         	devicePort  = 0;
device_state  	values;
String 			returnValue = "";


	device = topic.substring(30);
	for (int i = 0; i <= 20; i++) {
		if (devices[i].frendlyName == device) {
			deviceIP   = devices[i].deviceIP;
			devicePort = devices[i].devicePort;
			break;
		}
	}
	returnValue = wemo_getState(deviceIP, devicePort);
	String pub_topicState  =  FriendlyName + "/Wemo/BinaryState/" + device;
 	String pub_topicBright =  FriendlyName + "/Wemo/Brightness/" + device;

	client.publish(pub_topicState.c_str(),  deviceState.state.c_str());
  	if (deviceState.brightness != "x")
  		client.publish(pub_topicBright.c_str(), deviceState.brightness.c_str());
}




//********************************************************************************
// Set Binary State
//********************************************************************************
void SetBinaryState(String topic, String payload) {
	String      device      = "";
	const char* deviceIP    = "";
	int         devicePort  = 0;

		device = topic.substring(30);
		for (int i = 0; i <= 20; i++) {
			if (devices[i].frendlyName == device) {
				deviceIP   = devices[i].deviceIP;
				devicePort = devices[i].devicePort;
				break;
			}
		}
	String pub_topicState  =  FriendlyName + "/Wemo/BinaryState/" + device;
	Serial.print("Publishing status of ");
	Serial.println(pub_topicState);
	client.publish(pub_topicState.c_str(), payload.c_str());
	wemo_control(deviceIP, devicePort, payload.toInt());
}



//********************************************************************************
// Set Brightness
//********************************************************************************
void SetBrightness(String topic, String payload) {
	String      device      = "";
	const char* deviceIP    = "";
	int         devicePort  = 0;

	Serial.print("Setting brightness to ");
	Serial.println(payload);
		device = topic.substring(29);
		for (int i = 0; i <= 20; i++) {
			if (devices[i].frendlyName == device) {
				deviceIP   = devices[i].deviceIP;
				devicePort = devices[i].devicePort;
				break;
			}
		}
	String pub_topicState = FriendlyName + "/Wemo/Brightness/" + device;
	Serial.print("Publishing brightness of ");
	Serial.println(pub_topicState);
	client.publish(pub_topicState.c_str(), payload.c_str());
	wemo_brightness(deviceIP, devicePort, payload.toInt());
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
// Wemo Get State
//********************************************************************************
String wemo_getState(const char* IP, int port) {
int     		pos = 0;
int     		pos1 = 0;
String  		binaryState = "";
String  		brightness  = "";

//  Serial.print("Connecting to Wemo at ");
//  Serial.println(IP);

  // Use WiFiClient class to create TCP connections
  WiFiClient Wemoclient;

  if (!Wemoclient.connect(IP, port)) {
    Serial.println("Connection failed");
    return "";
  }
	Wemoclient.println("POST /upnp/control/basicevent1 HTTP/1.1");
	Wemoclient.println("Host: " + String(IP) + ":" + String(port));
	Wemoclient.println("User-Agent: ESP8266/1.0");
	Wemoclient.println("Connection: close");
	Wemoclient.println("Content-type: text/xml; charset=\"utf-8\"");
	Wemoclient.print("Content-Length: ");
	Wemoclient.println(wemo_state.length()); // both wemo_on and wemo_off are the same length, just in case it changes in the future
	Wemoclient.println("SOAPACTION: \"urn:Belkin:service:basicevent:1#GetBinaryState\"");
	Wemoclient.println();
	Wemoclient.println(wemo_state);
	delay(10);

  while(Wemoclient.available()){
    String line = Wemoclient.readStringUntil('\r');
    pos = line.indexOf("</BinaryState>");
    if (pos > 0) binaryState = line.substring(pos-1, pos);
    pos1 = line.indexOf("</brightness>");
    if (pos1 > 0) brightness = line.substring(13, pos1);
  }
  Wemoclient.flush();

  if (brightness != "") {
	  deviceState.state = binaryState;
	  deviceState.brightness = brightness;}

  if (brightness == "") {
	  deviceState.state = binaryState;
  	  deviceState.brightness = "0";}

  return binaryState;
}



//********************************************************************************
// Wemo Control
//********************************************************************************
void wemo_control(const char* IP, int port, int cmd) {

//  Serial.print("Connecting to ");
//  Serial.println(IP);

  // Use WiFiClient class to create TCP connections
  WiFiClient Wemoclient;

  if (!Wemoclient.connect(IP, port)) {
    Serial.println("Connection failed");
    return;
  }

  // This will send the request to the server
  Wemoclient.println("POST /upnp/control/basicevent1 HTTP/1.1");
  Wemoclient.println("Host: " + String(IP) + ":" + String(port));
  Wemoclient.println("User-Agent: ESP8266/1.0");
  Wemoclient.println("Connection: close");
  Wemoclient.println("Content-type: text/xml; charset=\"utf-8\"");
  Wemoclient.print("Content-Length: ");
  if (cmd == 1) {
	  Wemoclient.println(wemo_on.length()); // both wemo_on and wemo_off are the same length, just in case it changes in the future
  }
  else {
	  Wemoclient.println(wemo_off.length());
  }
  Wemoclient.println("SOAPACTION: \"urn:Belkin:service:basicevent:1#SetBinaryState\"");
  Wemoclient.println();
  if (cmd == 1) {
	  Wemoclient.println(wemo_on);
  }
  else {
	  Wemoclient.println(wemo_off);
  }
  delay(10);
  Serial.println("Response___________");
  // Read all the lines of the reply from server and print them to Serial
  while(Wemoclient.available()){
    String line = Wemoclient.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("Closing connection");
}




//********************************************************************************
// Wemo Control
//********************************************************************************
void wemo_brightness(const char* IP, int port, int brightness) {
String wemo_new_brightness = wemo_brightness_value;
//  Serial.print("Connecting to ");
//  Serial.println(IP);

  // Use WiFiClient class to create TCP connections
  WiFiClient Wemoclient;

  if (!Wemoclient.connect(IP, port)) {
    Serial.println("Connection failed");
    return;
  }

  // This will send the request to the server
  Wemoclient.println("POST /upnp/control/basicevent1 HTTP/1.1");
  Wemoclient.println("Host: " + String(IP) + ":" + String(port));
  Wemoclient.println("User-Agent: ESP8266/1.0");
  Wemoclient.println("Connection: close");
  Wemoclient.println("Content-type: text/xml; charset=\"utf-8\"");
  Wemoclient.print("Content-Length: ");
  Wemoclient.println(wemo_brightness_value.length()); // both wemo_on and wemo_off are the same length, just in case it changes in the future
  Wemoclient.println("SOAPACTION: \"urn:Belkin:service:basicevent:1#SetBinaryState\"");
  Wemoclient.println();
  wemo_new_brightness.replace("~~~",String(brightness));
  Wemoclient.println(wemo_new_brightness);
  delay(10);

//  Serial.println("____________Response___________");
  // Read all the lines of the reply from server and print them to Serial
  while(Wemoclient.available()){
    String line = Wemoclient.readStringUntil('\r');
    Serial.print(line);
  }

//  Serial.println();
//  Serial.println("Closing connection");
}
