/*
 * NeoWiFi implementation for the ESP8266/ESP32
 * 
 * Edited version to connect to WPA2-Enterprise networks - set user
 * id, ssid and password in config.h
 * 
 * The API allows for setting each pixel to a RGB color over WiFi, 
 * and for running preset animations
 * 
 * Configuration parameters are set in config.h
 */
#include "esp_wpa2.h"
 
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <WiFiUdp.h>

#ifdef ARDUINO_ARCH_ESP32
#include <WebServer.h>
#include <mDNS.h>
#include <WiFi.h>
#else
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#endif

#include "config.h"

// Create a NeoPixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel (
  NEO_LED_COUNT, 
  NEO_PIN, 
  NEO_COLOR_ORDER + NEO_KHZ800
);

// Power flag
int powerOn = 0;

// Whether an animation is playing
int animationPlaying = 0;

// Array containing current color of all LEDs
uint32_t ledStates [NEO_LED_COUNT];

// Pointer to the currently playing preset animation handler function
void (*currentPresetHandler)();

#ifdef ARDUINO_ARCH_ESP32
WebServer server(SERVER_PORT);
#else
ESP8266WebServer    server(SERVER_PORT);
#endif

DynamicJsonDocument jsonBuffer(JSON_CAPACITY);

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  strip.setBrightness(BRIGHTNESS);
  strip.begin();

  fullWhite();
  setLedStates();

  // Blocks until connected to a network
  connectWifi();
  
  startWebServer();
  Serial.println("HTTP server started");

  startOTA();
  Serial.println("Ready");
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  // Check network connection
  if ( (WiFi.status() != WL_CONNECTED) || (WiFi.localIP().toString() == "0.0.0.0") ) {
    Serial.println("Lost network connection");

    // Attempt to reconnect
    connectWifi();
  }

  // Update current animation if running
  playAnimation();
}

/*  *  *  *  *  *  *  *  *  *  * Web Server *  *  *  *  *  *  *  *  * */
// Endpoint table lookup entry
struct endpoint_table_entry {
  const char * endpoint;       // endpoint string
  HTTPMethod   allowed_method; // allowed method
  void (*handler)();           // handler function
};

// Preset animation table lookup entry
struct animation_table_entry {
  int id;            // id for the animation
  const char * name; // name for the animation
  void (*handler)(); // handler function
};

// Lookup table to map each endpoint to its 
// allowed method and handler function
static struct endpoint_table_entry endpoint_table[] = 
{ 
  { "/",         HTTP_GET,  &handleRoot    },
  { "/monitor",  HTTP_GET,  &handleMonitor },
  { "/reset",    HTTP_GET,  &handleReset   },
  { "/on",       HTTP_GET,  &allOn         }, 
  { "/off",      HTTP_GET,  &allOff        }, 
  { "/write",    HTTP_POST, &setColor      }, 
  { "/preset",   HTTP_POST, &selectPreset  }, 
  { "/next",     HTTP_GET,  &nextPreset    }, 
  { "/previous", HTTP_GET,  &lastPreset    }, 
  { "/start",    HTTP_GET,  &startPreset   }, 
  { "/stop",     HTTP_GET,  &stopPreset    },
  { "/count",    HTTP_GET,  &getPixelCount },
  { NULL }
};

// Lookup table to map each preset animation to its
// handler. The handler is called each clock cycle if
// the animation is enabled.
static struct animation_table_entry animation_table[] = 
{ 
  { 1, "Breathe Red",   &breatheRed   },
  { 2, "Green Pulse",   &pulseGreen   },
  { 3, "Green Marquee", &marqueeGreen }, 
  { NULL }
};

// Set up all endpoints/pages and start the web server
void startWebServer() {
  struct endpoint_table_entry *p_entry = endpoint_table;
  for ( ; p_entry -> endpoint != NULL ; p_entry++ )
    server.on(p_entry -> endpoint, handleRequest);
    
  server.onNotFound(handleNotFound);
  server.begin();
}
/*  *  *  *  *  *  *  *  *  *  * Web Server *  *  *  *  *  *  *  *  * */

/*  *  *  *  *  *  *  *  * API Endpoint Handlers *  *  *  *  *  *  *  */
// TODO: Shows a serial monitor-style console
void handleMonitor() {
  server.send(200, "text/plain", "NeoWiFi Status Monitor");
}

