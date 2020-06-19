#include <SPI.h>
#include <DHT.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define  SERIAL_BAUDRATE	115200
char     message_buff[100];
unsigned long lastMsg    	 = 0   ;       //Horodatage du dernier message publié sur MQTT
int      poolingTime     	 = 30  ;       //Pooling time between each scan
bool     debug               = true;       //Affiche sur la console si True
String   MQTT_Name           = "UNOGarage";
const char* mqtt_server  = "192.168.0.200";

int      nb_subscribe_topics = 4;
String   subscribe_topics[]  = { "Garage/Pooling/Set",
                                 "Garage/Debug/Set",
                                 "Garage/Status",
                                 "Garage/Front/Door/Set"};

// Update these with values suitable for your network.
byte mac[] = {  0xDE, 0xED, 0xBD, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 151);

// Pin definition
int Motion_Sensor_Pin        = 4;
int BackDoor_Sensor_Pin      = 5;
int FrontDoor_Sensor_Pin     = 6;
int FrontDoorLock_Sensor_Pin = 7;
int OpenDoor_Led_Pin         = 8;
int FrontDoor_Switch_Pin     = 9;

// Door Status flag
boolean FrontdoorStatus;
boolean FrontdoorLockStatus;
boolean BackdoorStatus;
boolean MotionSensor;
boolean LastFrontdoorStatus;
boolean LastFrontdoorLockStatus;
boolean LastBackdoorStatus;
boolean LastMotionSensor;

// DHT Definition
#define DHTPIN  2
#define DHTTYPE DHT22

//Création des objets
DHT dht(DHTPIN, DHTTYPE);
EthernetClient ethClient;
PubSubClient   client(ethClient);


//********************************************************************************
// Setup  (Initialisation du programme)
//********************************************************************************
void setup(){

  Serial.begin(SERIAL_BAUDRATE);

  pinMode(FrontDoor_Sensor_Pin, INPUT);     // declare Door_Sensor_Pin as input
  pinMode(FrontDoorLock_Sensor_Pin, INPUT); // declare Door_Sensor_Pin as input
  pinMode(BackDoor_Sensor_Pin, INPUT);      // declare Door_Sensor_Pin as input
  pinMode(FrontDoor_Switch_Pin, OUTPUT);    // declare Door_Sensor_Pin as input
  pinMode(OpenDoor_Led_Pin, OUTPUT);        // declare led pin for output
  pinMode(Motion_Sensor_Pin, INPUT);        // declare pin for motion

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Ethernet.begin(mac, ip);                  // Start network connection
  dht.begin();                              // Initialise temperature Sensor
}



//****************************************************************************
void loop() {
//****************************************************************************
unsigned long now = millis();

	if (!client.connected()) { reconnect(); }  // Connect to MQTT

	client.loop();
	CheckDoorStatus();
	PublishMotionSensor();

	if (now - lastMsg > (1000 * poolingTime)) { lastMsg = now;
		PublishTemperature();
	    PublishDoorStatus();
	  }
}



//********************************************************************************
// Check Door Status if changed
//********************************************************************************
void CheckDoorStatus() {
	FrontdoorStatus      = digitalRead(FrontDoor_Sensor_Pin);
	FrontdoorLockStatus  = digitalRead(FrontDoorLock_Sensor_Pin);
	BackdoorStatus       = digitalRead(BackDoor_Sensor_Pin);

	if (FrontdoorStatus     != LastFrontdoorStatus     or
		FrontdoorLockStatus != LastFrontdoorLockStatus or
		BackdoorStatus      != LastBackdoorStatus  )    {

			PublishDoorStatus();
			LastFrontdoorStatus     = FrontdoorStatus;
			LastFrontdoorLockStatus = FrontdoorLockStatus;
			LastBackdoorStatus      = BackdoorStatus;
	}
}



//********************************************************************************
// Check for motion sensor
//********************************************************************************
void PublishMotionSensor() {

	MotionSensor = digitalRead(Motion_Sensor_Pin);
	if (MotionSensor != LastMotionSensor) {
		if (MotionSensor == HIGH) {
	      client.publish("Garage/Motion", "Motion Detected");
	    } else {
	      client.publish("Garage/Motion", "No Motion Detected");
	    }
	    LastMotionSensor = MotionSensor;
	}
}



//********************************************************************************
// Publish Door Status
//********************************************************************************
void PublishDoorStatus() {
    if (FrontdoorStatus == LOW)      { client.publish("Garage/Front/Door","Close"); }
    if (FrontdoorStatus == HIGH)     { client.publish("Garage/Front/Door","Open") ; }
    if (BackdoorStatus  == LOW)      { client.publish("Garage/Back/Door" ,"Close"); }
    if (BackdoorStatus  == HIGH)     { client.publish("Garage/Back/Door" ,"Open") ; }
    if (FrontdoorLockStatus == HIGH) { client.publish("Garage/Front/Door/Lock","Unlock"); }
    if (FrontdoorLockStatus == LOW)  { client.publish("Garage/Front/Door/Lock","Lock")  ; }

    if (FrontdoorStatus or BackdoorStatus){digitalWrite(OpenDoor_Led_Pin, HIGH);} else {digitalWrite(OpenDoor_Led_Pin, LOW);}
}


