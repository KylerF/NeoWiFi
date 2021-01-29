#ifndef _CONFIG_H
#define _CONFIG_H

// WiFi settings
const char * HOSTNAME = "NeoWiFi";
const int CONNECT_TIMEOUT = 60;

// Enterprise connection details
#define EAP_SSID     "SSID"
#define EAP_USERID   "userid"
#define EAP_PASSWORD "password"
#define USE_CERT // comment out if not using a certificate

// Root CA certificate
const char* CA_PEM = "-----BEGIN CERTIFICATE-----\n"
"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\n"
"9CcdA/blDouY6bGULa4=\n"
"-----END CERTIFICATE-----\n\0";

// NeoPixel strip settings
#define NEO_COLOR_ORDER NEO_GRB
const uint8_t NEO_PIN = 16;
const uint8_t BRIGHTNESS = 100;
const uint8_t NEO_LED_COUNT = 49;

// Web server settings
#define SERVER_PORT 80

// HTTP constants
#define JSON_CAPACITY 4096

// OTA update settings
#define OTA_HOSTNAME "NeoWiFi"
#define OTA_PASS     "somecode4u"

#endif
