#include <FS.h>  // Include the SPIFFS library
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

struct struct_config {
 String ssid;
 String pwd;
 String deviceID;
 bool mqtt;
 String mqttIP;
 int mqttPort;
 String mqttUser;
 String mqttPwd;
 String IPmode;
 String staticIP;
 String gateway;
 String subnet;
 String dns;
 String mac;
 bool syslog;
 String syslogServer;
 int syslogPort;
 bool upnp;
 bool api;
 bool ssl;
 String certPath;
};

#define     SERIAL_BAUDRATE	115200
bool   debug       = true; //Affiche sur la console si True
struct struct_config config;

const char* APssid   = "BedlightV3";

IPAddress local_IP(192,168,4,3);
IPAddress gateway(192,168,4,255);
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80); //Server on port 80

char    message_buff[100];
int     nb_subscribe_topics;
String  subscribe_topics[10];
bool    modeAccessPoint = false;
String  mqtt_status = "Not connected";


WiFiClient   espClient; // @suppress("Abstract class cannot be instantiated")
PubSubClient client(espClient);

//===================================================================================
// Setup procedure
//===================================================================================
void setup() {
	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	delay(10);
	Serial.println('\n');

	SPIFFS.begin();
	//SPIFFS.remove("/config.json");
	if (!SPIFFS.exists("/config.json")) {
		startAccessPoint();
	} else {
		readConfigFile();
		setup_wifi();
		if (config.ssl)    {setup_ssl();   }
		if (config.mqtt)   {setup_mqtt();  }
		if (config.syslog) {setup_syslog();}
		if (config.upnp)   {setup_upnp();  }
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
 Serial.println("\nReading config file");
 parseConfigJson(file.readStringUntil('\n'));
}



//==================================================================================
// Parse JSON read from FS
//==================================================================================
void parseConfigJson(String document)  {
	Serial.println("\nParsing config file");
	Serial.println(document);

	StaticJsonDocument<512> doc;
	deserializeJson(doc, document);
	const char* ssid         = doc["ssid"];
	const char* pwd          = doc["pwd"];
	const char* deviceID     = doc["deviceID"];
	const char* mqtt         = doc["mqtt"];
	const char* mqttIP       = doc["mqttIP"];
	const char* mqttUser     = doc["mqttUser"];
	const char* mqttPwd      = doc["mqttPwd"];
	const char* IPmode       = doc["IPmode"];
	const char* staticIP     = doc["staticIP"];
	const char* gateway      = doc["gateway"];
	const char* subnet       = doc["subnet"];
	const char* dns          = doc["dns"];
	const char* mac          = doc["mac"];
	const char* syslogServer = doc["syslogServer"];
	const char* syslogPort   = doc["syslogPort"];
	const char* certPath     = doc["certPath"];

	config.ssid     = ssid;
	config.pwd      = pwd;
	config.deviceID = deviceID;
	//if (mqtt == "ON") { config.mqtt = true; } else { config.mqtt = false; }
	config.mqtt         = true;
	config.mqttIP       = mqttIP;
	config.mqttPort     = 1883;
	config.mqttUser     = mqttUser;
	config.mqttPwd      = mqttPwd;
	config.IPmode       = IPmode;
	config.staticIP     = staticIP;
	config.gateway      = gateway;
	config.subnet       = subnet;
	config.dns          = dns;
	config.dns          = mac;
	config.syslogServer = syslogServer;
//	config.syslogPort   = syslogPort.toInt();
	config.certPath     = certPath;
	config.syslog       = false;
	config.upnp         = false;
	config.api		    = false;
	config.ssl		    = false;
	config.ssl	        = false;
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
//     This rutine is exicuted when you open its IP in browser
//===================================================================================
void handlePost() {
void(* resetFunc) (void) = 0; //declare reset function @ address 0
String parms;

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

   serializeJson(doc, parms);
   Serial.print(parms);

   File file = SPIFFS.open("/config.json", "w");  // open the file in write mode // @suppress("Abstract class cannot be instantiated")
   file.println(parms);
   Serial.println("\nconfig file saved");

   server.send(200, "<! DOCTYPE html>", "<html><body>Saved</body><html>");
   //resetFunc();
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
//Connexion au r�seau WiFi
//********************************************************************************
void setup_wifi() {
const char* ptrWifi_ssid = config.ssid.c_str();
const char* ptrWifi_password = config.pwd.c_str();

  delay(10);
  Serial.println();
  Serial.print("Connecting to :");
  Serial.print(config.ssid);
  Serial.print(" ");
  WiFi.begin(ptrWifi_ssid, ptrWifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(" ");
  Serial.println("Connexion WiFi etablie ");
  Serial.print("=> Addresse IP : ");
  Serial.println(WiFi.localIP());
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
const char* ptrmqtt_server = config.mqttIP.c_str();

 client.setServer(ptrmqtt_server, config.mqttPort); //Configuration de la connexion au serveur MQTT
 client.setCallback(callback); //La fonction de callback qui est execut�e � chaque r�ception de message
 sub_Topics();
}


//********************************************************************************
// Initialise Subscribed topc
//********************************************************************************
void sub_Topics(){
	nb_subscribe_topics = 3;
 	subscribe_topics[0] = config.deviceID + "/Status" ;
 	subscribe_topics[1] = config.deviceID + "/Reset" ;
 	subscribe_topics[2] = config.deviceID + "/Set" ;
}


//********************************************************************************
// Configuration de la connexion Syslog
//********************************************************************************
void setup_syslog() {
}


//********************************************************************************
// Configuration de la connexion UPnP
//********************************************************************************
void setup_upnp() {
}


//********************************************************************************
// Configuration de la connexion API
//********************************************************************************
void setup_api() {
}



//********************************************************************************
// Check for incomming messages
//********************************************************************************
void callback(char* topic, byte* payload, unsigned int length) {
std::size_t i;
 for(i=0; i<length; i++) { message_buff[i] = payload[i]; }
 message_buff[i] = '\0';
 String msgString = String(message_buff);

 if ( debug ) { Serial.println("Topic recu => Payload: " + msgString); }
 //TraiterMessageRecu(topic, msgString);
}




//********************************************************************************
//Connexion or Reconnexion
//********************************************************************************
void reconnect() { //Boucle jusqu'� obtenur une reconnexion

  while (!client.connected()) {
    Serial.print("MQTT Connecxion...");

    if (client.connect(config.ssid.c_str())) {
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
	server.handleClient();
	//Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());
	//delay(3000);

	if (!modeAccessPoint) {
		if (!client.connected()) { reconnect(); }
		client.loop();
	}
}

