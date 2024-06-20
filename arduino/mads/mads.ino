#include <ArduinoJson.h>
#define VERSION "1.0.0"
#define BAUD_RATE 115200
#define CURRENT_X A0
#define CURRENT_Y A1
#define CURRENT_Z A2
#define CURRENT_B A3
#define CURRENT_C A4
#define DELAY 50UL     // microseconds
#define TIMESTEP 160UL // milliseconds
#define DATA_FIELD "data"

JsonDocument doc;
String out;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);
  Serial.print("# Starting power meter v" VERSION "\n");
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  static unsigned long prev_time = 0;
  static const unsigned long timestep_us = TIMESTEP * 1000;
  static const float factor = 1.0 / (1024.0 * 20.0);
  static bool onoff = LOW;
  unsigned long now = micros();
  if (now - prev_time >= timestep_us) {
    digitalWrite(LED_BUILTIN, onoff);
    onoff = !onoff;
    doc["millis"] = millis();
    doc[DATA_FIELD]["AX"] = analogRead(CURRENT_X) * factor;
    doc[DATA_FIELD]["AY"] = analogRead(CURRENT_Y) * factor;
    doc[DATA_FIELD]["AZ"] = analogRead(CURRENT_Z) * factor;
    doc[DATA_FIELD]["AB"] = analogRead(CURRENT_B) * factor;
    doc[DATA_FIELD]["AC"] = analogRead(CURRENT_C) * factor;
    serializeJson(doc, out);
    Serial.print(out);
    Serial.print("\n");
    prev_time = now;
  }
  delayMicroseconds(DELAY);
}

