#include <ArduinoJson.h>
#define VERSION "1.0.0"
#define BAUD_RATE 115200
#define INPUT_PIN A2
#define DELAY 100

JsonDocument doc;
String out;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);
  Serial.print("# Starting power meter v" VERSION "\n");
}

void loop() {
  int value = analogRead(INPUT_PIN);
  doc["millis"] = millis();
  doc["current"] = value / 1024.0 * 20;
  serializeJson(doc, out);
  Serial.print(out);
  Serial.print("\n");
  delay(DELAY);
}

