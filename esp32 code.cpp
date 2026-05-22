#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGSM.h>

// Config
#include "config.h"

// Libraries Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

DHT dht(DHT_PIN, DHT22);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

HardwareSerial simSerial(1);  // UART1 for SIM800L

// Variables
bool pumpState = false;
unsigned long lastUpdate = 0;

struct SensorData {
  float soilMoisture;
  float soilTemp;
  float airTemp;
  float humidity;
  bool  rainDetected;
};

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // Relay OFF (active LOW)

  dht.begin();
  sensors.begin();
  
  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // SIM800L
  simSerial.begin(9600, SERIAL_8N1, 16, 17);  // TX=16, RX=17
  delay(3000);
  sendSMS("AgriGuard Started Successfully!");

  Serial.println("🌾 AgriGuard Ready!");
}

void loop() {
  if (millis() - lastUpdate > 10000) {  // Every 10 seconds
    SensorData data = readSensors();
    float score = calculateIrrigationScore(data);
    
    updateDisplay(data, score);
    controlPump(score);
    checkSMSCommands();
    
    lastUpdate = millis();
  }
}

// ==================== SENSOR READING ====================
SensorData readSensors() {
  SensorData data;
  
  // Soil Moisture (0-100%)
  int raw = analogRead(SOIL_MOISTURE_PIN);
  data.soilMoisture = map(raw, 4095, 0, 0, 100);  // Calibrate as needed
  
  // Soil Temperature
  sensors.requestTemperatures();
  data.soilTemp = sensors.getTempCByIndex(0);
  
  // Air Temp & Humidity
  data.airTemp = dht.readTemperature();
  data.humidity = dht.readHumidity();
  
  // Rain Sensor
  data.rainDetected = digitalRead(RAIN_PIN) == LOW;
  
  return data;
}

// ==================== MULTI-FACTOR ALGORITHM ====================
float calculateIrrigationScore(SensorData data) {
  // Normalization
  float sm_norm = constrain(data.soilMoisture / 100.0, 0.0, 1.0);           // Lower = drier
  float temp_factor = 1.0 - abs(data.soilTemp - 25.0) / 40.0;              // Ideal \~25°C
  float humidity_factor = data.humidity / 100.0;
  float rain_factor = data.rainDetected ? 0.0 : 1.0;

  // Dynamic Weights
  float w_moisture = 0.55;
  float w_temp = 0.20;
  float w_humidity = 0.15;
  float w_rain = 0.10;

  if (data.rainDetected) {
    w_moisture -= 0.20;   // Reduce importance during rain
  }

  // Normalize weights
  float total_w = w_moisture + w_temp + w_humidity + w_rain;
  w_moisture /= total_w;
  w_temp /= total_w;
  w_humidity /= total_w;
  w_rain /= total_w;

  // Final Score (0 = needs water, 1 = sufficient)
  float score = (sm_norm * w_moisture) + 
                (temp_factor * w_temp) + 
                (humidity_factor * w_humidity) + 
                (rain_factor * w_rain);

  return score * 100;  // Return as percentage
}

void controlPump(float score) {
  if (score < SCORE_THRESHOLD_ON && !pumpState) {
    digitalWrite(RELAY_PIN, LOW);   // Pump ON
    pumpState = true;
    sendSMS("Pump TURNED ON - Low Moisture");
  } 
  else if (score > SCORE_THRESHOLD_OFF && pumpState) {
    digitalWrite(RELAY_PIN, HIGH);  // Pump OFF
    pumpState = false;
    sendSMS("Pump TURNED OFF - Sufficient Moisture");
  }
}

void updateDisplay(SensorData data, float score) {
  display.clearDisplay();
  display.setTextSize(1);
  
  display.setCursor(0, 0);
  display.print("AgriGuard");
  
  display.setCursor(0, 12);
  display.print("Moist: "); display.print(data.soilMoisture); display.print("%");
  
  display.setCursor(0, 24);
  display.print("SoilT: "); display.print(data.soilTemp); display.print("C");
  
  display.setCursor(0, 36);
  display.print("AirT: "); display.print(data.airTemp); display.print("C");
  
  display.setCursor(0, 48);
  display.print("Score: "); display.print(score); display.print("%");
  
  display.display();
}

// ==================== GSM FUNCTIONS ====================
void sendSMS(String message) {
  simSerial.println("AT+CMGF=1");
  delay(100);
  simSerial.print("AT+CMGS=\"");
  simSerial.print(PHONE_NUMBER);
  simSerial.println("\"");
  delay(100);
  simSerial.print(message);
  simSerial.write(26);  // CTRL+Z
  delay(1000);
}

void checkSMSCommands() {
  // Basic SMS command handler (expand as needed)
  // You can add STATUS, PUMP ON, PUMP OFF etc.
}