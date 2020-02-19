#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* wifi_ssid = "SDC";
const char* wifi_password = "hjk2xbm7";
const char* mqtt_server = "192.168.0.200";

char    SBUF0 = 0;
String  Buffer = "";
String  Buffer2 = "";
String  Phone = "";
char    message_buff[100];
bool    debug = true;  //Affiche sur la console si True
int     nb_subscribe_topics = 1;
String  subscribe_topics[]  = { "/House/Phone/Set"};


//Création des objets
WiFiClient espClient;
PubSubClient client(espClient);

 
void setup() {
  Serial.begin(1200);
  delay(200);

  setup_wifi();           //On se connecte au réseau wifi
  client.setServer(mqtt_server, 1883);    //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);  //La fonction de callback qui est executée à chaque réception de message
  Serial.println("Waiting Caller..");
}
 

//********************************************************************************
//Connexion au réseau WiFi
//********************************************************************************
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion a ");
  Serial.print(wifi_ssid);
  Serial.print(" ");
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ");
  Serial.println("Connexion WiFi etablie ");
  Serial.print("Addresse IP : ");
  Serial.println(WiFi.localIP());
}



//********************************************************************************
//Connexion or Reconnexion
//********************************************************************************
void reconnect() {   //Boucle jusqu'à obtenur une reconnexion
int i;

  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");

    if (client.connect("ESP8266Phone")) {
      Serial.println("OK");
      for (int i=0; i<nb_subscribe_topics; i++) {
        client.subscribe(subscribe_topics[i].c_str());
        if (debug) { Serial.println("Subscribing to topic: " +  subscribe_topics[i]); }
      }
    } else {
      Serial.print("KO, erreur : ");
      Serial.print(client.state());
      Serial.println(" On attend 5 secondes avant de recommencer");
      delay(5000);
    }
  }
}


//********************************************************************************
// Check for incomming messages
//********************************************************************************
void callback(char* topic, byte* payload, unsigned int length) {
int i = 0;
  for(i=0; i<length; i++) { message_buff[i] = payload[i]; }
  message_buff[i] = '\0';
  String msgString = String(message_buff);
  TraiterMessageRecu(topic, msgString);
}



//********************************************************************************
// Main Loop
//********************************************************************************
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  Ht9032_get();
  if (Buffer != "") { PublishPhoneCall(); }
}
 


//********************************************************************************
// Traiter message recu
//********************************************************************************
void TraiterMessageRecu(String topic, String payload) {
  if ( debug ) { Serial.println("Topic recu => " + topic + "     Payload: " + payload); }
}



//********************************************************************************
// Receiving phone call
//********************************************************************************
void Ht9032_get() {
  if (Serial.available() > 0) { delay(1000); }

  while (Serial.available() > 0) {
    SBUF0 = Serial.read(); 
    if (SBUF0 >='0' and SBUF0 <= '9') {
      Buffer = Buffer + SBUF0;
    }
  }
}


//****************************************************************************
// Publish Phone Call
//****************************************************************************
void PublishPhoneCall() {

    Buffer2 = Buffer.substring(Buffer.length()-10, Buffer.length());
    Phone = "(" + Buffer2.substring(0,3) + ")" + Buffer2.substring(3,6) + "-" + Buffer2.substring(6,10);
    if (Phone.length() > 10) {
      if (debug) { Serial.println(Phone); }
      client.publish("/House/Phone", (char*) Phone.c_str());
    }
    Buffer = "";
}