// Reset the board
void handleReset() {
  ESP.restart();
  server.send(200, "text/plain", "Restarting");
}

// Returns some text (for testing)
void handleRoot() {
  server.send(200, "text/plain", "NeoWiFi Home");
}

// Sets all LEDs to last saved state
void allOn() {
  for(uint16_t i = 0; i < NEO_LED_COUNT; i++)
    strip.setPixelColor(i, ledStates[i]);

  strip.show();
  server.send(200, "text/plain", "On");
}

// Saves LED state and sets all to 0
void allOff() {  
  clearStrip();
  animationPlaying = 0;
  server.send(200, "text/plain", "Off");
}

// Sets the color of one or more LEDs
void setColor() {
  deserializeJson(jsonBuffer, server.arg("plain"));
  JsonArray jsonBody = jsonBuffer.as<JsonArray>();

  for(JsonVariant line : jsonBody) {
    JsonVariant jsonLedIndex = line.getMember("index");
    JsonVariant jsonR = line.getMember("r");
    JsonVariant jsonG = line.getMember("g");
    JsonVariant jsonB = line.getMember("b");

    int ledIndex = jsonLedIndex.as<int>();
    int r = jsonR.as<int>();
    int g = jsonG.as<int>();
    int b = jsonB.as<int>();
  
    strip.setPixelColor(ledIndex, strip.Color(r, g, b));
  }
  
  strip.show();
  
  server.send(200, "text/plain", "Colors set");
}

// Starts a preset animation by ID
void selectPreset() {
  deserializeJson(jsonBuffer, server.arg("plain"));
  int presetID = jsonBuffer["id"];

  // Look up the selection
  struct animation_table_entry *p_entry = animation_table;
  for ( ; p_entry -> id != NULL ; p_entry++ ) {
    if ( presetID == p_entry -> id ) {
      // Valid animation
      clearStrip();
      currentPresetHandler = p_entry -> handler;
      animationPlaying = presetID;
      
      server.send(
        200, 
        "text/plain", 
        "Playing animation " + String(animationPlaying) + ": " + p_entry -> name
      );

      return;
    }
  }

  // Invalid animation ID received
  server.send(400, "text/plain", "Invalid animation ID: " + String(presetID));
}

void nextPreset() {
  server.send(200, "text/plain", "");
}

void lastPreset() {
  server.send(200, "text/plain", "");
}

void startPreset() {
  animationPlaying = 1;
  server.send(200, "text/plain", "Start");
}

void stopPreset() {
  animationPlaying = 0;
  server.send(200, "text/plain", "Stop");
}

// Returns the number of pixels in the strip
void getPixelCount() {
  server.send(200, "text/plain", String(NEO_LED_COUNT));
}

// Returns a 404 "not found" error to the client
void handleNotFound() {
  String message = "404 Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  
  server.send(404, "text/plain", message);
}

// Returns a 405 "method not allowed" error to the client
void handleWrongMethod() {
  String message = "405 Method Not Allowed\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  
  server.send(405, "text/plain", message);
}

/*  *  *  *  *  *  *  *  * API Endpoint Handlers *  *  *  *  *  *  *  */


/*  *  *  *  *  *  *  *   * Preset Animations *  *  *  *  *  *  *  *  */
// Call the current animation handler, if it should be playing
void playAnimation() {
  if (animationPlaying)
    currentPresetHandler();
}

// Breathing red animation
void breatheRed() {
  // Brightness
  static int intensity = 0;

  // Increase or decrease brightness
  static boolean increasing = true;
  
  // Time to wait between steps
  static const int delayMillis = 50;
  static unsigned long lastMillis;

  // Check the time
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= delayMillis) {
    // Update the pixel states
    for(int i = 0; i < NEO_LED_COUNT; i++)
      strip.setPixelColor(i, strip.Color(intensity, 0, 0) );

    // Update the brightness
    if(intensity >= 255)
      increasing = false;

    if(intensity <= 0)
      increasing = true;

    if(increasing)
      intensity++;
    else
      intensity--;


    strip.show();
    lastMillis = currentMillis;
  }
}

