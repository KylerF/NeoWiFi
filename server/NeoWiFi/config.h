#ifndef _CONFIG_H
#define _CONFIG_H

#include <Adafruit_NeoPixel.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// WiFi settings
#define DHCP_HOSTNAME "NeoWiFi"

// Network settings
#define SERVER_PORT 80

// HTTP constants
#define JSON_CAPACITY 4096

// NeoPixel strip settings
#define NEO_COLOR_ORDER NEO_GRB
const uint8_t NEO_PIN = 5;
const uint8_t BRIGHTNESS = 100;
const uint8_t NEO_LED_COUNT = 49;

// OTA update settings
#define OTA_HOSTNAME "NeoWiFi"
#define OTA_PASS     "somecode4u"

#endif