//********************************************************************************
// Check for incomming messages
//********************************************************************************
void callback(char* topic, byte* payload, unsigned int length) {
int  i;

  for(i = 0; i<length; i++) { message_buff[i] = payload[i]; }
  message_buff[i] = '\0';
  String msgString = String(message_buff);
  TraiterPayload(topic, msgString);
}



//********************************************************************************
//Connexion or Reconnexion
//********************************************************************************
void reconnect() {   //Boucle jusqu'à obtenur une reconnexion

  while (!client.connected()) {
    Serial.print("MQTT Connecxion...");

    if (client.connect(MQTT_Name.c_str())) {
      Serial.println("OK");
      for (int i=0; i<nb_subscribe_topics; i++) {
        client.subscribe(subscribe_topics[i].c_str());
        if (debug) { Serial.println("Subscribing to : " +  subscribe_topics[i]); }
      }
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println(" wait 5 sec.");
      delay(5000);
    }
  }
}



//****************************************************************************
void ReceivedCmd(String Command, String topic ) {
//****************************************************************************

    if (topic == "Garage/Front/Door/Set") {
      if (FrontdoorStatus == LOW and Command=="Open") {
          client.publish("Garage/Front/Door","Openning");
          digitalWrite(FrontDoor_Switch_Pin, HIGH);
          delay(500);
          digitalWrite(FrontDoor_Switch_Pin, LOW);
      }
      if (FrontdoorStatus == HIGH and Command=="Close") {
          client.publish("Garage/Front/Door","Closing");
          digitalWrite(FrontDoor_Switch_Pin, HIGH);
          delay(1000);
          digitalWrite(FrontDoor_Switch_Pin, LOW);
      }
    }
}



//********************************************************************************
// Traiter message recu
//********************************************************************************
void TraiterPayload(String topic, String payload) {

  if (topic == subscribe_topics[0]) { poolingTime = payload.toInt(); }
  if (topic == subscribe_topics[1]) { if (payload == "true") {debug = true;}else{debug = false;}}
  if (topic == subscribe_topics[2]) { DisplayStatus(); }
  if (topic == subscribe_topics[3]) { SwitchFrontDoor(payload); }

  if ( debug ) { Serial.println("Topic recu => " + topic + "     Payload: " + payload); }
}



//****************************************************************************
// Publish Temperature
//****************************************************************************
void PublishTemperature() {
	  String  pubTemp  = "";
	  String  pubTempR = "";
	  String  pubHumid = "";

	  float h = dht.readHumidity();
	  float t = dht.readTemperature();

	    if (isnan(h) || isnan(t)) {
	      Serial.println("Failed to read from DHT sensor!");
	      return;
	    }
	    float hic = dht.computeHeatIndex(t, h, false);

	    pubHumid += constrain(h,0,50) ;
	    pubTemp  += constrain(t,0,50) ;
	    pubTempR += constrain(hic,0,50) ;

	    client.publish("Garage/Humidity",pubHumid.c_str());
	    client.publish("Garage/Temperature",pubTemp.c_str());
	    client.publish("Garage/Temperature2", pubTempR.c_str());
}



//********************************************************************************
// Open or Close Front Door
//********************************************************************************
void SwitchFrontDoor(String payload) {

	if (FrontdoorStatus == LOW and payload=="Open") {
		client.publish("Garage/Front/Door","Openning");
        digitalWrite(FrontDoor_Switch_Pin, HIGH);
        delay(500);
        digitalWrite(FrontDoor_Switch_Pin, LOW);
      }
      if (FrontdoorStatus == HIGH and payload=="Close") {
        client.publish("/Garage/Front/Door","Closing");
        digitalWrite(FrontDoor_Switch_Pin, HIGH);
        delay(1000);
        digitalWrite(FrontDoor_Switch_Pin, LOW);
      }
}



//********************************************************************************
// Display controler status
//********************************************************************************
void DisplayStatus() {
String DebugStr = "false";

if ( debug ) { DebugStr= "true"; }
  String ControlerStatus = "\n{\n'Controler Name' : '"+ MQTT_Name +"',\n'Debug' : '"+DebugStr+"',\n'SSID' :'"+"wifi_ssid"+"',\n'Controler IP': '"+"IP"+"',\n'Network connection' : 'Established',\n'MQTT Server' : '"+mqtt_server+"',\n'MQTT Connection' : 'Established',\n'Pooling Device' : '"+poolingTime+" Sec',\n'Subscribed Topics' : ['"+subscribe_topics[0]+"',\n                       '"+subscribe_topics[1]+"',\n                       '"+subscribe_topics[2]+"',\n                       '"+subscribe_topics[3]+"'\n                      ]\n}";

  Serial.println (ControlerStatus.c_str()) ;
  client.publish("/Device/Connection/Status", ControlerStatus.c_str());
}
