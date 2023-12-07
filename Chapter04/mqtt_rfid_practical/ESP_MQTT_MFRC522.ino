/*
This example uses FreeRTOS softwaretimers as there is no built-in Ticker library
*/


#include <WiFi.h>
extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>
#include <Wire.h>

#define SWITCH_PIN 32
#define LED_PIN 2

#define WIFI_SSID "<INSERT_WIFI_SSID_HERE>"
#define WIFI_PASSWORD "<INSERT_WIFI_PASSWORD_HERE>"
//#define WIFI_SSID "yourSSID"
//#define WIFI_PASSWORD "yourpass"

#define MQTT_HOST         "broker.hivemq.com"        // Broker address
//#define MQTT_HOST IPAddress(192, 168, 1, 10)
#define MQTT_PORT 1883
#define MQTT_SUB_Output "mrtg/card_id"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

// Notes : MFRC522 SPI connection to ESP32 : SCK 18, MISO 19, MOSI 23, CS 5, RST 27
#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN         27           // Configurable, see typical pin layout above
#define SS_PIN          5          // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;

long lastMsg = 0;
char msg[50];
char registeredID[100][17];
int totalRegisteredID = 0;
int readSwitchValue = 0;
int previousReadSwitchValue = 0;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        xTimerStart(wifiReconnectTimer, 0);
        break;
    }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_SUB_Output, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
  /*
  mqttClient.publish(MQTT_SUB_Output, 0, true, "test 1");
  Serial.println("Publishing at QoS 0");
  uint16_t packetIdPub1 = mqttClient.publish(MQTT_SUB_Output, 1, true, "test 2");
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdPub1);
  uint16_t packetIdPub2 = mqttClient.publish(MQTT_SUB_Output, 2, true, "test 3");
  Serial.print("Publishing at QoS 2, packetId: ");
  Serial.println(packetIdPub2);
  */
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  /*
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
  */
  String messageTemp;
  for (int i = 0; i < len; i++) {
    messageTemp += (char)payload[i];
  }
  Serial.print("Message : ");
  Serial.println(messageTemp);
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

  // Init MFRC522 using SPI
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
  }

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
}

