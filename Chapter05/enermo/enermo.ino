#define ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET      "ESPAsync_WiFiManager v1.15.0"
#define ESP_ASYNC_WIFIMANAGER_VERSION_MIN             1015000

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <AsyncElegantOTA.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

uint32_t requestDelay = 30000;
String sensorName = "PZEM-004T Home";
String sensorLocation = "{ENTER_YOUR_SENSOR_ADDRESS_LOCATION_HERE}";

String voltageString, currentString, powerString, energyString, frequencyString, pfString;
unsigned long currentTime,thisTime;
const uint32_t SERIAL_SPEED{115200};

#include <PZEM004Tv30.h>
PZEM004Tv30 pzem(Serial2, 16, 17);
float voltage, current, power, energy, frequency, pf, total_voltage, total_current, total_power, total_frequency, total_pf, prev_energy, temp_energy;
int16_t loopCounter = 0;
int16_t samplingCount = 0;

String dataHandler(const String& var){
  if(var == "VOLTAGE"){
    return voltageString;
  }
  else if(var == "CURRENT"){
    return currentString;
  }
  else if(var == "POWER"){
    return powerString;
  }
 else if(var == "ENERGY"){
    return energyString;
  }
 else if(var == "FREQUENCY"){
    return frequencyString;
  }
 else if(var == "PF"){
    return pfString;
  }
}

#define LED_BUILTIN       2
#define LED_ON            HIGH
#define LED_OFF           LOW
#define FORMAT_FILESYSTEM false
#define USE_LITTLEFS    true
#define USE_SPIFFS      false
#include "FS.h"
#include <LittleFS.h>
FS* filesystem =      &LittleFS;
#define FileFS        LittleFS
#define FS_Name       "LittleFS"
#include <SPIFFSEditor.h>
#define ESP_DRD_USE_LITTLEFS    true
#define ESP_DRD_USE_SPIFFS      false
#define ESP_DRD_USE_EEPROM      false
#define FORMAT_LITTLEFS_IF_FAILED true

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

// SPIFFS-like write and delete file, better use #define CONFIG_LITTLEFS_SPIFFS_COMPAT 1

void writeFile2(fs::FS &fs, const char * path, const char * message){
    if(!fs.exists(path)){
		if (strchr(path, '/')) {
            Serial.printf("Create missing folders of: %s\r\n", path);
			char *pathStr = strdup(path);
			if (pathStr) {
				char *ptr = strchr(pathStr, '/');
				while (ptr) {
					*ptr = 0;
					fs.mkdir(pathStr);
					*ptr = '/';
					ptr = strchr(ptr+1, '/');
				}
			}
			free(pathStr);
		}
    }

    Serial.printf("Writing file to: %s\r\n", path);
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void deleteFile2(fs::FS &fs, const char * path){
    Serial.printf("Deleting file and empty folders on path: %s\r\n", path);

    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }

    char *pathStr = strdup(path);
    if (pathStr) {
        char *ptr = strrchr(pathStr, '/');
        if (ptr) {
            Serial.printf("Removing all empty folders on path: %s\r\n", path);
        }
        while (ptr) {
            *ptr = 0;
            fs.rmdir(pathStr);
            ptr = strrchr(pathStr, '/');
        }
        free(pathStr);
    }
}

void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}

#define DOUBLERESETDETECTOR_DEBUG       true  //false

#include <ESP_DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

//DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
DoubleResetDetector* drd = NULL;

// Wifi Setup
// From v1.1.0
#define MIN_AP_PASSWORD_SIZE    8
#define SSID_MAX_LEN            32
//From v1.0.10, WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN            64

typedef struct
{
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw  [PASS_MAX_LEN];
}  WiFi_Credentials;

typedef struct
{
  String wifi_ssid;
  String wifi_pw;
}  WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS      2

// Assuming max 49 chars
#define TZNAME_MAX_LEN            50
#define TIMEZONE_MAX_LEN          50

