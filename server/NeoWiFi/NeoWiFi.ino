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
#include "config.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel (
  NEO_LED_COUNT, 
  NEO_PIN, 
  NEO_RGBW + NEO_KHZ800
);

int power_on = 0;

// Array containing current color of all LEDs
uint32_t ledStates [NEO_LED_COUNT];

// Whether an animation is playing
int animationPlaying = 0;

// Pointer to the currently playing preset animation
void (*currentPresetHandler)();

ESP8266WebServer    server(SERVER_PORT);
DynamicJsonDocument jsonBuffer(JSON_CAPACITY);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Booting");
  
  connectWifi();
  startWebServer();
  Serial.println("HTTP server started");

  startOTA();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  memset ( ledStates, 0, sizeof(ledStates) );
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  fullWhite();
  setLedStates();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  // Update current animation
  if(animationPlaying) {
    cradle();
  }
}

// Endpoint table lookup entry
struct endpoint_table_entry {
  const char * endpoint;         // endpoint string
  HTTPMethod   allowed_method;   // allowed method
  void (*handler)();             // handler function
};

// Preset animation table lookup entry
struct animation_table_entry {
  int id;             // id for the animation
  const char * name;  // name for the animation
  void (*handler)();  // handler function
};

// Lookup table to map each endpoint to its 
// allowed method and handler function
static struct endpoint_table_entry endpoint_table[] = 
{ 
  { "/",         HTTP_GET,  &handleRoot    },
  {"/monitor",   HTTP_GET,  &handleMonitor },
  { "/on",       HTTP_GET,  &allOn         }, 
  { "/off",      HTTP_GET,  &allOff        }, 
  { "/write",    HTTP_POST, &setColor      }, 
  { "/preset",   HTTP_POST, &selectPreset  }, 
  { "/next",     HTTP_GET,  &nextPreset    }, 
  { "/previous", HTTP_GET,  &lastPreset    }, 
  { NULL }
};

// Lookup table to map each preset animation to its
// handler. The handler is called each clock cycle if
// the animation is enabled.
static struct animation_table_entry animation_table[] = 
{ 
  { 0, "Newton's Cradle", &cradle },
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

/*  *  *  *  *  *  *  *  * API Endpoint Handlers *  *  *  *  *  *  *  */
// Shows a serial monitor-type console
void handleMonitor() {
  server.send(200, "text/plain", "NeoWiFi Status Monitor");
}

// Returns the current state of all LEDs
void handleRoot() {
  server.send(200, "text/plain", "NeoWiFi Home");
}

// Sets all LEDs to last saved state
void allOn() {
  for(uint16_t i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor(i, ledStates[i]);

  strip.show();
  server.send(200, "text/plain", "");
}

// Saves LED state and sets all to 0
void allOff() {  
  clearStrip();
  server.send(200, "text/plain", "");
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
    JsonVariant jsonW = line.getMember("w");

    int ledIndex = jsonLedIndex.as<int>();
    int r = jsonR.as<int>();
    int g = jsonG.as<int>();
    int b = jsonB.as<int>();
    int w = jsonW.as<int>();
  
    strip.setPixelColor(ledIndex, strip.Color(g, r, b, w));
  }
  
  strip.show();
  
  server.send(200, "text/plain", "");
}

// Starts a preset animation by ID
void selectPreset() {
  deserializeJson(jsonBuffer, server.arg("plain"));
  int presetID = jsonBuffer["id"];

  // Look up the selection
  struct animation_table_entry *p_entry = animation_table;
  
  if(presetID == 0) {
    // Valid animation
    clearStrip();
    currentPresetHandler = p_entry -> handler;
    animationPlaying = 1;
    
    server.send(
      200, 
      "text/plain", 
      "Playing animation " + String(animationPlaying) + ": " + p_entry -> name
    );
  } else {
    // Invalid animation ID received
    server.send(400, "text/plain", "Invalid animation ID: " + String(presetID));
  }
}

void nextPreset() {
  server.send(200, "text/plain", "");
}

void lastPreset() {
  server.send(200, "text/plain", "");
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



/*  *  *  *  *  *  *  *  * Preset Animations *  *  *  *  *  *  *  */
void cradle() {
  // Save the position of the moving ball.
  // Starts at the right-most edge of the strip.
  static int movingBall = 0;

  // Speed of the ball (pixels per second)
  int ballSpeed = 4;

  // Need to subtract a ball if the total number of 
  // pixels is even
  int offset = 0;
  if(NEO_LED_COUNT % 2 == 0)
    offset = 1;

  // Number of balls in the cradle
  int numBalls = (NEO_LED_COUNT / 2) - offset;
  
  // Light up pixels in the center
  int center = (NEO_LED_COUNT / 2) - (numBalls / 2);

  for(int i = 0; i < numBalls - 1; i++){
    strip.setPixelColor(center + i, strip.Color(0, 0, 0, 255));
  }

  // Put the moving ball in position
  strip.setPixelColor(movingBall, strip.Color(0, 0, 0, 255));
  strip.show();
}

/*  *  *  *  *  *  *  *  * Preset Animations *  *  *  *  *  *  *  */
// Saves the current color of each LED
void setLedStates() {
  for(uint16_t i = 0; i < strip.numPixels(); i++)
    ledStates[i] = strip.getPixelColor(i);
}

// Turn all pixels white
void fullWhite() {
  for(uint16_t i=0; i<strip.numPixels(); i++)
    strip.setPixelColor(i, strip.Color(0,0,0, 255 ) );
  
  strip.show();
}

// Turn all pixels off
void clearStrip() {
 // Save current color of all LEDs
  setLedStates();
  
  for(uint16_t i = 0; i < strip.numPixels(); i++)
    strip.setPixelColor(i, strip.Color(0, 0, 0, 0) );

  strip.show(); 
}

// Validates all incoming requests, and decides which handler to call
void handleRequest() {
  digitalWrite(LED_BUILTIN, 1);

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

  digitalWrite(LED_BUILTIN, 0);
}

// Connect to preconfigured WiFi network, or broadcast as AP to configure a new one
void connectWifi() {
  // Attempt connection. Blocks until successful.
  WiFi.hostname(DHCP_HOSTNAME);
  WiFiManager wifiManager;
  wifiManager.autoConnect();
}

void startOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASS);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  ArduinoOTA.begin();
}
