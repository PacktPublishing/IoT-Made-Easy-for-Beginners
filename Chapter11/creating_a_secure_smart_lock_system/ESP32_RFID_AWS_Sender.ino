#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <SPI.h>
#include <MFRC522.h>
#include "certificates.h"

#define RST_PIN         27           // Configurable, see typical pin layout above
#define SS_PIN          5          // Configurable, see typical pin layout above
#define SWITCH_PIN 32
#define LOCK_PIN 33
#define LED_PIN 2
MFRC522 rfid(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
 
const char WIFI_SSID[] = "<YOUR_WIFI_SSID>";               //change this to your Wifi credentials
const char WIFI_PASSWORD[] = "<YOUR_WIFI_PASSWORD>";           //change this to your Wifi credentials
const char AWS_IOT_ENDPOINT[] = "<YOUR_AWS_IOT_ENDPOINT>";   //change this to your AWS_IOT_Endpoint
 
#define THINGNAME "ESP32_RFID"                         //change this to your thing name
#define AWS_IOT_PUB_TOPIC   "rfid/1/pub"
#define AWS_IOT_SUB_TOPIC   "rfid/1/sub"

String device_id = "1"; 
String rfidUid;
int shift = 3;

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

String getUidString(byte *buffer, byte bufferSize) {
  String uidString = "";

  for (byte i = 0; i < bufferSize; i++) {
    uidString += String(buffer[i], HEX);
  }

  return uidString;
}

//Caesar cypher to encrypt and decrypt data
String caesar_cipher_encrypt(String message, int shift){
  String result = "";
  for(int i=0; i<message.length(); i++){
    char c = ((message[i] - 32 + shift)%95) + 32;
    result += c;
  }
  return result;
}

String caesar_cipher_decrypt(String message, int shift){
  String result = "";
  for(int i=0; i<message.length(); i++){
    char c = (message[i] - shift - 32)%95 + 32;
    if(c < 32){
      c = c + 95;
    }
    result += c;
  }
  return result;
}

void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
}

void connectAWS()
{
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUB_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("THINGNAME")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("rfid/1/outTopic","hello world");
      // ... and resubscribe
      client.subscribe("AWS_IOT_SUB_TOPIC");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishMessage(String &pub_device_id, String &pub_encryptUid, String &pub_state)
{
  StaticJsonDocument<200> doc;
  doc["device_id"] = pub_device_id;
  doc["encryptUid"] = pub_encryptUid;
  doc["state"] = pub_state;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  int publishResult = client.publish(AWS_IOT_PUB_TOPIC, jsonBuffer);
  Serial.print("A message published : Result = ");
  Serial.print(publishResult);
  Serial.print(", payload = ");
  Serial.println(jsonBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message =  doc["encryptUid"];
  shift = 4;
  String decryptUid = caesar_cipher_decrypt(message, shift);
  Serial.print("Message : UID = ");
  Serial.println(decryptUid);
  if(decryptUid == rfidUid){
    digitalWrite(LOCK_PIN, 1);
    Serial.println("DOOR LOCK OPENED!");
    delay(250);
    digitalWrite(LOCK_PIN, 0);
    Serial.println("DOOR LOCK CLOSED!");
  }
}
 
void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  SPI.begin();
  connectWifi();
  connectAWS();
  rfid.PCD_Init();
  Serial.println("\nPlace an RFID tag on the reader!");
}
 
void loop()
{
  if (!client.connected()) {
    reconnect();
  }  
  client.loop();
  
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    rfidUid = getUidString(rfid.uid.uidByte, rfid.uid.size);
    Serial.print("RFID tag detected: ");
    Serial.println(rfidUid);
    shift = 3;
    String encryptedUid = caesar_cipher_encrypt(rfidUid, shift);
    Serial.print("Encrypted string : ");
    Serial.println(encryptedUid);
 
    // Publish RFID data to AWS MQTT
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
    int switchPinState = digitalRead(SWITCH_PIN);
    String state = "verify";
    if(switchPinState == 0){
      state = "register";
    }
    publishMessage(device_id, encryptedUid, state);
    delay(500);
  }
}