typedef struct
{
  WiFi_Credentials  WiFi_Creds [NUM_WIFI_CREDENTIALS];
  char TZ_Name[TZNAME_MAX_LEN];     // "America/Toronto"
  char TZ[TIMEZONE_MAX_LEN];        // "EST5EDT,M3.2.0,M11.1.0"
  uint16_t checksum;
} WM_Config;

WM_Config         WM_config;

#define  CONFIG_FILENAME              F("/wifi_cred.dat")


// Indicates whether ESP has WiFi credentials saved from previous session, or double reset detected
bool initialConfig = false;

// Use false if you don't like to display Available Pages in Information Page of Config Portal
// Comment out or use true to display Available Pages in Information Page of Config Portal
// Must be placed before #include <ESP_WiFiManager.h>
#define USE_AVAILABLE_PAGES     false

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You'll loose the feature of dynamically changing from DHCP to static IP, or vice versa
// You have to explicitly specify false to disable the feature.
#define USE_STATIC_IP_CONFIG_IN_CP          false

// Use false to disable NTP config. Advisable when using Cellphone, Tablet to access Config Portal.
// See Issue 23: On Android phone ConfigPortal is unresponsive (https://github.com/khoih-prog/ESP_WiFiManager/issues/23)
#define USE_ESP_WIFIMANAGER_NTP     false

// Just use enough to save memory. On ESP8266, can cause blank ConfigPortal screen
// if using too much memory
#define USING_AFRICA        false
#define USING_AMERICA       true
#define USING_ANTARCTICA    false
#define USING_ASIA          true
#define USING_ATLANTIC      false
#define USING_AUSTRALIA     true
#define USING_EUROPE        false
#define USING_INDIAN        false
#define USING_PACIFIC       false
#define USING_ETC_GMT       false

// Use true to enable CloudFlare NTP service. System can hang if you don't have Internet access while accessing CloudFlare
// See Issue #21: CloudFlare link in the default portal (https://github.com/khoih-prog/ESP_WiFiManager/issues/21)
#define USE_CLOUDFLARE_NTP          false
#define USING_CORS_FEATURE          true

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
  // Force DHCP to be true
  #if defined(USE_DHCP_IP)
    #undef USE_DHCP_IP
  #endif
  #define USE_DHCP_IP     true
#else
  // You can select DHCP or Static IP here
  #define USE_DHCP_IP     true
  //#define USE_DHCP_IP     false
#endif

#if ( USE_DHCP_IP )
  // Use DHCP

  #if (_ESPASYNC_WIFIMGR_LOGLEVEL_ > 3)
    #warning Using DHCP IP
  #endif

  IPAddress stationIP   = IPAddress(0, 0, 0, 0);
  IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);

#else
  // Use static IP

  #if (_ESPASYNC_WIFIMGR_LOGLEVEL_ > 3)
    #warning Using static IP
  #endif

  #ifdef ESP32
    IPAddress stationIP   = IPAddress(192, 168, 2, 232);
  #else
    IPAddress stationIP   = IPAddress(192, 168, 2, 186);
  #endif

  IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
  IPAddress netMask     = IPAddress(255, 255, 255, 0);
#endif


#define USE_CONFIGURABLE_DNS      true

IPAddress dns1IP      = gatewayIP;
IPAddress dns2IP      = IPAddress(8, 8, 8, 8);

#define USE_CUSTOM_AP_IP          false

IPAddress APStaticIP  = IPAddress(192, 168, 100, 1);
IPAddress APStaticGW  = IPAddress(192, 168, 100, 1);
IPAddress APStaticSN  = IPAddress(255, 255, 255, 0);

#include <ESPAsync_WiFiManager.h>               //https://github.com/khoih-prog/ESPAsync_WiFiManager

// SSID and PW for Config Portal
String ssid = "ESP_" + String(ESP_getChipId(), HEX);
String password;

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

String host = "enermo";

#define HTTP_PORT     80

AsyncWebServer server(HTTP_PORT);
AsyncDNSServer dnsServer;

AsyncEventSource events("/events");

