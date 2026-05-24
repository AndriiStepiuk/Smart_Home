#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <FastLED.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_sleep.h>

const char* ssid = "";
const char* password = "";

const char* serverURL = "http://  :5000/data";  
const char* ledServer = "http://  ";  


// функція відпраки Wi-Fi 
void sendDataToServer(float temp, float hum, float pres, float co2, float nh3, float smoke, float alcohol, float benzene, float light)  {

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"temperature\":" + String(temp) + ",";
    json += "\"humidity\":" + String(hum) + ",";
    json += "\"pressure\":" + String(pres) + ",";
    json += "\"co2\":" + String(co2) + ",";
    json += "\"nh3\":" + String(nh3) + ",";
    json += "\"smoke\":" + String(smoke) + ",";
    json += "\"alcohol\":" + String(alcohol) + ",";
    json += "\"benzene\":" + String(benzene) + ",";
    json += "\"light\":" + String(light);
    json += "}";

    int httpResponseCode = http.POST(json);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}
//  режим сну 
void setupWakeup() {

  // пробудження кнопкою
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, 0);

  // пробудження PIR
  esp_sleep_enable_ext1_wakeup(
      (1ULL << GPIO_NUM_25),
      ESP_EXT1_WAKEUP_ANY_HIGH
  );

  // пробудження по таймеру
  esp_sleep_enable_timer_wakeup(10ULL * 1000000ULL);
}


void sendLEDCommand(String cmd) {

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String url = String(ledServer) + "/" + cmd;

    http.begin(url);
    int httpResponseCode = http.GET();

    Serial.print("LED HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}

 
#define LED_STRIP_PIN 27 
#define NUM_LEDS 60 
CRGB leds[NUM_LEDS];


//  PIR 
#define PIR_PIN 25

//  OLED 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//  BMP280 
Adafruit_BMP280 bmp;

//  DHT11 
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define MQ135_PIN 34

float RLOAD = 10.0;
float R0 = 76.63;

float getResistance() {

  int adc = analogRead(MQ135_PIN);
  float voltage = adc * (3.3 / 4095.0);

  float Rs = ((3.3 - voltage) / voltage) * RLOAD;

  return Rs;
}

float getRatio() {
  return getResistance() / R0;
}

float getCO2() {
  float ratio = getRatio();
  return 116.6020682 * pow(ratio, -2.769034857);
}

float getNH3() {
  float ratio = getRatio();
  return 102.2 * pow(ratio, -2.473);
}

float getAlcohol() {
  float ratio = getRatio();
  return 77.255 * pow(ratio, -3.18);
}

float getBenzene() {
  float ratio = getRatio();
  return 44.947 * pow(ratio, -3.445);
}

float getSmoke() {
  float ratio = getRatio();
  return 34.668 * pow(ratio, -3.369);
}

// LDR 
#define LDR_PIN 32

// BUTTON 
#define BUTTON_PIN 14

//  VARIABLES 
int screenIndex = 0;
const int totalScreens = 4;

unsigned long lastButtonPress = 0;
const unsigned long debounceTime = 200;

bool lastButtonState = HIGH;

bool ledState = false;
unsigned long lastSend = 0;
//--------------------------------------------------------------------------------------SETUP---------------------------------------------------------------------------------------------
void setup() {

  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(21, 22);
   
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED error");
    return;
  }
  
  if (!bmp.begin(0x76)) {  
    Serial.println("BMP280 error"); 
    return;
  } 
  
 FastLED.addLeds<WS2812, LED_STRIP_PIN, GRB>(leds, NUM_LEDS); 
 FastLED.clear(); 
 FastLED.show();
  
  pinMode(PIR_PIN, INPUT);
  dht.begin();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);


  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }

  Serial.println("WiFi connected"); 
}


 void handleButton() {

  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState == LOW && lastButtonState == HIGH) {
    if (millis() - lastButtonPress > debounceTime) {
      screenIndex++;
      if (screenIndex >= totalScreens) {
        screenIndex = 0;
      }
      lastButtonPress = millis();
    }
  }

  lastButtonState = currentState;
}

void showScreen() {

  display.clearDisplay();
  display.setCursor(0, 0);

  switch (screenIndex) {

    case 0:
      showDHT();
      break;

    case 1:
      showBMP();
      break;

    case 2:
      showMQ135();
      break;

    case 3:
      showLDR();
      break;
  }

  display.display();
}


