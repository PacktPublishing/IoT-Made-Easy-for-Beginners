#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <FastLED.h>

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/topic"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/topic"

// We define IR Sensor pin
// Note : ESP32 cannot use ADC2 since it is used by Wifi, so use ADC1 (GPIO 12-14, 25-27, 32-36, 39)
const int irSensorPin = 32;
const int buzzerPin = 33;
int irSensorValue;
bool isObstacleDetected;
unsigned long startTime, currentTime;

// We define LED Strip pin and length (number of LEDs)
const int ledStripPin = 25;
const int ledLength = 30;
CRGB leds[ledLength];

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);


void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["Obstacle_detected"] = String(isObstacleDetected ? "true" : "false");
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  int publishResult = client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.print("A message published: Result = ");
  Serial.print(publishResult);
  Serial.print(", payload = ");
  Serial.println(jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
}

void setup() {
  Serial.begin(115200);
  connectAWS();

  FastLED.addLeds<WS2812, ledStripPin, GRB>(leds, ledLength);
  FastLED.setBrightness(50);
  startTime = millis();
}

void loop() {
  client.loop();
  if (!client.connected()) {
    connectAWS();
  }

  irSensorValue = analogRead(irSensorPin);
  isObstacleDetected = irSensorValue < 1000;

  currentTime = millis();
  if(currentTime - startTime > 5000){
    publishMessage();
  Serial.print(", irSensorValue = ");
  Serial.println(irSensorValue);

  if (isObstacleDetected) {
    Serial.println("Buzzer play tone");
    playTone(buzzerPin, 1000, 200);
    animateLEDs();
  } else {
    resetLEDs();
  }

    startTime = currentTime;
  }

  delay(1000);
}

void playTone(int pin, int frequency, int duration) {
  tone(pin, frequency, duration);
  delay(duration);
}

void animateLEDs() {
  for (int i = 0; i < ledLength; i++) {
    leds[i] = CHSV(i * 10, 255, 255);
    FastLED.show();
    delay(20);
  }
}

void resetLEDs() {
  for (int i = 0; i < ledLength; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}
