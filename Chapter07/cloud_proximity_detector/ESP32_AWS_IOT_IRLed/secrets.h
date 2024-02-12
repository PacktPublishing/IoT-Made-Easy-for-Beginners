#define SECRET
#define THINGNAME "ESP32Thing"

const char WIFI_SSID[] = "My_Wifi_SSID";
const char WIFI_PASSWORD[] = "My-Wifi-Password:IOT";
const char AWS_IOT_ENDPOINT[] = "<INSERT_YOUR_ENDPOINT_HERE>";

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<INSERT_YOUR_CERTIFICATE_HERE>
-----END CERTIFICATE-----
)EOF";

// Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
<INSERT_YOUR_CERTIFICATE_HERE>
-----END CERTIFICATE-----
)KEY";

// Device Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
<INSERT_YOUR_CERTIFICATE_HERE>
-----END RSA PRIVATE KEY-----
)KEY";
