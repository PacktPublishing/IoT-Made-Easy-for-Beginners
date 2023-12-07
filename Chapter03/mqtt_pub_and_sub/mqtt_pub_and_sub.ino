#include <WiFi.h>
#include <PubSubClient.h>

#define EXAMPLE_ESP_WIFI_SSID "<YOUR_WIFI_SSID_HERE>"
#define EXAMPLE_ESP_WIFI_PASS "<YOUR_WIFI_PASSWORD_HERE>"
#define MQTT_SERVER "<YOUR_MQTT_SERVER_IP_HERE>"
#define MQTT_PORT 1883

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(EXAMPLE_ESP_WIFI_SSID);

  WiFi.begin(EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("WiFi connected");
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String message = String((char *)payload);
  Serial.println(message);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe("/topic/test1");
      client.subscribe("/topic/test2");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    client.publish("/topic/test1", msg.c_str());
  }

  if (millis() - lastMsg > 15000) {
    lastMsg = millis();
    client.publish("/topic/test3", "Hello World");
  }
}
