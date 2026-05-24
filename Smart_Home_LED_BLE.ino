#include <WiFi.h>
#include <FastLED.h>
#include <WebServer.h>

WebServer server(80);

//  WiFi 
const char* ssid = " ";
const char* password = " ";

//  LED 
#define LED_PIN 3   
#define NUM_LEDS 60

CRGB leds[NUM_LEDS];

String currentMode = "static";

int redValue = 255;
int greenValue = 255;
int blueValue = 255;

bool ledsEnabled = false;

bool manualMode = false;

unsigned long lastEffect = 0;
uint8_t hue = 0;


void setup() {
  Serial.begin(115200);
  
  Serial.println("START");

  // LED init
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  // WiFi connect
  WiFi.begin(ssid, password);

  Serial.print("Connecting");

  Serial.print("Connecting to WiFi");

 int attempts = 0;

 while (WiFi.status() != WL_CONNECTED && attempts < 20) {
  delay(500);
  Serial.print(".");
  attempts++;
 }

 if (WiFi.status() == WL_CONNECTED) {
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
 } else {
  Serial.println("\nWiFi FAILED!");
 }

  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());  

  server.on("/on", []() {
  ledsEnabled = true;
  server.send(200, "text/plain", "LED ON");
});

server.on("/off", []() {
  ledsEnabled = false;
  FastLED.clear();
  FastLED.show();
  server.send(200, "text/plain", "LED OFF");
});

server.on("/color", []() {
  manualMode = true;

  if (server.hasArg("r")) redValue = server.arg("r").toInt();
  if (server.hasArg("g")) greenValue = server.arg("g").toInt();
  if (server.hasArg("b")) blueValue = server.arg("b").toInt();

  currentMode = "static";

  server.send(200, "text/plain", "Color updated");
});

server.on("/mode", []() {
  manualMode = true;  

  if (server.hasArg("name")) {
    currentMode = server.arg("name");
  }

  server.send(200, "text/plain", "Mode updated");
});

server.on("/auto", []() {

  manualMode = false;

  server.send(200, "text/plain", "AUTO MODE");
});

server.begin();

}

void loop() {

  server.handleClient();

  if (!ledsEnabled) {

  FastLED.clear();
  FastLED.show();

  return;
}

  if (currentMode == "static") {

  fill_solid(leds, NUM_LEDS,
      CRGB(redValue, greenValue, blueValue));

  FastLED.show();
}

else if (currentMode == "rainbow") {

  if (millis() - lastEffect > 20) {

    fill_rainbow(leds, NUM_LEDS, hue++);
    FastLED.show();

    lastEffect = millis();
  }
}

else if (currentMode == "breathing") {

  static int brightness = 0;
  static int fadeAmount = 5;

  FastLED.setBrightness(brightness);

  fill_solid(leds, NUM_LEDS,
      CRGB(redValue, greenValue, blueValue));

  FastLED.show();

  brightness += fadeAmount;

  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }

  delay(30);
}

else if (currentMode == "police") {

  static bool state = false;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = state ? CRGB::Red : CRGB::Blue;
  }

  FastLED.show();

  state = !state;

  delay(150);
}

}