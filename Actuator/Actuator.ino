/*
Last Update
*   Dimas - 9 Apr - 06.30
*       - Subscribe msg
*       - Blink LED sesuai msg
*       - [DEBUG] Publish msg + "go"
*/

#include "WiFi.h"
#include <PubSubClient.h>
#include <stdlib.h>

// Update these with values suitable for your network.
const char* ssid = "Wifi";
const char* password = "12345678";
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
bool messagePublished = false;


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-Actuator-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("ESP32_Insulin_Pump_topic", "Actuator Online");
      // ... and resubscribe
      client.subscribe("ESP32_Insulin_Pump_topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


//---------------------------------- Terminal Rapih ----------------------------------------
// bool ledBlinked = false;

// void callback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("Message arrived on topic: ");
//   Serial.print(topic);
//   for (int i = 0; i < length; i++) {
//     msg[i] = (char)payload[i];
//     Serial.print(msg[i]);
//   }
//   Serial.println();

//   if (ledBlinked) {
//     // If the LED has already been blinked, do nothing
//     return;
//   } else if (strcmp((char*)payload, "Done") == 0) {
//     // If the received message is "Done", reset the flag variable
//     ledBlinked = false;
//     digitalWrite(BUILTIN_LED, HIGH);
//   } else {
//     // Convert received message from char to integer
//     int blink = atoi((char*)payload);
    
//     // Blink the LED as many times as the received integer
//     for (int i = 0; i < blink; i++) {
//       digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on
//       delay(1000);
//       digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED off
//       delay(1000);
//       digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on
//     }

//     // Set the flag variable to true to indicate that the LED has been blinked
//     ledBlinked = true;

//     // Publish "Done" back to the MQTT broker
//     client.publish("ESP32_Insulin_Pump_topic", "Done");
//   }
// }

//------------------------------------ Works better -----------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
    Serial.print(msg[i]);
  }
  Serial.println();

  // Convert received message from char to integer
  int blink = atoi((char*)payload);

  // Blink the LED as many times as the received integer
  for (int i = 0; i < blink; i++) {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED off
    delay(1000);
    digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED on
    delay(1000);
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED off
  }

  client.publish("ESP32_Insulin_Pump_topic", "Done");
}

//------------------------------------------------------------------------------

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}
