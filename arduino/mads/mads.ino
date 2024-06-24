#include <ArduinoJson.h>
#define VERSION "1.1.0"
#define BAUD_RATE 115200
#define CURRENT_X A0
#define CURRENT_Y A1
#define CURRENT_Z A2
#define CURRENT_B A3
#define CURRENT_C A4
#define DELAY 40UL     // microseconds
#define TIMESTEP 160UL // milliseconds
#define DATA_FIELD "data"

#define limit(v, t, fV, fA) (((v * fV) < t ? 0 : v * fV) * fA)

JsonDocument doc;
String out;
const double to_V = 5.0 / 1024.0;
const double to_A = 20.0 / 2.8;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);
  Serial.print("# Starting power meter v" VERSION "\n");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(CURRENT_X, INPUT);
  pinMode(CURRENT_Y, INPUT);
  pinMode(CURRENT_Z, INPUT);
  pinMode(CURRENT_B, INPUT);
  pinMode(CURRENT_C, INPUT);
}

void loop() {
  static unsigned long prev_time = 0;
  static unsigned long timestep_us = TIMESTEP * 1000;
  static unsigned long delay = DELAY;
  static unsigned int threshold_mV = 280;
  static bool onoff = LOW, pause = false, raw = false;
  unsigned long now = micros();
  static unsigned long v = 0; // accumulator for serial values
  char ch;

  // Read serial in
  if (Serial.available()) {
    ch = Serial.read();
    switch (ch) {
      case '0'...'9':
        v = v * 10 + ch - '0';
        break;
      case 'p':
        timestep_us = constrain(v * 1000, 1000, 1E6);
        v = 0;
        break;
      case 'd':
        delay = constrain(v, 1, timestep_us / 10.0);
        v = 0;
        break;
      case 't':
        threshold_mV = constrain(v, 0, 5000);
        v = 0;
        break;
      case 'x':
        pause = !pause;
        break;
      case 'r':
        raw = !raw;
        break;
      case '?':
        Serial.print("Version: " VERSION "\n");
        Serial.print("Usage:\n");
        Serial.print("- 10p  set sampling period to 10 milliseconds (now ");
        Serial.print(timestep_us / 1000);
        Serial.print(" ms)\n- 30d  set loop delay to 30 microseconds (now ");
        Serial.print(delay);
        Serial.print(" us)\n- 280t set threshold to 0.28V (now ");
        Serial.print(threshold_mV);
        Serial.print(" mV)\n");
        Serial.print("- x    toggle pause\n");
        Serial.print("- r    toggle raw output\n");
        break;
      default:
        v = 0;
    }
  }

  if (pause) return;
  if (now - prev_time >= timestep_us) {
    bool active = false;
    digitalWrite(LED_BUILTIN, onoff);
    onoff = !onoff;
    doc["millis"] = millis();
    doc[DATA_FIELD]["AX"] = limit(analogRead(CURRENT_X), threshold_mV / 1000.0, to_V, to_A);
    active = active || (doc[DATA_FIELD]["AX"].as<double>() > 0);
    doc[DATA_FIELD]["AY"] = limit(analogRead(CURRENT_Y), threshold_mV / 1000.0, to_V, to_A);
    active = active || (doc[DATA_FIELD]["AY"].as<double>() > 0);
    doc[DATA_FIELD]["AZ"] = limit(analogRead(CURRENT_Z), threshold_mV / 1000.0, to_V, to_A);
    active = active || (doc[DATA_FIELD]["AZ"].as<double>() > 0);
    doc[DATA_FIELD]["AB"] = limit(analogRead(CURRENT_B), threshold_mV / 1000.0, to_V, to_A);
    active = active || (doc[DATA_FIELD]["AB"].as<double>() > 0);
    doc[DATA_FIELD]["AC"] = limit(analogRead(CURRENT_C), threshold_mV / 1000.0, to_V, to_A);
    active = active || (doc[DATA_FIELD]["AC"].as<double>() > 0);
    if (active > 0) {
      if (raw) {
        Serial.print(doc[DATA_FIELD]["AX"].as<double>());
        Serial.print(" "); 
        Serial.print(doc[DATA_FIELD]["AY"].as<double>());
        Serial.print(" "); 
        Serial.print(doc[DATA_FIELD]["AZ"].as<double>());
        Serial.print(" "); 
        Serial.print(doc[DATA_FIELD]["AB"].as<double>());
        Serial.print(" "); 
        Serial.print(doc[DATA_FIELD]["AC"].as<double>());
        Serial.print("\n");
      } else {
        serializeJson(doc, out);
        Serial.print(out);
        Serial.print("\n");
      }
    }
    prev_time = now;
  }
  delayMicroseconds(delay);
}