// Green pulsing animation. Systems operational.
void pulseGreen() {
  // Brightness
  static int intensity = 0;

  // Increase or decrease brightness
  static boolean increasing = true;
  
  // Time to wait between steps
  static const int delayMillis = 2;
  static unsigned long lastMillis;

  // Check the time
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= delayMillis) {
    // Update the pixel states
    for(int i = 0; i < NEO_LED_COUNT; i++)
      strip.setPixelColor(i, strip.Color(0, intensity, 0) );

    // Update the brightness
    if(intensity >= 255)
      increasing = false;

    if(intensity <= 0)
      increasing = true;

    if(increasing)
      intensity++;
    else
      intensity--;


    strip.show();
    lastMillis = currentMillis;
  }
}

// Green marquee animation
void marqueeGreen() {
  // Brightness
  static int index = 0;

  // Width of moving section
  static const int width = 4;

  // Increase or decrease index
  static boolean increasing = true;

  // Time to wait between steps
  static const int delayMillis = 20;
  static unsigned long lastMillis;

  // Check the time
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= delayMillis) {
    // Update the pixel states
    for(int i = index; i < width; i++)
      strip.setPixelColor(i, strip.Color(0, 255, 0) );

    // Clear trailing pixels
    if(increasing) {
      //for(int i = 0; i < index; i++)
       //strip.setPixelColor(i, strip.Color(0, 0, 0) );
    } else {
      //for(int i = index+width+1; i <= NEO_LED_COUNT; i++)
        //strip.setPixelColor(i, strip.Color(0, 0, 0) );
    }

    // Update the index
    if(index >= NEO_LED_COUNT)
      increasing = false;

    if(index <= 0)
      increasing = true;

    if(increasing)
      index++;
    else
      index--;


    strip.show();
    lastMillis = currentMillis;
  }
}
/*  *  *  *  *  *  *  *   * Preset Animations *  *  *  *  *  *  *  *  */

// Saves the current color of each LED
void setLedStates() {
  for(uint16_t i = 0; i < NEO_LED_COUNT; i++)
    ledStates[i] = strip.getPixelColor(i);
}

// Turn all pixels white
void fullWhite() {
  for(uint16_t i=0; i < NEO_LED_COUNT; i++)
    strip.setPixelColor(i, strip.Color(255, 255, 255) );
  
  strip.show();
}

// Turn all pixels off
void clearStrip() {
  // Save current color of all LEDs
  setLedStates();
  
  for(uint16_t i = 0; i < NEO_LED_COUNT; i++)
    strip.setPixelColor(i, strip.Color(0, 0, 0) );

  strip.show(); 
}

// Validates all incoming requests, and decides which handler to call
void handleRequest() {
  String endpoint = server.uri();
  struct endpoint_table_entry *p_entry = endpoint_table;

  // Look up the endpoint
  for ( ; p_entry -> endpoint != NULL ; p_entry++ ) {
    if ( endpoint == p_entry -> endpoint ) {
      if ( server.method() == p_entry -> allowed_method ) {
        // Call the endpoint's handler
        p_entry -> handler();
      } else {
        // 405
        handleWrongMethod();
      }
    }
  }   

  // Endpoint does not exist
  handleNotFound();
}

/*  *  *  *  *  *  *  *  *  *  * WiFi and OTA *  *  *  *  *  *  *  *  */

// Connect to preconfigured WiFi network
void connectWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(EAP_SSID);
    
  WiFi.disconnect(true);  

  WiFi.mode(WIFI_STA);
  
  esp_wpa2_config_t defaultConfig = WPA2_CONFIG_INIT_DEFAULT();
  esp_wifi_sta_wpa2_ent_enable(&defaultConfig);

  // Configure enterprise connection
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_USERID, strlen(EAP_USERID));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERID, strlen(EAP_USERID));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  
  #ifdef USE_CERT
  esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)CA_PEM, strlen(CA_PEM)+1);
  #endif

  // Connect
  WiFi.begin(EAP_SSID);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Connected to network");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Start the OTA update server
void startOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASS);

  ArduinoOTA.onStart([]() {
    Serial.println("OTA start");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println("OTA ready");
}
/*  *  *  *  *  *  *  *  *  *  * WiFi and OTA *  *  *  *  *  *  *  *  */
