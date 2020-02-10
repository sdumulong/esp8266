#include <FS.h>  // Include the SPIFFS library
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define     SERIAL_BAUDRATE	115200
bool    debug           = true; //Affiche sur la console si True

const char* APssid   = "BedlightV3";
const char* password = "12345678";

IPAddress local_IP(192,168,4,3);
IPAddress gateway(192,168,4,255);
IPAddress subnet(255,255,255,0);
ESP8266WebServer server(80); //Server on port 80

bool        modeAccessPoint = false;



//===================================================================================
// Setup procedure
//===================================================================================
void setup() {
	Serial.begin(SERIAL_BAUDRATE);   //Facultatif pour le debug
	delay(10);
	Serial.println('\n');

	StaticJsonDocument<200> doc;
	doc["sensor"] = "gpsfff";
	doc["time"] = 1351824120;
	serializeJson(doc, Serial);

	SPIFFS.begin();
	if (!SPIFFS.exists("/config.json")) {
	    startAccessPoint();
	}
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

   Serial.println("Reception of Data");
   server.send(200, "<! DOCTYPE html>", "<html><body>Saved</body><html>");
   //resetFunc();
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


//==================================================================================
//  Main Loop
//==================================================================================
void loop() {
	server.handleClient();
	//Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());
	//delay(3000);
}

