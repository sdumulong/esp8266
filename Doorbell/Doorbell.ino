#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define wifi_ssid "SDC"
#define wifi_password "hjk2xbm7"
#define mqtt_server "192.168.0.200"

//Buffer qui permet de décoder les messages MQTT reçus
char message_buff[100];
long lastMsg = 0;   //Horodatage du dernier message publié sur MQTT
long lastRecu = 0;
bool debug = true;  //Affiche sur la console si True

int    nb_subscribe_topics = 1;
String subscribe_topics[]  = { "/House/Doorbell" };
//Création des objets
WiFiClient espClient;
PubSubClient client(espClient);




//********************************************************************************
// Setup  (Initialisation du programme)
//********************************************************************************
void setup() {
  pinMode(D4, INPUT);
  Serial.begin(9600);     //Facultatif pour le debug
  setup_wifi();           //On se connecte au réseau wifi
  client.setServer(mqtt_server, 1883);    //Configuration de la connexion au serveur MQTT
  client.setCallback(callback);  //La fonction de callback qui est executée à chaque réception de message
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
  Serial.print(" ");
  Serial.println("Connexion WiFi etablie ");
  Serial.print("=> Addresse IP : ");
  Serial.println(WiFi.localIP());
}



//********************************************************************************
//Connexion or Reconnexion
//********************************************************************************
void reconnect() {   //Boucle jusqu'à obtenur une reconnexion
int i;

  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");

    if (client.connect("ESP8266Client")) {
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
// Main Loop
//********************************************************************************
void loop() {
  if (!client.connected()) { reconnect(); }
  client.loop();

  if (D4 == HIGH) {
    Serial.println(D4);
    client.publish("/House/Doorbell", "Ring", true);
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
// Traiter message recu
//********************************************************************************
void TraiterMessageRecu(String topic, String payload) {

// Pour un changement de pooling delay
//................................................................................
  if (topic == "/Doorbell") {
    if (payload == "TestBell") {
    // make Doorbell ring
       if ( debug ) { Serial.println("Doorbell is rigning"); }
    }
  }

// Trace sur le port série
//................................................................................
//  if ( debug ) { Serial.println("Topic recu => " + topic + "     Payload: " + payload); }
}

