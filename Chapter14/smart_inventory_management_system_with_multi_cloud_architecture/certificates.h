#include <pgmspace.h>

#define CERTIFICATES
#define THINGNAME "<YOUR_THING_NAME_HERE>"                         //change this

const char WIFI_SSID[] = "<YOUR_WIFI_SSID_HERE>";               //change this
const char WIFI_PASSWORD[] = "<YOUR_WIFI_PASSWORD_HERE>";           //change this
const char AWS_IOT_ENDPOINT[] = "<YOUR_AWS_IOT_ENDPOINT_HERE>";       //change this

// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<insert_your_aws_root_certificate_here>
-----END CERTIFICATE-----
)EOF";

// Device Certificate                                               //change this
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
<insert_your_device_certificate_here>
-----END CERTIFICATE-----
)KEY";

// Device Private Key                                               //change this
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
<insert_your_device_private_key_here>
-----END RSA PRIVATE KEY-----
)KEY";
