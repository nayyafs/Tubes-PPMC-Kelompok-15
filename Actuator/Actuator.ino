/*
Last Update
*   Dimas - 9 Apr - 06.30
*       - Subscribe msg
*       - Blink LED sesuai msg
*       - [DEBUG] Publish msg + "go"
*/

#include "WiFi.h"
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "Dimasrifky";
const char* password = "dinanfamily";
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
bool messagePublished = false;


// //---------------------DEKRIPSI------------------------------
// // Define the encryption key
// char encryptionKey[] = "secret_key";
// // Encryption function using XOR operation
// void encryptMessage(char* message) {
//   int messageLength = strlen(message);
//   int keyLength = strlen(encryptionKey);
//   for (int i = 0; i < messageLength; i++) {
//     message[i] = message[i] ^ encryptionKey[i % keyLength];
//   }
// }

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
    Serial.print(msg[i]);
  }
  Serial.println();

  //--------------------Bagian Publish dan Subscribe----------------------

  // Encrypt the received message
  // encryptMessage(msg);

  // Check if the message is "Nyalain LED dongs"
  if (strcmp((char*)payload, "Enkripnibos") == 0) {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off
  }

  strcat(msg, " Received");

  if (!messagePublished) {
      client.publish("ESP32_Try_topic", msg);
      messagePublished = true;
    }
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
      client.publish("ESP32_Try_topic", "Actuator Online");
      // ... and resubscribe
      client.subscribe("ESP32_Try_topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
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
