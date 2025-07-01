#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;
ESP8266WebServer server(80);

String ssid = "";
String password = "";

void startupAnimation() {
  lcd.clear();
  String msg = "Starting Calmi AI...";
  for (int i = 0; i < msg.length(); i++) {
    lcd.setCursor(i % 16, i / 16);
    lcd.print(msg[i]);
    delay(100);
  }
  delay(1000);
  lcd.clear();
  lcd.print("WiFi Ready...");
}

void scrollText(String text) {
  int len = text.length();
  if (len <= 16) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(text);
    return;
  }

  for (int i = 0; i <= len - 16; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(text.substring(i, i + 16));
    delay(300);
  }
}

void connectToWiFi(String ssid, String password) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
  WiFi.begin(ssid.c_str(), password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    lcd.print(".");
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    Serial.println("\n✅ Connected: " + WiFi.localIP().toString());
    delay(1500);
    startupAnimation();

    if (MDNS.begin("calmi")) {
      Serial.println("✅ mDNS responder started: http://calmi.local");
      lcd.clear();
      lcd.print("mDNS: calmi.local");
    } else {
      Serial.println("❌ mDNS failed.");
    }
  } else {
    lcd.clear();
    lcd.print("❌ WiFi Failed!");
    Serial.println("\n❌ WiFi failed.");
  }
}

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Enter SSID:");

  // Read SSID from Serial Monitor
  while (ssid == "") {
    if (Serial.available()) {
      ssid = Serial.readStringUntil('\n');
      ssid.trim();
      Serial.println("✔️ SSID: " + ssid);
      lcd.clear();
      lcd.print("Enter PASS:");
    }
  }

  // Read Password from Serial Monitor
  while (password == "") {
    if (Serial.available()) {
      password = Serial.readStringUntil('\n');
      password.trim();
      Serial.println("✔️ Password received");
    }
  }

  connectToWiFi(ssid, password);

  // Setup web endpoint
  server.on("/update", []() {
    if (server.hasArg("text")) {
      String txt = server.arg("text");
      Serial.println("📥 Received: " + txt);
      scrollText(txt);
      server.send(200, "text/plain", "Displayed: " + txt);
    } else {
      server.send(400, "text/plain", "Missing 'text' param");
    }
  });

  server.begin();
  Serial.println("🌐 HTTP server started");
}

void loop() {
  server.handleClient();

  // Optional: Show Serial Monitor input on LCD
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(input);
    }
  }
}
