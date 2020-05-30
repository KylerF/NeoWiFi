/*
 * NeoWiFi implementation for the ESP8266
 * 
 * The API allows for setting each pixel to a RGB color over WiFi, 
 * and for running preset animations.
 * 
 * Configuration parameters are set in config.h
 */
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

ESP8266WebServer    server(SERVER_PORT);
DynamicJsonDocument jsonBuffer(JSON_CAPACITY);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  // Update current animation if running
  playAnimation();
}

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
  { 1, "Bleed", &bleed },
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
      animationPlaying = 1;
      
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


/*  *  *  *  *  *  *  *  * Preset Animations *  *  *  *  *  *  *  */
// Call the current animation handler, if it should be playing
void playAnimation() {
  if (animationPlaying)
    currentPresetHandler();
}

// Bleeding animation handler
void bleed() {
  static int stripIndex = 0;
  
  // Time to wait between steps
  static const int delayMillis = 10;
  static unsigned long lastMillis;

  // Check the time
  unsigned long currentMillis = millis();
  if (currentMillis - lastMillis >= delayMillis) {
    // Update the pixel states
    strip.setPixelColor(stripIndex, strip.Color(255, 0, 0) );
    strip.setPixelColor((stripIndex - 1) % NEO_LED_COUNT, strip.Color(5, 0, 0) );
    strip.setPixelColor((stripIndex - 5) % NEO_LED_COUNT, strip.Color(0, 0, 0));

    // Update the indices
    stripIndex++;
    if (stripIndex >= NEO_LED_COUNT)
      stripIndex = 0;
    
    strip.show();
    lastMillis = currentMillis;
  }
}

/*  *  *  *  *  *  *  *  * Preset Animations *  *  *  *  *  *  *  */
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

// Connect to preconfigured WiFi network, or broadcast as 
// AP to configure a new one
void connectWifi() {
  // Attempt connection. Blocks until successful.
  WiFi.hostname(DHCP_HOSTNAME);
  WiFiManager wifiManager;
  wifiManager.autoConnect(DHCP_HOSTNAME);
}

// Start the OTA update server
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
