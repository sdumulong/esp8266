// ************************************************************************************************************
// Appel possible
//   Bedlight/Status                     --> Transmet un JSON sur le statut du controleur
//   Bedlight/Reset                      --> Fait un reset du controleur
//   Bedlight/Set    #FF0000             --> Allume les leds en rouge
//   Bedlight/Set    On                  --> Allume les leds avec la dernière couleurs affecté
//   Bedlight/Set    Off                 --> Éteins les leds
//   Bedlight/Set    1                   --> Allume les leds avec la dernière couleurs affecté
//   Bedlight/Set    0                   --> Éteins les leds
//   Bedlight/Set    Blink               --> Faire blinker les leds avec la couleur activeColor
//   Bedlight/Set    BlinkRed            --> Faire blinker les leds rouge
//   Bedlight/Set    BlinkGreen          --> Faire blinker les leds verte
//   Bedlight/Set    BlinkBlue           --> Faire blinker les leds bleue
//   Bedlight/Set    PubDelay=30         --> Initialise le temps d'intervales entre les publications d'état
// **************************************************************************************************

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>

// D1 = Pin-D3
// D2 = Pin-D4, Pin-D14
// D3 = Pin-D8
// D4 = Pin-D9
// D5 = Pin-D5, Pin-D13
// D6 = Pin-D6, Pin-D12
// D7 = Pin-D7, Pin-D11
// D8 = Pin-D10

#define     SERIAL_BAUDRATE	115200

String  MQTT_id         = "";
String  wifi_ssid 		= "";
String  wifi_password 	= "";
String  mqtt_server 	= "";
String  mqtt_port    	= "";
String  mqtt_status    	= "Not connected";

//SSID and Password to your ESP Access Point
const char* APssid   = "BedlightV2";
const char* password = "12345678";

IPAddress local_IP(192,168,4,2);
IPAddress gateway(192,168,4,255);
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80); //Server on port 80

String blinkDelay = "" ;
String pubDelay   = "";

int     ChannelRed      = D2;
int     ChannelGreen    = D6;
int     ChannelBlue     = D5;
int     pirPin      	= D7;
int     microSwitch 	= D8;

// Motion Definition

String  activeColor    		= "#000000";  //Light Value
String  activeColorOff		= "#000000";  //Light Value
String  savedColor   		= "#FFFFFF";
String  backupColor   		= "#000000";

bool    debug           = true; //Affiche sur la console si True
unsigned long lastMsg   = 0;    //Horodatage du dernier message publié sur MQTT
unsigned long lastMsg2  = 0;
int     blinkCount     	= 0;
bool    modeAccessPoint = false;

char    message_buff[100];
int     nb_subscribe_topics;
String  subscribe_topics[10];

//Création des objets
WiFiClient   espClient; // @suppress("Abstract class cannot be instantiated")
PubSubClient client(espClient);



//===================================================================================
//     This routine is executed when you open its IP in browser
//===================================================================================
void handleRoot() {
  server.send(200, "<! DOCTYPE html>", "<html><body><form action=""/save"">Network Setup<br><br>SSID :<input type=""text"" name=""SSID"" id=""SSID""><br>PWD  :<input type=""password"" name=""password"" id=""password""><br>MQTT ID:<input type=""text"" name=""mqttID"" id=""mqttID""><br>MQTT IP :<input type=""text"" name=""mqttIP"" id=""mqttIP""><br>MQTT Port :<input type=""text"" name=""mqttPort"" id=""mqttPort""><br><br>&nbsp;&nbsp;&nbsp;&nbsp;<input type=""submit"" value=""Save Network configuration""></form></body></html>");
}


//********************************************************************************
// Setup  (Initialisation du programme)
//********************************************************************************
void setup() {
const char* ptrmqtt_server = mqtt_server.c_str();
	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	pinMode(microSwitch, INPUT);
	pinMode(pirPin, INPUT);

	ReadSetupValues();
	SetLight("#000000");

	  if (!ReadNetworkSetup()) {
		  modeAccessPoint = true;
	  	  startAccessPoint();
	  } else {
		  modeAccessPoint = false;
		  initSubscribeTopics();
		  setup_wifi();           				//On se connecte au réseau wifi
		  client.setServer(ptrmqtt_server, mqtt_port.toInt());    //Configuration de la connexion au serveur MQTT
		  client.setCallback(callback);           //La fonction de callback qui est executée à chaque réception de message
	  }
}


//********************************************************************************
// Init Topics
//********************************************************************************
void initSubscribeTopics() {
	nb_subscribe_topics = 3;
	subscribe_topics[0]  = MQTT_id + "/Status" ;
	subscribe_topics[1]  = MQTT_id + "/Reset" ;
	subscribe_topics[2]  = MQTT_id + "/Set" ;
}



