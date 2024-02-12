#include <Wire.h>
#include <TinyGPS++.h>
#include <WiFi.h>

#include <U8g2lib.h>
#include <U8x8lib.h>

#define BLYNK_TEMPLATE_ID "<INSERT_YOUR_TEMPLATE_ID_HERE>"         // Please change this according to your BLYNK settings
#define BLYNK_TEMPLATE_NAME "<INSERT_A_TEMPLATE_NAME_HERE>"        // Please change this according to your BLYNK settings
#define BLYNK_AUTH_TOKEN "<INSERT_YOUR_BLYNK_TOKEN_HERE>"          // Please change this according to your BLYNK settings
#include <BlynkSimpleEsp32.h>

const char *networkSSID = "<INSERT_YOUR_NETWORK_SSID_HERE>";       // Please change this according to your Wifi settings
const char *networkPASS = "<INSERT_YOUR_NETWORK_PASSWORD_HERE>";   // Please change this according to your Wifi settings

unsigned long startLoopTime;
unsigned int sendPeriod = 30000;

double gpsLat, gpsLon, gps1Lat, gps1Long;
unsigned int gpsSatNumber;
char charGPSSatNumber[4] = {0};
char charLatitude[12] = {0};
char charLongitude[12] = {0};

// The TinyGPSPlus object
#define RXD2 17
#define TXD2 18
HardwareSerial ser2(2);
TinyGPSPlus gps;

// U8G2 parameter for I2C SH1106 OLED Display module
U8G2_SH1106_128X64_NONAME_2_HW_I2C oledDisplay(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud rate
  ser2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  WiFi.begin(networkSSID, networkPASS); // Initiate WiFi connection

  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // Wait for connection
  }

  Serial.println(F("Connected to WiFi"));

  oledDisplay.begin();

  Blynk.begin(BLYNK_AUTH_TOKEN, networkSSID, networkPASS);
  Blynk.virtualWrite(V5, "clr");

  startLoopTime = millis();
}

void loop() {
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ser2.available())
    {
      if (gps.encode(ser2.read()))
      {
        newData = true;
      }
    }
  }

  if(newData == true){
    newData = false;
    Serial.println("New Data");
    gpsSatNumber = gps.satellites.value();
    dtostrf(gpsSatNumber, 3, 0, charGPSSatNumber);
    Serial.println(gpsSatNumber);
    if (gps.location.isValid() == 1){
      gpsLat = gps.location.lat();
      dtostrf(gpsLat,10,6,charLatitude);
      gpsLon = gps.location.lng();
      dtostrf(gpsLon,10,6,charLongitude);

      printToConsole();
      printToOledDisplay();
      sendToBlynk();
    }
    else {
      Serial.println("Finding satellites ... ");
    }
  }
  else
  {
    Serial.println("No Data");
  }
  delay(5000);
}

void printToConsole()
{
  Serial.print("Detected Satellites:");
  Serial.println(gpsSatNumber);
  Serial.print("Lat: ");
  Serial.print(gpsLat, 6);
  Serial.print(", Lon: ");
  Serial.println(gpsLon, 6);
}

void printToOledDisplay()
{
  oledDisplay.firstPage();
  do
  {
    oledDisplay.clearBuffer();					// clear the internal memory
    oledDisplay.drawRFrame(0, 0, 127, 63, 3);
    oledDisplay.drawLine(0, 20, 127, 20);
    oledDisplay.setFont(u8g2_font_8x13_tr);
    oledDisplay.drawStr(4,15, "GPS Position");
    oledDisplay.drawStr(100, 15, charGPSSatNumber);
    //oledDisplay.drawStr(16,15, "GPS Position");
    oledDisplay.drawStr(4, 37, "Lat:");
    oledDisplay.drawStr(42,37, charLatitude);
    oledDisplay.drawStr(4, 55, "Lng:");
    oledDisplay.drawStr(42, 55, charLongitude);
    oledDisplay.sendBuffer();					// transfer internal memory to the display
  } while ( oledDisplay.nextPage() );
}

void sendToBlynk()
{
  if(millis() - startLoopTime > sendPeriod){
    Serial.print("Data sent to Blynk!");
    Blynk.virtualWrite(V5, gps.location.lng(), gps.location.lat());
    //Blynk.virtualWrite(V5, 144.955645, -37.813250);  // Melbourne Australia
    //Blynk.virtualWrite(V5, 106.762380, -6.186627); // Kedoya Selatan, Jakarta, Indonesia
    startLoopTime = millis();
  }
}

void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}