String http_username = "admin";
String http_password = "admin";

String separatorLine = "===============================================================";

WiFi_AP_IPConfig  WM_AP_IPconfig;
WiFi_STA_IPConfig WM_STA_IPconfig;

void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig);

void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig);
void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig);

void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig);
uint8_t connectMultiWiFi();

String formatBytes(size_t bytes);
void toggleLED();
void heartBeatPrint();

#if USE_ESP_WIFIMANAGER_NTP
  void printLocalTime();
#endif

void heartBeatPrint();
void check_WiFi();
void check_status();
int calcChecksum(uint8_t* address, uint16_t sizeToCalc);
bool loadConfigData();
void saveConfigData();

// Main html page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Energy Monitoring Station</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.2.0/css/all.min.css">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: DimGrey; color: white; font-size: 1.7rem; padding: 10px; }
    .content { padding: 20px; }
    .card { background-color: whitesmoke; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.voltage { color: blue; }
    .card.current { color: teal; }
    .card.power { color: navy; }
    .card.energy { color: tomato; }
    .card.frequency { color: orange; }
    .card.energy { color: purple; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>ENERGY MONITORING STATION</h3>
    <h5>Name : PZEM-004T, Location : Melbourne 3000, VIC, Australia</h5>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card voltage">
        <h4><i class="fas fa-plug"></i> VOLTAGE</h4><p><span class="reading"><span id="voltage">%VOLTAGE%</span> V</span></p>
      </div>
      <div class="card current">
        <h4><i class="fas fa-solid fa-wind"></i> CURRENT</h4><p><span class="reading"><span id="current">%CURRENT%</span> A</span></p>
      </div>
      <div class="card power">
        <h4><i class="fas fa-solid fa-bolt"></i> POWER</h4><p><span class="reading"><span id="power">%POWER%</span> W</span></p>
      </div>
      <div class="card energy">
        <h4><i class="fas fa-solid fa-bolt-lightning"></i> ENERGY</h4><p><span class="reading"><span id="energy">%ENERGY%</span> kwh</span></p>
      </div>
      <div class="card frequency">
        <h4><i class="fas fa-solid fa-wave-square"></i> FREQUENCY</h4><p><span class="reading"><span id="frequency">%FREQUENCY%</span> Hz</span></p>
      </div>
      <div class="card pf">
        <h4><i class="fas fa-solid fa-plug-circle-bolt"></i> POWER FACTOR</h4><p><span class="reading"><span id="pf">%PF%</span>  </span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');

 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);

 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);

 source.addEventListener('voltage', function(e) {
  console.log("voltage", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);

 source.addEventListener('current', function(e) {
  console.log("current", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);

 source.addEventListener('power', function(e) {
  console.log("power", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);

 source.addEventListener('energy', function(e) {
  console.log("energy", e.data);
  document.getElementById("energy").innerHTML = e.data;
 }, false);

 source.addEventListener('frequency', function(e) {
  console.log("frequency", e.data);
  document.getElementById("frequency").innerHTML = e.data;
 }, false);

 source.addEventListener('pf', function(e) {
  console.log("pf", e.data);
  document.getElementById("pf").innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup()
{
  total_voltage = total_current = total_power = total_frequency = total_pf = prev_energy = 0;
  //set led pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial);

  delay(200);

  Serial.print(F("\nStarting Async_ESP32_FSWebServer_DRD using ")); Serial.print(FS_Name);
  Serial.print(F(" on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_ASYNC_WIFIMANAGER_VERSION);
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);

#if defined(ESP_ASYNC_WIFIMANAGER_VERSION_INT)
  if (ESP_ASYNC_WIFIMANAGER_VERSION_INT < ESP_ASYNC_WIFIMANAGER_VERSION_MIN)
  {
    Serial.print("Warning. Must use this example on Version later than : ");
    Serial.println(ESP_ASYNC_WIFIMANAGER_VERSION_MIN_TARGET);
  }
#endif

  Serial.setDebugOutput(false);

  if (FORMAT_FILESYSTEM)
    FileFS.format();

  // Format FileFS if not yet
  if (!FileFS.begin(true))
  {
    Serial.println(F("SPIFFS/LittleFS failed! Already tried formatting."));

    if (!FileFS.begin())
    {
      // prevents debug info from the library to hide err message.
      delay(100);

#if USE_LITTLEFS
      Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM. Stay forever"));
#else
      Serial.println(F("SPIFFS failed!. Please use LittleFS or EEPROM. Stay forever"));
#endif

      while (true)
      {
        delay(1);
      }
    }
  }

  File root = FileFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    String fileName = file.name();
    size_t fileSize = file.size();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    file = root.openNextFile();
  }

  Serial.println();

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

  if (!drd)
    Serial.println(F("Can't instantiate. Disable DRD feature"));

  unsigned long startedAt = millis();

  // New in v1.4.0
  initAPIPConfigStruct(WM_AP_IPconfig);
  initSTAIPConfigStruct(WM_STA_IPconfig);
  //////

  digitalWrite(LED_BUILTIN, LED_ON);

  //Local intialization. Once its business is done, there is no need to keep it around
  // Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
  //ESPAsync_WiFiManager ESPAsync_wifiManager(&webServer, &dnsServer);
  // Use this to personalize DHCP hostname (RFC952 conformed)0
  #if ( USING_ESP32_S2 || USING_ESP32_C3 )
    ESPAsync_WiFiManager ESPAsync_wifiManager(&server, NULL, "AsyncESP32-FSWebServer");
  #else
    AsyncDNSServer dnsServer;

    ESPAsync_WiFiManager ESPAsync_wifiManager(&server, &dnsServer, "AsyncESP32-FSWebServer");
  #endif

  #if USE_CUSTOM_AP_IP
    //set custom ip for portal
    // New in v1.4.0
    ESPAsync_wifiManager.setAPStaticIPConfig(WM_AP_IPconfig);
    //////
  #endif

  ESPAsync_wifiManager.setMinimumSignalQuality(-1);

  // Set config portal channel, default = 1. Use 0 => random channel from 1-13
  ESPAsync_wifiManager.setConfigPortalChannel(0);
  //////

  #if !USE_DHCP_IP
      // Set (static IP, Gateway, Subnetmask, DNS1 and DNS2) or (IP, Gateway, Subnetmask). New in v1.0.5
      // New in v1.4.0
      ESPAsync_wifiManager.setSTAStaticIPConfig(WM_STA_IPconfig);
      //////
  #endif

  // New from v1.1.1
  #if USING_CORS_FEATURE
    ESPAsync_wifiManager.setCORSHeader("Your Access-Control-Allow-Origin");
  #endif

  // We can't use WiFi.SSID() in ESP32as it's only valid after connected.
  // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are also cleared in reboot
  // Have to create a new function to store in EEPROM/SPIFFS for this purpose
  Router_SSID = ESPAsync_wifiManager.WiFi_SSID();
  Router_Pass = ESPAsync_wifiManager.WiFi_Pass();

  //Remove this line if you do not want to see WiFi password printed
  Serial.println("ESP Self-Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);

  // SSID to uppercase
  ssid.toUpperCase();
  password = "My" + ssid;

  bool configDataLoaded = false;

  // From v1.1.0, Don't permit NULL password
  if ( (Router_SSID != "") && (Router_Pass != "") )
  {
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass);
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());

    ESPAsync_wifiManager.setConfigPortalTimeout(120); //If no access point name has been previously entered disable timeout.
    Serial.println(F("Got ESP Self-Stored Credentials. Timeout 120s for Config Portal"));
  }

  if (loadConfigData())
  {
    configDataLoaded = true;

    ESPAsync_wifiManager.setConfigPortalTimeout(120); //If no access point name has been previously entered disable timeout.
    Serial.println(F("Got stored Credentials. Timeout 120s for Config Portal"));

    #if USE_ESP_WIFIMANAGER_NTP
        if ( strlen(WM_config.TZ_Name) > 0 )
        {
          LOGERROR3(F("Saving current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

      #if ESP8266
          configTime(WM_config.TZ, "pool.ntp.org");
      #else
          //configTzTime(WM_config.TZ, "pool.ntp.org" );
          configTzTime(WM_config.TZ, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
      #endif
        }
        else
        {
          Serial.println(F("Current Timezone is not set. Enter Config Portal to set."));
        }
    #endif
  }
  else
  {
    // Enter CP only if no stored SSID on flash and file
    Serial.println(F("Open Config Portal without Timeout: No stored Credentials."));
    initialConfig = true;
  }

  if (drd->detectDoubleReset())
  {
    // DRD, disable timeout.
    ESPAsync_wifiManager.setConfigPortalTimeout(0);

    Serial.println(F("Open Config Portal without Timeout: Double Reset Detected"));
    initialConfig = true;
  }

  if (initialConfig)
  {
    Serial.print(F("Starting configuration portal @ "));

    #if USE_CUSTOM_AP_IP
        Serial.print(APStaticIP);
    #else
        Serial.print(F("192.168.4.1"));
    #endif

    Serial.print(F(", SSID = "));
    Serial.print(ssid);
    Serial.print(F(", PWD = "));
    Serial.println(password);

    #if DISPLAY_STORED_CREDENTIALS_IN_CP
        // New. Update Credentials, got from loadConfigData(), to display on CP
        ESPAsync_wifiManager.setCredentials(WM_config.WiFi_Creds[0].wifi_ssid, WM_config.WiFi_Creds[0].wifi_pw,
                                            WM_config.WiFi_Creds[1].wifi_ssid, WM_config.WiFi_Creds[1].wifi_pw);
    #endif

    // Starts an access point
    if (!ESPAsync_wifiManager.startConfigPortal((const char *) ssid.c_str(), password.c_str()))
      Serial.println(F("Not connected to WiFi but continuing anyway."));
    else
    {
      Serial.println(F("WiFi connected...yeey :)"));
    }

    // Stored  for later usage, from v1.1.0, but clear first
    memset(&WM_config, 0, sizeof(WM_config));

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      String tempSSID = ESPAsync_wifiManager.getSSID(i);
      String tempPW   = ESPAsync_wifiManager.getPW(i);

      if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);

      if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);

      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    #if USE_ESP_WIFIMANAGER_NTP
        String tempTZ   = ESPAsync_wifiManager.getTimezoneName();

        if (strlen(tempTZ.c_str()) < sizeof(WM_config.TZ_Name) - 1)
          strcpy(WM_config.TZ_Name, tempTZ.c_str());
        else
          strncpy(WM_config.TZ_Name, tempTZ.c_str(), sizeof(WM_config.TZ_Name) - 1);

        const char * TZ_Result = ESPAsync_wifiManager.getTZ(WM_config.TZ_Name);

        if (strlen(TZ_Result) < sizeof(WM_config.TZ) - 1)
          strcpy(WM_config.TZ, TZ_Result);
        else
          strncpy(WM_config.TZ, TZ_Result, sizeof(WM_config.TZ_Name) - 1);

        if ( strlen(WM_config.TZ_Name) > 0 )
        {
          LOGERROR3(F("Saving current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);

        #if ESP8266
              configTime(WM_config.TZ, "pool.ntp.org");
        #else
              //configTzTime(WM_config.TZ, "pool.ntp.org" );
              configTzTime(WM_config.TZ, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
        #endif
        }
        else
        {
          LOGERROR(F("Current Timezone Name is not set. Enter Config Portal to set."));
        }
    #endif

    // New in v1.4.0
    ESPAsync_wifiManager.getSTAStaticIPConfig(WM_STA_IPconfig);
    //////

    saveConfigData();
  }

  startedAt = millis();

  if (!initialConfig)
  {
    // Load stored data, the addAP ready for MultiWiFi reconnection
    if (!configDataLoaded)
      loadConfigData();

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    if ( WiFi.status() != WL_CONNECTED )
    {
      Serial.println(F("ConnectMultiWiFi in setup"));
      connectMultiWiFi();
    }
  }

  Serial.print(F("After waiting "));
  Serial.print((float) (millis() - startedAt) / 1000);
  Serial.print(F(" secs more in setup(), connection result is "));

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print(F("connected. Local IP: "));
    Serial.println(WiFi.localIP());
  }
  else
    Serial.println(ESPAsync_wifiManager.getStatus(WiFi.status()));

  if ( !MDNS.begin(host.c_str()) )
  {
    Serial.println(F("Error starting MDNS responder!"));
  }

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", HTTP_PORT);

  //SERVER INIT
  events.onConnect([](AsyncEventSourceClient * client)
  {
    client->send("hello!", NULL, millis(), 1000);
  });

  server.addHandler(&events);

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.addHandler(new SPIFFSEditor(FileFS, http_username, http_password));
  //server.serveStatic("/", FileFS, "/").setDefaultFile("index.htm");
    // Handle Web Server

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, dataHandler);
  });

  server.onNotFound([](AsyncWebServerRequest * request)
  {
    Serial.print(F("NOT_FOUND: "));

    if (request->method() == HTTP_GET)
      Serial.print(F("GET"));
    else if (request->method() == HTTP_POST)
      Serial.print(F("POST"));
    else if (request->method() == HTTP_DELETE)
      Serial.print(F("DELETE"));
    else if (request->method() == HTTP_PUT)
      Serial.print(F("PUT"));
    else if (request->method() == HTTP_PATCH)
      Serial.print(F("PATCH"));
    else if (request->method() == HTTP_HEAD)
      Serial.print(F("HEAD"));
    else if (request->method() == HTTP_OPTIONS)
      Serial.print(F("OPTIONS"));
    else
      Serial.print(F("UNKNOWN"));

    Serial.println(" http://" + request->host() + request->url());

    if (request->contentLength())
    {
      Serial.println("_CONTENT_TYPE: " + request->contentType());
      Serial.println("_CONTENT_LENGTH: " + request->contentLength());
    }

    int headers = request->headers();
    int i;

    for (i = 0; i < headers; i++)
    {
      AsyncWebHeader* h = request->getHeader(i);
      Serial.println("_HEADER[" + h->name() + "]: " + h->value());
    }

    int params = request->params();

    for (i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);

      if (p->isFile())
      {
        Serial.println("_FILE[" + p->name() + "]: " + p->value() + ", size: " + p->size());
      }
      else if (p->isPost())
      {
        Serial.println("_POST[" + p->name() + "]: " + p->value());
      }
      else
      {
        Serial.println("_GET[" + p->name() + "]: " + p->value());
      }
    }

    request->send(404);
  });

  server.onFileUpload([](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final)
  {
    (void) request;

    if (!index)
      Serial.println("UploadStart: " + filename);

    Serial.print((const char*)data);

    if (final)
      Serial.println("UploadEnd: " + filename + "(" + String(index + len) + ")" );
  });

  server.onRequestBody([](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total)
  {
    (void) request;

    if (!index)
      Serial.println("BodyStart: " + total);

    Serial.print((const char*)data);

    if (index + len == total)
      Serial.println("BodyEnd: " + total);
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA

  server.begin();

  Serial.print(F("HTTP server started @ "));
  Serial.println(WiFi.localIP());

  Serial.println(separatorLine);
  Serial.print("Open http://"); Serial.print(host);
  Serial.println(".local/edit to see the file browser");
  Serial.println("Using username = " + http_username + " and password = " + http_password);
  Serial.println(separatorLine);

  digitalWrite(LED_BUILTIN, LED_OFF);

  delay(2000);
  currentTime = millis();
}


void loop()
{
    // Call the double reset detector loop method every so often,
    // so that it can recognise when the timeout expires.
    // You can also call drd.stop() when you wish to no longer
    // consider the next reset as a double reset.
    if (drd)
      drd->loop();

    //check_status();

    ///// PZEM-004T BEGIN
    //static int32_t  voltage, current, power, gas;  // BME readings
    static char loopCounterStr[16], voltageStr[16], currentStr[16], powerStr[16], pfStr[16], energyStr[16];
    static char     buf[16];                        // sprintf text buffer
    static uint16_t stringLen;
    static float    alt;                            // Temporary variable
    static String strToInt;


    if((float) (millis() - thisTime) > 15000) {
        thisTime = millis();

        //// Read PZEM004T Sensor, loop if failed
        Serial.print("Custom Address:");
        Serial.println(pzem.readAddress(), HEX);

        // Read the data from the sensor
        voltage = pzem.voltage();
        current = pzem.current();
        power = pzem.power();
        energy = pzem.energy();
        frequency = pzem.frequency();
        pf = pzem.pf();

        // Check if the data is valid
        if(isnan(voltage)){
            Serial.println("Error reading voltage");
        } else if (isnan(current)) {
            Serial.println("Error reading current");
        } else if (isnan(power)) {
            Serial.println("Error reading power");
        } else if (isnan(energy)) {
            Serial.println("Error reading energy");
        } else if (isnan(frequency)) {
            Serial.println("Error reading frequency");
        } else if (isnan(pf)) {
            Serial.println("Error reading power factor");
        } else {

            if (loopCounter == 0) {                    // Show header @25 loops
              if(samplingCount == 0){
                  prev_energy = energy;
              }
              delay(500);
              Serial.print(F("\nLoop Temp\xC2\xB0\x43 Volt V Curr A   Power W Energ kWh Freq Hz PF   "));
              Serial.print(F("\xE2\x84\xA6\n==== ====== ====== ========= ======= ======\n"));  // "�C" symbol
            }                                                     // if-then time to show headers

            samplingCount += 1;
            total_voltage += voltage;
            total_current += current;
            total_power += power;
            total_pf += pf;

            // Print the values to the Serial console
            Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
            Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
            Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
            Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
            Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
            Serial.print("PF: ");           Serial.println(pf);

            sprintf(loopCounterStr, "%4d", (loopCounter - 1) % 9999);  // loopCounter, Clamp to 9999
            voltageString = String(voltage, 2);
            currentString = String(current, 2);
            powerString = String(power, 2);
            energyString = String(energy, 2);
            frequencyString = String(frequency, 2);
            pfString = String(pf, 2);

            Serial.print(loopCounterStr);
            Serial.print(voltageString);
            Serial.print(currentString);
            Serial.print(powerString);
            Serial.print(energyString);
            Serial.print(frequencyString);
            Serial.print(pfString);
            Serial.println("  ");

            // Send Events to the local Web Server with the Sensor Readings
            events.send("ping",NULL,millis());
            events.send(voltageString.c_str(),"voltage",millis());
            events.send(currentString.c_str(),"current",millis());
            events.send(powerString.c_str(),"power",millis());
            events.send(energyString.c_str(),"energy",millis());
            events.send(frequencyString.c_str(),"frequency",millis());
            events.send(pfString.c_str(),"pf",millis());
        }
        Serial.println();

        if((float) (millis() - currentTime) > requestDelay || loopCounter == 0) {
            currentTime = millis();
            loopCounter++;
            voltage = total_voltage/samplingCount;
            current = total_current/samplingCount;
            power = total_power/samplingCount;
            pf = total_pf/samplingCount;
            temp_energy = energy;
            energy = energy - prev_energy;
            samplingCount = 0;
            total_voltage = 0;
            total_current = 0;
            total_power = 0;
            total_pf = 0;
            prev_energy = temp_energy;

            sprintf(loopCounterStr, "%4d", (loopCounter - 1) % 9999);  // loopCounter, Clamp to 9999
            voltageString = String(voltage, 2);
            currentString = String(current, 2);
            powerString = String(power, 2);
            energyString = String(energy, 2);
            frequencyString = String(frequency, 2);
            pfString = String(pf, 2);
        }
    }
}