//===================================================================================
//     This rutine is exicuted when you open its IP in browser
//===================================================================================
void handlePost() {
void(* resetFunc) (void) = 0; //declare reset function @ address 0
String SSID = server.arg("SSID");
String PWD = server.arg("password");
String MQTTID = server.arg("mqttID");
String MQTTIP = server.arg("mqttIP");
String MQTTPort = server.arg("mqttPort");

   Serial.println("Reception of Data");
   server.send(200, "<! DOCTYPE html>", "<html><body>Saved</body><html>");
   saveNetworkSetup(SSID, PWD, MQTTID, MQTTIP, MQTTPort);
   resetFunc();
}



//==================================================================================
//     Start Access point
//==================================================================================
void startAccessPoint() {

	  Serial.print("Setting soft-AP configuration ... ");
	  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

	  Serial.print("Setting soft-AP ... ");
	  Serial.println(WiFi.softAP("ESPsoftAP_01") ? "Ready" : "Failed!");

	  Serial.print("Soft-AP IP address = ");
	  Serial.println(WiFi.softAPIP());

	  WiFi.mode(WIFI_AP_STA);         //Both Access point and Wifi client are enabled
	  WiFi.softAP(APssid, password);  //Start HOTspot removing password will disable security

	  server.on("/", handleRoot);      //Which routine to handle at root location
	  server.on("/save", handlePost);
	  server.begin();                  //Start server

	  Serial.println("HTTP server started");
}



//********************************************************************************
// Main Loop
//********************************************************************************
void loop() {

	server.handleClient();          //Handle client requests

	if (!modeAccessPoint) {
		if (!client.connected()) { reconnect(); }
		client.loop();
		monitorResetSwitch();
	  }

	unsigned long now = millis();
	if (now - lastMsg > (1000 * pubDelay.toInt())) {
		lastMsg = now;
		PublishLightStatus();
	}
}



