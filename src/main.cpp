#include <Arduino.h>
#include "config.h"

bool sensorsEnabled[NUM_ANALOG_INPUTS] = {};
uint8_t filterThreshold = 5;

void setup() {
  for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
    pinMode(A0 + i, INPUT);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  Serial.print("$r");
  Serial.print(":");
  Serial.print(PROTOCOL_VERSION);
  Serial.print(":");
  Serial.print(NUM_ANALOG_INPUTS);
  Serial.print(":");
  Serial.print(FEATURES);
  Serial.write('\n');
}

uint16_t pots[NUM_ANALOG_INPUTS] = {};
uint8_t potsMapped[NUM_ANALOG_INPUTS] = {};

bool readPots() {
  bool potsChanged = false;

  for (uint8_t i = 0; i < NUM_ANALOG_INPUTS; i++) {
    if (!sensorsEnabled[i]) continue;
    uint16_t sensor = analogRead(A0 + i);
    if ((abs(pots[i] - sensor) >= filterThreshold) && (potsMapped[i] != map(sensor, 0, MAX_SENSOR_VALUE, 0, 100))) {
      pots[i] = sensor;
      potsMapped[i] = map(pots[i], 0, MAX_SENSOR_VALUE, 0, 100);
      potsChanged = true;
    }
  }
  
  return potsChanged;
}

bool programingMode = false;

void loop() {
  while (Serial.available()) {
    char incoming = Serial.read();
    
    if (incoming == '#') {
      programingMode = !programingMode;
      digitalWrite(LED_BUILTIN, programingMode);
      break;
    }
    
    if (programingMode) {
      uint8_t index = incoming - '0';

      if (index < NUM_ANALOG_INPUTS - 1) sensorsEnabled[index] = !sensorsEnabled[index];
    }
  }
  
  if (!programingMode && readPots()) {
    Serial.write('=');
    for (uint8_t i = 0; i < NUM_ANALOG_INPUTS; i++) {
      if (!sensorsEnabled[i]) Serial.write('-');
      else Serial.print(potsMapped[i]);
      if (i != NUM_ANALOG_INPUTS - 1) Serial.print(':');
    }
    Serial.write('\n');
  }
  
  delay(1);
}