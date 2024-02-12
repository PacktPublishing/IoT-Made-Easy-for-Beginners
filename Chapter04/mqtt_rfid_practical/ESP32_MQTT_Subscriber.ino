#include <WiFi.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include <AsyncMqtt_Generic.h>

#define MQTT_HOST         "broker.hivemq.com"        // Broker address
//#define MQTT_HOST         "broker.emqx.io"         // Another broker address
#define MQTT_PORT         1883
#define WIFI_SSID "My_Wifi_SSID"
#define WIFI_PASSWORD "My-Wifi-Password:IOT"
#define RELAY_PIN1        32
#define RELAY_PIN2        33
#define LED_PIN           2

unsigned long relayStartTime = 0;
int relayDelayTime = 5000;
bool relayStatus = false;

const char *PubTopic  = "mrtg/card_id";               // Topic to publish
char payloadResult[128];
char payload_keyword[8];

char keyword[8] = "SUCCESS";
int keyword_len = 7; 

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
#if USING_CORE_ESP32_CORE_V200_PLUS

    case ARDUINO_EVENT_WIFI_READY:
      Serial.println("WiFi ready");
      break;

    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi STA starting");
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi STA connected");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("WiFi lost IP");
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
#else

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
#endif

    default:
      break;
  }
}

void printSeparationLine()
{
  Serial.println("************************************************");
}

void onMqttConnect(bool sessionPresent)
{
  Serial.print("Connected to MQTT broker: ");
  Serial.print(MQTT_HOST);
  Serial.print(", port: ");
  Serial.println(MQTT_PORT);
  Serial.print("PubTopic: ");
  Serial.println(PubTopic);

  printSeparationLine();
  Serial.print("Session present: ");
  Serial.println(sessionPresent);

  mqttClient.publish(PubTopic, 0, true, "Publish Test");
  Serial.println("Publishing at QoS 0");

  uint16_t packetIdSub = mqttClient.subscribe(PubTopic, 2);
  
  printSeparationLine();  
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  (void) reason;

  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total)
{
  (void) payload;
  
  int i = 0;
  for (i = 0; i < len; i++){
      payloadResult[i] = payload[i];
  }
  payloadResult[i] == '\0';
  Serial.println(payloadResult);

  for(i = 0; i < keyword_len; i++){
      payload_keyword[i] = payload[i];
  }
  payload_keyword[i] == '\0';

  if(strcmp(payload_keyword, "Publish") != 0){
      if(strcmp(payload_keyword, keyword) == 0){
          // Start activating the relay
          Serial.println(". Access GRANTED!!!");
          relayStartTime = millis();      // record the current time
      }
      else{
          Serial.println(". Access NOT Granted!!!");
      }
  }
}

void onMqttPublish(const uint16_t& packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY_PIN1, OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN1, HIGH);
  digitalWrite(RELAY_PIN2, HIGH);
  digitalWrite(LED_PIN, LOW);

  while (!Serial && millis() < 5000);

  Serial.print("\nStarting FullyFeature_ESP32 on ");
  Serial.println(ARDUINO_BOARD);
  Serial.println(ASYNC_MQTT_GENERIC_VERSION);

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
}

void loop()
{
    if(relayStartTime != 0){
        if(millis() < relayStartTime + relayDelayTime){
            if(relayStatus == false){
                relayStatus = true;
                digitalWrite(RELAY_PIN1, LOW);
                digitalWrite(LED_PIN, HIGH);
                Serial.println("Relay1 Turn ON!");
            }
        }
        else{
          if(relayStatus == true){
              relayStatus = false;
              relayStartTime = 0;
              digitalWrite(RELAY_PIN1, HIGH);
              digitalWrite(LED_PIN, LOW);
              Serial.println("Relay1 Turn OFF!");
          }
        }
    }
}
