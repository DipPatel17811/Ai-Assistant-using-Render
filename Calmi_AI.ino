#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;  // Auto I2C address detection
const int LCD_COLS = 16;
const int LCD_ROWS = 2;

String ssid, password;
const String serverURL = "https://calmi-ai.onrender.com/ask?q=";
String userInput = "";
String fullAnswer = "";
unsigned long scrollTimer = 0;
int scrollIndex = 0;
bool scrolling = false;

void setup() {
  Serial.begin(115200);
  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.clear();
  lcd.print("Enter SSID:");
  waitForSerialInput(ssid);
  lcd.clear();
  lcd.print("Enter PASS:");
  waitForSerialInput(password);

  connectWiFi();
  lcd.clear();
  lcd.print("Ask Gemini:");
}

void loop() {
  if (Serial.available()) {
    userInput = Serial.readStringUntil('\n');
    userInput.trim();

    if (userInput.length() > 0) {
      lcd.clear();
      lcd.print("Thinking...");
      fullAnswer = askGemini(userInput);
      scrollIndex = 0;
      scrolling = true;
      scrollTimer = millis();
    }
  }

  // Scroll long messages
  if (scrolling && millis() - scrollTimer > 500) {
    scrollTimer = millis();
    if (scrollIndex < fullAnswer.length()) {
      lcd.clear();
      lcd.print(fullAnswer.substring(scrollIndex, scrollIndex + LCD_COLS));
      if (fullAnswer.length() - scrollIndex > LCD_COLS) {
        lcd.setCursor(0, 1);
        lcd.print(fullAnswer.substring(scrollIndex + LCD_COLS, scrollIndex + LCD_COLS * 2));
      }
      scrollIndex++;
    } else {
      scrolling = false;
      lcd.clear();
      lcd.print("Ask Gemini:");
    }
  }
}

void waitForSerialInput(String &inputVar) {
  inputVar = "";
  while (inputVar.length() == 0) {
    if (Serial.available()) {
      inputVar = Serial.readStringUntil('\n');
      inputVar.trim();
    }
  }
}

void connectWiFi() {
  lcd.clear();
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid.c_str(), password.c_str());

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries++ < 30) {
    delay(500);
    lcd.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("WiFi Connected!");
    delay(1000);
  } else {
    lcd.clear();
    lcd.print("WiFi Failed!");
    while (true);
  }
}

String askGemini(String question) {
  HTTPClient http;
  String fullURL = serverURL + urlencode(question);
  http.begin(fullURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Raw: " + payload);

    int start = payload.indexOf("{\"response\":\"") + 13;
    int end = payload.lastIndexOf("\"}");

    if (start > 12 && end > start) {
      String answer = payload.substring(start, end);
      answer.replace("\\n", " ");
      answer.replace("\\\"", "\"");
      answer.trim();
      return answer;
    }
    return "Parse error!";
  } else {
    return "HTTP error " + String(httpCode);
  }

  http.end();
}

String urlencode(String str) {
  String encoded = "";
  char c;
  char buf[4];
  for (size_t i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      sprintf(buf, "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}
