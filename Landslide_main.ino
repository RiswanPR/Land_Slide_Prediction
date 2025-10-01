#include <SoftwareSerial.h>

// GSM SIM800L
SoftwareSerial sim800l(7, 8); // RX, TX

// ESP8266 Serial
SoftwareSerial espSerial(2, 6); // RX, TX (Arduino side)

// Sensor pins
#define SOIL_A  A0
#define SOIL_D  3
#define RAIN_A  A1
#define RAIN_D  4
#define VIB_A   A2
#define VIB_D   5

// Thresholds
#define SOIL_DRY 600
#define SOIL_WET 350
#define RAIN_DETECTED 400
#define VIB_THRESHOLD 500

// Time interval for 5-minute upload
#define INTERVAL 300000UL // 5 minutes in ms

int soilA, rainA, vibA;
int soilD, rainD, vibD;
int risk = 0;

// Previous readings
int prevSoil = -1, prevRain = -1, prevVib = -1, prevRisk = -1;

// Timing
unsigned long lastUploadTime = 0;

void setup() {
  Serial.begin(9600);
  sim800l.begin(9600);
  espSerial.begin(9600);

  pinMode(SOIL_D, INPUT);
  pinMode(RAIN_D, INPUT);
  pinMode(VIB_D, INPUT);

  delay(1000);
  Serial.println("Landslide Monitoring Started...");
  //sendSMS("System Started: Monitoring Activated"); // Temporarily commented
}

void loop() {
  // Read sensors
  soilA = analogRead(SOIL_A);
  rainA = analogRead(RAIN_A);
  vibA  = analogRead(VIB_A);

  soilD = digitalRead(SOIL_D);
  rainD = digitalRead(RAIN_D);
  vibD  = digitalRead(VIB_D);

  // Calculate risk
  risk = calculateRisk(soilA, rainA, vibA, soilD, rainD, vibD);

  // Print values
  Serial.print("Soil: "); Serial.print(soilA);
  Serial.print(" | Rain: "); Serial.print(rainA);
  Serial.print(" | Vib: "); Serial.print(vibA);
  Serial.print(" | Risk: "); Serial.println(risk);

  // Send to ESP8266
  espSerial.print(soilA); espSerial.print(",");
  espSerial.print(rainA); espSerial.print(",");
  espSerial.print(vibA);  espSerial.print(",");
  espSerial.println(risk);

  // Check for significant change or alert level
  bool changed = abs(soilA - prevSoil) > 5 || abs(rainA - prevRain) > 5 || abs(vibA - prevVib) > 5 || abs(risk - prevRisk) > 5;
  bool alert = risk >= 50; // Yellow or Red alert

  if (changed || alert || (millis() - lastUploadTime >= INTERVAL)) {
    // Alert condition temporarily commented
    /*
    if (risk >= 50 && prevRisk < 50) {
      if (risk < 80) sendSMS("⚠️ Yellow Alert: Landslide Risk Moderate!");
      else sendSMS("🚨 RED ALERT: Landslide Risk HIGH!");
    }
    */

    lastUploadTime = millis();

    // Update previous readings
    prevSoil = soilA;
    prevRain = rainA;
    prevVib  = vibA;
    prevRisk = risk;
  }

  delay(500); // Small delay to avoid serial flooding
}

int calculateRisk(int soilA, int rainA, int vibA, int soilD, int rainD, int vibD) {
  int score = 0;

  // Soil contribution
  if (soilA <= SOIL_WET || soilD == LOW) score += 40;
  else if (soilA <= SOIL_DRY) score += 20;

  // Rain contribution
  if (rainA >= RAIN_DETECTED || rainD == LOW) score += 30;

  // Vibration contribution
  if (vibA >= VIB_THRESHOLD || vibD == HIGH) score += 40;

  if (score > 100) score = 100;
  return score;
}

/*
// Temporarily commented
void sendSMS(String msg) {
  sim800l.println("AT+CMGF=1"); // Text mode
  delay(500);
  sim800l.println("AT+CMGS=\"+91xxxxxxxxxx\""); // Your phone number
  delay(500);
  sim800l.print(msg);
  delay(500);
  sim800l.write(26); // Ctrl+Z
  delay(5000);
}
*/