void loop() {
  readSwitchValue = digitalRead(SWITCH_PIN);
  //Serial.print("\nreadSwitchValue = ");
  //Serial.println(readSwitchValue);
  if(readSwitchValue != previousReadSwitchValue){
      if(readSwitchValue == 0){
        //Serial.println("Switch is Pressed\n");
        digitalWrite(LED_PIN, HIGH);
        previousReadSwitchValue = readSwitchValue;
      }
      else{
        //Serial.println("Switch is Not Pressed\n");
        digitalWrite(LED_PIN, LOW);
        previousReadSwitchValue = readSwitchValue;
      }
  }

  // MFRC522
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent())
      return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
      return;

  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  byte *buffer1 = mfrc522.uid.uidByte;
  byte bufferSize1 = mfrc522.uid.size;
  Serial.print("***** Buffer size 1 = ");
  Serial.println(bufferSize1);
  //dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  dump_byte_array(buffer1, bufferSize1);
  //Serial.print("mfrc522.uid.size = ");
  //Serial.println(mfrc522.uid.size);

  // ***********
  //byte array[4] = {0xAB, 0xCD, 0xEF, 0x99};
  char strResult1[17];
  //char strResult1[bufferSize1+1];
  array_to_string(buffer1, bufferSize1, strResult1);
  strResult1[17] = '\0';
  // ***********

  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // Check for compatibility
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
      &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("This sample only works with MIFARE Classic cards."));
      return;
  }

  // In this sample we use the second sector,
  // that is: sector #1, covering block #4 up to and including block #7
  byte sector         = 1;
  byte blockAddr4      = 4;
  byte blockAddr5      = 5;
  byte blockAddr6      = 6;
  byte blockAddr7      = 7;

  byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  // Authenticate using key A
  Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
  }

  // Show the whole sector as it currently is
  Serial.println(F("Current data in sector:"));
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();


  // Read data from the block
  Serial.print(F("Reading data from block ")); Serial.print(blockAddr4);
  Serial.println(F(" ..."));
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr4, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data in block ")); Serial.print(blockAddr4); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();

  // Read data from the block
  Serial.print(F("Reading data from block ")); Serial.print(blockAddr5);
  Serial.println(F(" ..."));
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr5, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data in block ")); Serial.print(blockAddr5); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();

  byte count = 0;

  if(readSwitchValue == 0){ // Switch is pressed, this is cardID register mode

    // Check if cardID already registered
    bool found = 0;
    for(int i = 0; i < totalRegisteredID; i++){
      if(strcmp(strResult1, registeredID[i]) == 0){
        found = 1;
        break;
      }
    }
    if(!found){ // cardID not registered
      byte dataBlock4[]    = {
          0x70, 0x71, 0x72, 0x73, //  1,  2,   3,  4,
          0x74, 0x75, 0x76, 0x77, //  5,  6,   7,  8,
          0x78, 0x79, 0x7a, 0x7b, //  9, 10, 255, 11,
          0x7c, 0x7d, 0x7e, 0x7f,  // 12, 13, 14, 15
          0x00, 0x01, 0x02, 0x03, //  1,  2,   3,  4,
          0x04, 0x05, 0x06, 0x07, //  5,  6,   7,  8,
          0x08, 0x09, 0x0a, 0x0b, //  9, 10, 255, 11,
          0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
      };
      byte blockAddr5      = 5;
      byte dataBlock5[]    = {
          0x00, 0x01, 0x02, 0x03, //  1,  2,   3,  4,
          0x04, 0x05, 0x06, 0x07, //  5,  6,   7,  8,
          0x08, 0x09, 0x0a, 0x0b, //  9, 10, 255, 11,
          0x0c, 0x0d, 0x0e, 0x0f,  // 12, 13, 14, 15
          0x70, 0x71, 0x72, 0x73, //  1,  2,   3,  4,
          0x74, 0x75, 0x76, 0x77, //  5,  6,   7,  8,
          0x78, 0x79, 0x7a, 0x7b, //  9, 10, 255, 11,
          0x7c, 0x7d, 0x7e, 0x7f  // 12, 13, 14, 15
      };

      // Write data to the block4
      Serial.print(F("Writing data into block ")); Serial.print(blockAddr4);
      Serial.println(F(" ..."));
      dump_byte_array(dataBlock4, 16); Serial.println();
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr4, dataBlock4, 16);
      //status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr4, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Write() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
      }
      Serial.println();


      // Write data to the block5
      Serial.print(F("Writing data into block ")); Serial.print(blockAddr5);
      Serial.println(F(" ..."));
      dump_byte_array(dataBlock5, 16); Serial.println();
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr5, dataBlock5, 16);
      if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Write() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
      }
      Serial.println();

      count = 0;
      // Read data from the block4 (again, should now be what we have written)
      Serial.print(F("Reading data from block 4 ")); Serial.print(blockAddr4);
      Serial.println(F(" ..."));
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr4, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Read() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
      }
      Serial.print(F("Data in block 4 ")); Serial.print(blockAddr4); Serial.println(F(":"));
      dump_byte_array(buffer, 16); Serial.println();

      // Check that data in block is what we have written
      // by counting the number of bytes that are equal
      Serial.println(F("Checking result..."));
      for (byte i = 0; i < 16; i++) {
          // Compare buffer (= what we've read) with dataBlock (= what we've written)
          if (buffer[i] == dataBlock4[i])
              count++;
      }

      // Read data from the block5 (again, should now be what we have written)
      Serial.print(F("Reading data from block ")); Serial.print(blockAddr5);
      Serial.println(F(" ..."));
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr5, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Read() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
      }
      Serial.print(F("Data in block ")); Serial.print(blockAddr5); Serial.println(F(":"));
      dump_byte_array(buffer, 16); Serial.println();

      // Check that data in block is what we have written
      // by counting the number of bytes that are equal
      Serial.println(F("Checking result..."));

      for (byte i = 0; i < 16; i++) {
          // Compare buffer (= what we've read) with dataBlock (= what we've written)
          if (buffer[i] == dataBlock5[i])
              count++;
      }

      //Serial.print(F("Number of bytes that match = ")); Serial.println(count);
      if (count == 32) {
          // Add cardID into the registeredID array
          for(int i = 0; i<5; i++){
            strcpy(&registeredID[totalRegisteredID][i], &strResult1[i]);
          }
          totalRegisteredID = totalRegisteredID + 1;
          // published result and blink LED
          char outString[60] = "New card registered, Card ID = ";
          strcat(outString, strResult1);
          mqttClient.publish("mrtg/card_id", 0, true, outString);
          blink_led(3, 300);
      }
      else {
          char outString[60] = "FAILURE, verify data written not match, Card ID = ";
          strcat(outString, strResult1);
          mqttClient.publish("mrtg/card_id", 0, true, outString);
          blink_led(10, 100);
      }
      //Serial.println();

      // Dump the sector data
      Serial.println(F("Current data in sector:"));
      mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
      Serial.println();
    }
    else {  // cardID already registered
      char outString[60] = "Already Registered, Card ID = ";
      strcat(outString, strResult1);
      mqttClient.publish("mrtg/card_id", 0, true, outString);
      blink_led(5, 200);
    }
  }
  else {
    // Compare the cardID with existing registeredID
    bool found = 0;
    for(int i = 0; i < totalRegisteredID; i++){
      if(strcmp(strResult1, registeredID[i]) == 0){
        found = 1;
        break;
      }
    }

    if(found){    // success, card id already registered, open the door
      char outString[60] = "SUCCESS, opening the door, Card ID = ";
      strcat(outString, strResult1);
      mqttClient.publish("mrtg/card_id", 0, true, outString);
      blink_led(1, 1000);
    }
    else {        // card id not registered
      char outString[60] = "WARNING!!! Unregistered Card ID =  ";
      strcat(outString, strResult1);
      mqttClient.publish("mrtg/card_id", 0, true, outString);
      blink_led(10, 100);
    }
  }

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
 void blink_led(int times, int duration){
    for(int i = 0; i < times; i++){
        digitalWrite(LED_PIN, HIGH);
        delay(duration/2);
        digitalWrite(LED_PIN, LOW);
        delay(duration/2);
    }
 }
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void array_to_string(byte array[], byte bufferSize, char buffer[])
{
    for (unsigned int i = 0; i < bufferSize; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
    }
    buffer[bufferSize*2] = '\0';
}
