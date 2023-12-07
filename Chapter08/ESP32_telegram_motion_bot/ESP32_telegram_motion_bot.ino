#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

typedef struct {
    const char* network_id = "<INSERT_YOUR_NETWORK_ID_HERE>";
    const char* network_pass = "<INSERT_YOUR_NETWORK_PASSWORD_HERE>";
} NetworkCredentials;

class TelegramBot {
public:
    TelegramBot(const char* botToken, WiFiClientSecure& client) : bot(botToken, client) {}

    void sendMessage(const char* chatId, const char* msg) {
        bot.sendMessage(chatId, msg, "");
    }

private:
    UniversalTelegramBot bot;
};

NetworkCredentials networkCredentials;
const char* bot_token = "<YOUR_BOT_TOKEN_HERE>";
const char* chat_id = "<YOUR_CHAT_ID_HERE>";

WiFiClientSecure wifi_client;
TelegramBot telegramBot(bot_token, wifi_client);

constexpr int PIR_SENSOR_PIN = 27;
volatile bool movementDetected = false;

void IRAM_ATTR detectMotion() {
    movementDetected = true;
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(networkCredentials.network_id, networkCredentials.network_pass);
    wifi_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(PIR_SENSOR_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIR_SENSOR_PIN), detectMotion, RISING);

    connectWiFi();

    telegramBot.sendMessage(chat_id, "Bot activated");
}

void loop() {
    if (movementDetected) {
        telegramBot.sendMessage(chat_id, "Motion detected!");
        movementDetected = false;
    }
}
