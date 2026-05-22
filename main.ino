#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Config
#include "config.h"

// Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DHT dht(DHT_PIN, DHT22);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

HardwareSerial simSerial(1); // UART1

bool pumpState = false;
unsigned long lastUpdate = 0;

struct SensorData {
  float soilMoisture = 0;
  float soilTemp = 0;
  float airTemp = 0;
  float humidity = 0;
  bool rainDetected = false;
};

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);        // Pump OFF

  dht.begin();
  sensors.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  simSerial.begin(9600, SERIAL_8N1, 16, 17);
  delay(3000);
  
  sendSMS("🌾 AgriGuard System Started Successfully!");
  Serial.println("🌾 AgriGuard Ready!");
}

void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    SensorData data = readSensors();
    float score = calculateIrrigationScore(data);
    
    updateDisplay(data, score);
    controlPump(score);
    checkSMSCommands();
    
    lastUpdate = millis();
  }
}

// ==================== SENSORS ====================
SensorData readSensors() {
  SensorData data;
  
  int raw = analogRead(SOIL_MOISTURE_PIN);
  data.soilMoisture = map(raw, 0, 4095, 100, 0);   // Dry=0, Wet=100 (calibrate!)

  sensors.requestTemperatures();
  data.soilTemp = sensors.getTempCByIndex(0);

  data.airTemp = dht.readTemperature();
  data.humidity = dht.readHumidity();
  data.rainDetected = (digitalRead(RAIN_PIN) == LOW);

  return data;
}

// ==================== MULTI-FACTOR ALGORITHM ====================
float calculateIrrigationScore(SensorData data) {
  float sm_norm = constrain(data.soilMoisture / 100.0, 0.0, 1.0);
  float temp_factor = 1.0 - abs(data.soilTemp - 27.0) / 50.0;   // Ideal \~27°C for most crops
  float hum_factor = data.humidity / 100.0;
  float rain_factor = data.rainDetected ? 0.0 : 1.0;

  float w_moist = 0.55, w_temp = 0.20, w_hum = 0.15, w_rain = 0.10;

  if (data.rainDetected) w_moist = 0.35;

  float total = w_moist + w_temp + w_hum + w_rain;
  w_moist /= total; w_temp /= total; w_hum /= total; w_rain /= total;

  float score = (sm_norm * w_moist) + (temp_factor * w_temp) + 
                (hum_factor * w_hum) + (rain_factor * w_rain);

  return score * 100.0;
}

void controlPump(float score) {
  if (score < SCORE_ON && !pumpState) {
    digitalWrite(RELAY_PIN, LOW);
    pumpState = true;
    sendSMS("🚰 Pump ON - Irrigation Started");
  } 
  else if (score > SCORE_OFF && pumpState) {
    digitalWrite(RELAY_PIN, HIGH);
    pumpState = false;
    sendSMS("🚰 Pump OFF - Soil Sufficient");
  }
}

void updateDisplay(SensorData data, float score) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0); display.println("AgriGuard");
  
  display.setCursor(0,12); display.print("Moist: "); display.print(data.soilMoisture,1); display.println("%");
  display.setCursor(0,24); display.print("SoilT: "); display.print(data.soilTemp,1); display.println("C");
  display.setCursor(0,36); display.print("AirT : "); display.print(data.airTemp,1); display.println("C");
  display.setCursor(0,48); display.print("Score: "); display.print(score,0); display.println("%");
  
  display.display();
}

// ==================== GSM & COMMANDS ====================
void sendSMS(String msg) {
  simSerial.println("AT+CMGF=1");
  delay(200);
  simSerial.print("AT+CMGS=\""); simSerial.print(PHONE_NUMBER); simSerial.println("\"");
  delay(200);
  simSerial.print(msg);
  simSerial.write(26);
  delay(1000);
}

void checkSMSCommands() {
  if (simSerial.available()) {
    String cmd = simSerial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.indexOf("STATUS") != -1) {
      SensorData d = readSensors();
      float s = calculateIrrigationScore(d);
      String msg = "Moist:" + String(d.soilMoisture,1) + "% | Score:" + String(s,0) + "% | Pump:" + (pumpState?"ON":"OFF");
      sendSMS(msg);
    }
    else if (cmd.indexOf("PUMP ON") != -1) {
      digitalWrite(RELAY_PIN, LOW);
      pumpState = true;
      sendSMS("Pump Manually Turned ON");
    }
    else if (cmd.indexOf("PUMP OFF") != -1) {
      digitalWrite(RELAY_PIN, HIGH);
      pumpState = false;
      sendSMS("Pump Manually Turned OFF");
    }
  }
}