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
#define RELAY_PIN 33
#define LED_PIN 2
unsigned long relayStartTime = 0;
int relayDelayTime = 5000;
bool relayStatus = false;
unsigned long ledStartTime = 0;
int ledDelayTime = 500;
bool ledStatus = false;
int switchPinState;
String state = "verify";

MFRC522 rfid(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;

const char WIFI_SSID[] = "My_Wifi_SSID";               //change this to your Wifi credentials
const char WIFI_PASSWORD[] = "My-Wifi-Password:IOT";           //change this to your Wifi credentials
const char AWS_IOT_ENDPOINT[] = "<YOUR_IOT_ENDPOINT_HERE>";   //change this to your AWS_IOT_Endpoint

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
  Serial.print(decryptUid);
  if(decryptUid == rfidUid){
    Serial.print(" - ");
    Serial.println("REGISTERED!");
    relayStartTime = millis();
  }
  else{
    Serial.println("WARNING, NOT REGISTERED!!!");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, HIGH);
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

  if(relayStartTime != 0){
    if(millis() < relayStartTime + relayDelayTime){
        if(relayStatus == false){
          relayStatus = true;
          digitalWrite(RELAY_PIN, LOW);
          digitalWrite(LED_PIN, HIGH);
          Serial.println("DOOR OPEN!");
        }
    }
    else{
      if(relayStatus == true){
        relayStatus = false;
        relayStartTime = 0;
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(LED_PIN, LOW);
        Serial.println("DOOR CLOSED!");
      }
    }
  }

  if(ledStartTime != 0){
    if(millis() < ledStartTime + ledDelayTime){
        if(ledStatus == false){
          ledStatus = true;
          digitalWrite(LED_PIN, HIGH);
        }
    }
    else{
      if(ledStatus == true){
        ledStatus = false;
        ledStartTime = 0;
        digitalWrite(LED_PIN, LOW);
      }
    }
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) { // new tag is available && NUID has been read
    rfidUid = getUidString(rfid.uid.uidByte, rfid.uid.size);
    Serial.print("RFID tag detected: ");
    Serial.println(rfidUid);
    shift = 3;
    String encryptedUid = caesar_cipher_encrypt(rfidUid, shift);
    Serial.print("Encrypted string : ");
    Serial.println(encryptedUid);

    ledDelayTime = 500;
    ledStartTime = millis();
    switchPinState = digitalRead(SWITCH_PIN);
    state = "verify";
    if(switchPinState == 0){
      state = "register";
    }
    // Publish RFID data to AWS MQTT
    publishMessage(device_id, encryptedUid, state);
    rfid.PICC_HaltA(); // halt PICC
    rfid.PCD_StopCrypto1(); // stop encryption on PCD
  }
}