//********************************************************************************
//Connexion au réseau WiFi
//********************************************************************************
void setup_wifi() {
const char* ptrWifi_ssid 		= wifi_ssid.c_str();
const char* ptrWifi_password 	= wifi_password.c_str();

  delay(10);
  Serial.println();
  Serial.print("Connecting to :");
  Serial.print(wifi_ssid);
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
//Connexion or Reconnexion
//********************************************************************************
void reconnect() {   //Boucle jusqu'à obtenur une reconnexion

  while (!client.connected()) {
    Serial.print("MQTT Connecxion...");

    if (client.connect(MQTT_id.c_str())) {
      Serial.println("OK");
      mqtt_status    	= "Connected";
      for (int i=0; i<nb_subscribe_topics; i++) {
        client.subscribe(subscribe_topics[i].c_str());
        if (debug) { Serial.println("Subscribing to topic: " +  subscribe_topics[i]); }
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



//****************************************************************************
void SaveSetupValues(String Color) {
//****************************************************************************
	  SPIFFS.begin();
	  if (SPIFFS.exists("/setupValues.txt")) { SPIFFS.remove("/lightValues.txt"); }

      File f = SPIFFS.open("/setupValues.txt", "w");  // open the file in write mode // @suppress("Abstract class cannot be instantiated")

	  if (!f) {
	    	Serial.println("file setupValues.txt creation failed");
	    } else {
	    	f.println("SavedColor=" + Color);
	    	f.println("BlinkDelay=" + blinkDelay);
	    	f.println("PubDelay="   + pubDelay);
		    Serial.println("Color data saved");
		    savedColor = Color;
	    }
//	  Serial.println(Color);
//	  Serial.println(blinkDelay);
//	  Serial.println(pubDelay);
}



//===================================================================================
//     Read saved light values configuration file
//===================================================================================
void ReadSetupValues() {

	 SPIFFS.begin();
	 if (SPIFFS.exists("/setupValues.txt")) {
		  File f = SPIFFS.open("/setupValues.txt", "r"); // @suppress("Abstract class cannot be instantiated")
		  Serial.println("\nReading setup file...");
		    while(f.available()) {
		    	String line = f.readStringUntil('\n');
		    	int splitChar = line.indexOf('=');

		    	if (line.substring(0, splitChar)=="SavedColor") {
		    		activeColor = line.substring(splitChar+1).c_str();
		    		activeColor.remove(activeColor.length()-1);
		    		savedColor = activeColor;
		    	}
		    	if (line.substring(0, splitChar)=="BlinkDelay") {
		    		blinkDelay = line.substring(splitChar+1).c_str();
		    		blinkDelay.remove(blinkDelay.length()-1);
		    	}
		    	if (line.substring(0, splitChar)=="PubDelay") {
		    		pubDelay = line.substring(splitChar+1).c_str();
		    		pubDelay.remove(pubDelay.length()-1);
		    		pubDelay = "30";
		    	}
		    }
	Serial.println(activeColor);
	Serial.println(blinkDelay);
	Serial.println(pubDelay);
	} else { Serial.println("No saved light file found."); }
}



//=================================================================================
//    Save network setup to file
//=================================================================================
void saveNetworkSetup(String ssid, String pwd, String mqttid, String mqttip, String mqttport) {

	  SPIFFS.begin();
	  File f = SPIFFS.open("/network.txt", "r"); // @suppress("Abstract class cannot be instantiated")

	  if (SPIFFS.exists("/network.txt")) { SPIFFS.remove("/network.txt"); }

	  if (!f) {
	    File f = SPIFFS.open("/network.txt", "w");  // open the file in write mode // @suppress("Abstract class cannot be instantiated")

	    if (!f) {
	    	Serial.println("file creation failed");
	    	Serial.println("Resetting board...");
	    } else {
	    	f.println("ssid="     + ssid);
	    	f.println("password=" + pwd);
	    	f.println("MQTTID="   + mqttid);
	    	f.println("MQTTIP="   + mqttip);
	    	f.println("MQTTPort=" + mqttport);
		    Serial.println("File created and data saved");
	    }
	  }
}

//===================================================================================
//     Read the configuration file
//===================================================================================
bool ReadNetworkSetup() {

	  SPIFFS.begin();
//	  SPIFFS.remove("/network.txt");
//	  Serial.println("Looking for network config file...");
	  if (SPIFFS.exists("/network.txt")) {
		  File f = SPIFFS.open("/network.txt", "r"); // @suppress("Abstract class cannot be instantiated")
		  Serial.println("Reading network config file...");
		    while(f.available()) {
		    	String line = f.readStringUntil('\n');
		    	int splitChar = line.indexOf('=');

		    	if (line.substring(0, splitChar)=="ssid") {
		    		wifi_ssid = line.substring(splitChar+1).c_str();
		    		// wifi_ssid = wifi_ssid.replace("\n","");
		    		wifi_ssid.remove(wifi_ssid.length()-1);
		    	}
		    	if (line.substring(0, splitChar)=="password") {
		    		wifi_password = line.substring(splitChar+1).c_str();
		    		wifi_password.remove(wifi_password.length()-1);
		    	}
		    	if (line.substring(0, splitChar)=="MQTTID") {
		    		MQTT_id = line.substring(splitChar+1).c_str();
		    		MQTT_id.remove(MQTT_id.length()-1);
		    	}
		    	if (line.substring(0, splitChar)=="MQTTIP") {
		    		mqtt_server = line.substring(splitChar+1).c_str();
		    		mqtt_server.remove(mqtt_server.length()-1);
		    	}
		    	if (line.substring(0, splitChar)=="MQTTPort") {
		    		mqtt_port = line.substring(splitChar+1).c_str();
		    		mqtt_port.remove(mqtt_port.length()-1);
		    	}
		    }
//		  Serial.println(wifi_ssid);
//		  Serial.println(wifi_password);
//		  Serial.println(MQTT_id);
//		  Serial.println(mqtt_server);
//		  Serial.println(mqtt_port);
		  return true;
	  } else { return false; }
}



//********************************************************************************
// Check for incomming messages
//********************************************************************************
void callback(char* topic, byte* payload, unsigned int length) {
std::size_t  i;
	for(i=0; i<length; i++) { message_buff[i] = payload[i]; }
	message_buff[i] = '\0';
	String msgString = String(message_buff);

	if ( debug ) { Serial.println("Topic recu =>      Payload: " + msgString); }
	TraiterMessageRecu(topic, msgString);
}



//********************************************************************************
// Traiter message recu
//********************************************************************************
void TraiterMessageRecu(String topic, String payload) {
void(* resetFunc) (void) = 0; //declare reset function @ address 0

	if ( debug ) { Serial.println("Topic recu => " + topic + "     Payload: " + payload); }

	if (topic == subscribe_topics[0]) { DisplayStatus(); }
    if (topic == subscribe_topics[1]) {
    	SPIFFS.begin();
		SPIFFS.remove("/network.txt");
		resetFunc(); }
	if (topic == subscribe_topics[2]) { SetLight(payload); }
}



//****************************************************************************
// Publishing Light Status
//****************************************************************************
void PublishLightStatus() {
String topic =  MQTT_id + "/Light";
String strStatus = "Off";
String strState  = "0";
String strColor  = activeColor;

	if (strColor != "#000000") {
		strStatus = "On";
		strState = "1";
	}
	String strStatus2 = "{\n\"Status\" : \""  + strStatus +
                        "\",\n\"State\" : \"" + strState  +
                        "\",\n\"Color\" : \"" + strColor  +
                        "\"\n} " ;

	client.publish(topic.c_str(), (char*) strStatus2.c_str());
}



//****************************************************************************
// Reset switch control
//****************************************************************************
void monitorResetSwitch() {
void(* resetFunc) (void) = 0; //declare reset function @ address 0
int buttonStatus = 0;
int buttonPressed = 0;

buttonStatus = digitalRead(microSwitch);
//Serial.println(buttonStatus);
delay(100);
	while (buttonStatus) {
		buttonPressed ++;
		delay(100);
		buttonStatus = digitalRead(microSwitch);
//		Serial.println(buttonStatus);
	}
    if (buttonPressed > 0 && buttonPressed <= 20) {
	    Serial.println("Button Pressed less than 20");
    	resetFunc();
    }
    if (buttonPressed > 20) {
	    Serial.println("Button Pressed more than 20");
    	SPIFFS.begin();
  	    SPIFFS.remove("/network.txt");
    	resetFunc();
    }
}



//****************************************************************************
void SetLight(String payload) {
//****************************************************************************
  String saveLV = activeColor;

//  Serial.println("SetLight " + payload );
//  Serial.println("activeColor " + activeColor );

  if (payload == "On")  {
	  ReadSetupValues();
  	  colorRGB(activeColor);
  	  PublishLightStatus();
  	  Serial.println("On " + activeColor );
  	  return;
  }
  if (payload == "Off") {
	  activeColor = "#000000";
  	  colorRGB(activeColor);
  	  PublishLightStatus();
  	  Serial.println("Off " + activeColor );
  	  return;
  }
  if (payload == "#000000") {
	  activeColor = "#000000";
  	  colorRGB(activeColor);
  	  PublishLightStatus();
  	  Serial.println("Off " + activeColor );
  	  return;
  }
//	  Serial.println("1 char : " + payload.substring(0,1) );
  if (payload.substring(0,1) == "#") {
	  activeColor = payload;
	  SaveSetupValues(activeColor);
  	  colorRGB(activeColor);
  	  PublishLightStatus();
  	  Serial.println("New Color " + activeColor );
  	  return;
  }
  if (payload.substring(0,6) == "PubDel") {
	  int splitChar = payload.indexOf('=');
	  pubDelay = payload.substring(splitChar+1).c_str();
  	  Serial.println("New PubDelay " + pubDelay );
	  SaveSetupValues(activeColor);
  	  return;
  }
}




//****************************************************************************
// Turn LED to Received color
//****************************************************************************
void colorRGB(String HexValue){

  Serial.print("colorRGB ");
  Serial.println(HexValue);

  long number = (long) strtol( &HexValue[1], NULL, 16);
  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;


  analogWrite(ChannelRed,constrain(r,0,255));
  analogWrite(ChannelGreen,constrain(g,0,255));
  analogWrite(ChannelBlue,constrain(b,0,255));
}



//********************************************************************************
// Display controler status
//********************************************************************************
void DisplayStatus() {
	String IP = WiFi.localIP().toString();
	String DebugStr = "false";

	//   Add SavedColor, BlinkDelay, PubDelay    *************%$&^$%^$&^$&^%$&^$&^%$&^%$&^$^&%$&^%

	if ( debug ) { DebugStr= "true"; }
	String ControlerStatus =
			           "{\n\"SSID\" : \"" + wifi_ssid +
	                   "\",\n\"Network connection\" : \"Established" +
	                   "\",\n\"Pooling Time\" : " + pubDelay +
	                   "\",\n\"Blink Delay\" : " + blinkDelay +
	                   ",\n\"Access Point IP\" : \"" + APssid +
					   "\",\n\"Controler IP\" : \"" + IP +
	                   "\",\n\"MQTT ID\" : \"" + MQTT_id +
	                   "\",\n\"MQTT Server\" : \"" + mqtt_server +
	                   "\",\n\"MQTT Port\" : \"" + mqtt_port +
	                   "\",\n\"MQTT Status\" : \"" + mqtt_status +
	                   "\",\n\"Debug\" : " + DebugStr +
	                   ",\n\"Subscribed Topics\" : [\n" ;
	                   for (int i=0; i < nb_subscribe_topics; i++) {
	                	   ControlerStatus = ControlerStatus + " \"" + subscribe_topics[i] + "\"" ;
	                	   if (i < nb_subscribe_topics-1) { ControlerStatus = ControlerStatus + ","; }
	                	   	   ControlerStatus = ControlerStatus + "\n";
	                   	   }
	                   ControlerStatus = ControlerStatus + " ]" + "\n} " ;

	  Serial.println (ControlerStatus.c_str());
	  client.publish(MQTT_id.c_str(),ControlerStatus.c_str());
}
