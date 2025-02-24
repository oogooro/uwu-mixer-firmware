#include <Arduino.h>
#include "config.h"

bool sensorsEnabled[NUM_OF_CHANNELS] = {};
uint8_t filterThreshold = 5;
uint32_t poolingRate = 30; // in Hz

void setup() {
  for (uint8_t channelPin : channelPins) {
    pinMode(channelPin, INPUT);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  Serial.print("$r");
  Serial.print(":");
  Serial.print(PROTOCOL_VERSION);
  Serial.print(":");
  Serial.print(NUM_OF_CHANNELS);
  Serial.print(":");
  Serial.print(FEATURES);
  Serial.write('\n');
}

uint16_t pots[NUM_OF_CHANNELS] = {};
uint8_t potsMapped[NUM_OF_CHANNELS] = {};
bool reversePots = false;

bool readPots() {
  bool potsChanged = false;

  for (uint8_t i = 0; i < NUM_OF_CHANNELS; i++) {
    if (!sensorsEnabled[i]) continue;
    uint16_t sensor = analogRead(channelPins[i]);
    if ((abs(pots[i] - sensor) >= filterThreshold) && (potsMapped[i] != map(sensor, 0, MAX_SENSOR_VALUE, 0, 100))) {
      pots[i] = sensor;
      potsMapped[i] = map(pots[i], 0, MAX_SENSOR_VALUE, 0, 100);
      potsChanged = true;
    }
  }
  
  return potsChanged;
}

void sendPots() {
  Serial.write('=');
  for (uint8_t i = 0; i < NUM_OF_CHANNELS; i++) {
    if (!sensorsEnabled[i]) Serial.write('-');
    else Serial.print(reversePots ? 100 - potsMapped[i] : potsMapped[i]);
    if (i != NUM_OF_CHANNELS - 1) Serial.print(':');
  }
  Serial.write('\n');
}

bool programingMode = false;
bool commandMode = false;

void loop() {
  while (Serial.available()) {
    char incoming = Serial.read();
    
    if (incoming == '#') {
      programingMode = !programingMode;
      digitalWrite(LED_BUILTIN, programingMode);

      if (!programingMode) {
        readPots();
        sendPots();
      }
      continue;
    }
    
    if (programingMode) {
      if (incoming == 'r') {
        reversePots = !reversePots;
      } else {
        uint8_t index = incoming - '0';
        if (index < NUM_OF_CHANNELS) sensorsEnabled[index] = !sensorsEnabled[index];
      }
    }

    if (incoming == '$') {
      commandMode = true;
      continue;
    }

    if (commandMode) {
      if (incoming == '\n') {
        commandMode = false;
        continue;
      } else if (incoming == 'e') {
        commandMode = false;
        Serial.write('*');
        for (uint8_t i = 0; i < NUM_OF_CHANNELS; i++) {
          Serial.write(sensorsEnabled[i] ? 'e' : 'd');
        }
        Serial.write('\n');
      }
    }
  }
  
  if (!programingMode && readPots()) sendPots();
  
  delay(1000 / poolingRate);
}