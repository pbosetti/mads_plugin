#include <ArduinoJson.h>
#define VERSION "1.0.0"
#define BAUD_RATE 115200
#define CURRENT_X A2
#define CURRENT_Y A3
#define CURRENT_Z A4
#define DELAY 50UL     // microseconds
#define TIMESTEP 500UL // milliseconds

JsonDocument doc;
String out;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);
  Serial.print("# Starting power meter v" VERSION "\n");
}

void loop() {
  static unsigned long prev_time = 0;
  static const unsigned long timestep_us = TIMESTEP * 1000;
  static const float factor = 1.0 / (1024.0 * 20.0);
  unsigned long now = micros();
  if (now - prev_time >= timestep_us) {
    doc["millis"] = millis();
    doc["AX"] = analogRead(CURRENT_X) * factor;
    doc["AY"] = analogRead(CURRENT_Y) * factor;
    doc["AZ"] = analogRead(CURRENT_Z) * factor;
    serializeJson(doc, out);
    Serial.print(out);
    Serial.print("\n");
    prev_time = now;
  }
  delayMicroseconds(DELAY);
}