//  PIR 
void handlePIR() {
  

  int ldrValue = analogRead(LDR_PIN);
  float lightPercent = 100.0 - (ldrValue / 4095.0) * 100.0;

  bool isDark = lightPercent < 35;
  int motion = digitalRead(PIR_PIN);

  if (isDark && motion == HIGH) {

    if (!ledState) {
      Serial.println("Motion detected");

      sendLEDCommand("on");   // 🔥 WiFi

      ledState = true;
    }

  } else {

    if (ledState) {
      sendLEDCommand("off");  // 🔥 WiFi
      ledState = false;
    }

  }
}

// DHT11 
void showDHT() {

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  display.println("DHT11");
  display.println("----------------");

  if (isnan(t) || isnan(h)) {
    display.println("Sensor error");
    Serial.println("DHT11 Sensor error");
    return;
  }

  display.print("Temp: ");
  display.print(t);
  display.println(" C");

  display.print("Hum:  ");
  display.print(h);
  display.println(" %");

  // --- Serial Output ---
  // Serial.println("DHT11");
  // Serial.print("Temperature: "); Serial.print(t); Serial.println(" C");
  // Serial.print("Humidity: "); Serial.print(h); Serial.println(" %");
  // Serial.println("----------------");
}

//  BMP280 
void showBMP() {

  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0;
  float altitude = bmp.readAltitude(1013.25);

  display.println("BMP280");
  display.println("----------------");

  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  display.print("Pres: ");
  display.print(pressure);
  display.println(" hPa");

  display.print("Alt: ");
  display.print(altitude);
  display.println(" m");

  // --- Serial ---
  // Serial.println("BMP280");
  // Serial.print("Temperature: "); Serial.println(temperature);
  // Serial.print("Pressure: "); Serial.println(pressure);
  // Serial.print("Altitude: "); Serial.println(altitude);
  // Serial.println("----------------");
}
// MQ135 
  void showMQ135() {

  float co2 = getCO2();
  float nh3 = getNH3();
  float alcohol = getAlcohol();
  float benzene = getBenzene();
  float smoke = getSmoke();

  display.println("Air Quality");
  display.println("----------------");

  display.print("CO2: ");
  display.print(co2,0);
  display.println(" ppm");

  display.print("NH3: ");
  display.print(nh3,0);
  display.println(" ppm");

  display.print("smoke: ");
  display.print(smoke,0);
  display.println(" ppm");

  display.print("alco: ");
  display.print(alcohol,0);
  display.println(" ppm");

  display.print("benz: ");
  display.print(benzene,0);
  display.println(" ppm");
}

// LDR 
void showLDR() {
  
  int value = analogRead(LDR_PIN);
  float percent = 100.0 - (value / 4095.0) * 100.0;

  display.println("Light Sensor");
  display.println("----------------");

  display.print("ADC: ");
  display.println(value);

  display.print("Light: ");
  display.print(percent);
  display.println(" %");

  // --- Serial Output ---
  // Serial.println("LDR");
  // Serial.print("ADC Value: "); Serial.println(value);
  // Serial.print("Light: "); Serial.print(percent); Serial.println(" %");
  // Serial.println("----------------");
}
//----------------------------------------------------------------------------------LOOP-------------------------------------------------------------------------------------------------
void loop() {

  handleButton();
  handlePIR();
  showScreen();

  //  період надсилання даних
   if (millis() - lastSend > 60000) { 

   float t = dht.readTemperature();
   float h = dht.readHumidity();
   float p = bmp.readPressure() / 100.0;
   float co2 = getCO2();
   float nh3 = getNH3();
   float smoke = getSmoke();
   float alcohol = getAlcohol();
   float benzene = getBenzene();
   float light = analogRead(LDR_PIN);

   sendDataToServer(t, h, p, co2, nh3, smoke, alcohol, benzene, light);

   Serial.println("Going to sleep...");
   delay(1000);

   display.ssd1306_command(SSD1306_DISPLAYOFF);
   WiFi.disconnect(true);
   WiFi.mode(WIFI_OFF);

   setupWakeup();
   
   esp_deep_sleep_start();
   } 

  }