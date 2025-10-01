#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <ESP8266HTTPClient.h>

#define WIFI_SSID     "AirFiber-KXNdsC"
#define WIFI_PASSWORD "ud1eic7la9iuPhai"

// Gmail SMTP
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL    "riswanpr7amses@gmail.com"
#define AUTHOR_PASSWORD "zhfgwqkgzqjhiwgw"   // Gmail App Password
#define RECIPIENT_EMAIL "pjriyas@gmail.com"

// ThingSpeak
String server = "http://api.thingspeak.com/update";
String apiKey = "ZTZXA08FUR68KN4W";

// ESP Mail Client
SMTPSession smtp;

// Timing
#define INTERVAL 300000UL // 5 minutes
unsigned long lastUploadTime = 0;

// Previous readings to detect change
int prevSoil = -1, prevRain = -1, prevVib = -1, prevRisk = -1;

WiFiClient client;

void setup() {
  Serial.begin(9600); // Serial from Arduino
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
}

void loop() {
  // Check if data available from Arduino
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n'); // Format: soil,rain,vib,risk
    data.trim();
    if (data.length() > 0) {
      Serial.print("Received: "); Serial.println(data);

      int soil, rain, vib, risk;
      sscanf(data.c_str(), "%d,%d,%d,%d", &soil, &rain, &vib, &risk);

      bool changed = abs(soil - prevSoil) > 5 || abs(rain - prevRain) > 5 || abs(vib - prevVib) > 5 || abs(risk - prevRisk) > 5;
      bool timePassed = millis() - lastUploadTime >= INTERVAL;

      if (changed || timePassed) {
        uploadThingSpeak(soil, rain, vib, risk);

        if (risk >= 50) sendEmail(risk, soil, rain, vib);

        // Update previous readings
        prevSoil = soil;
        prevRain = rain;
        prevVib  = vib;
        prevRisk = risk;
        lastUploadTime = millis();
      }
    }
  }
}

void uploadThingSpeak(int soil, int rain, int vib, int risk) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = server + "?api_key=" + apiKey +
                 "&field1=" + String(soil) +
                 "&field2=" + String(rain) +
                 "&field3=" + String(vib) +
                 "&field4=" + String(risk);
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("ThingSpeak updated: " + String(httpCode));
    } else {
      Serial.println("ThingSpeak update failed");
    }
    http.end();
  }
}

void sendEmail(int risk, int soil, int rain, int vib) {
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  SMTP_Message message;
  message.sender.name = "Landslide Alert System";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = (risk >= 80) ? "🚨 RED ALERT: Landslide Risk HIGH" : "⚠️ YELLOW ALERT: Landslide Risk Moderate";
  message.addRecipient("Admin", RECIPIENT_EMAIL);

  String htmlMsg = "Landslide Alert!<br><br>";
  htmlMsg += "Soil Moisture: " + String(soil) + "<br>";
  htmlMsg += "Rain Level: " + String(rain) + "<br>";
  htmlMsg += "Vibration: " + String(vib) + "<br>";
  htmlMsg += "Risk: " + String(risk) + "%<br>";
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session)) {
    Serial.println("SMTP connection failed");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("Error sending Email: " + smtp.errorReason());
  } else {
    Serial.println("Email sent successfully!");
  }
}
