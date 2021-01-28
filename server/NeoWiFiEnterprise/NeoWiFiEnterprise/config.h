#ifndef _CONFIG_H
#define _CONFIG_H

// WiFi settings
#define EAP_SSID     "SSID"
#define EAP_USERID   "userid"
#define EAP_PASSWORD "password"
const char * HOSTNAME = "NeoWiFi";
const int CONNECT_TIMEOUT = 60;

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
