#include "SRF05.h"
#define MAX_CM 80
#define LED 9

// trigPin, echoPin, MaxDist, readInterval
SRF05 Sensor(4, 5, MAX_CM, 100);

void setup() {
  Serial.begin(9600);
  Sensor.Unlock = true;
  pinMode(LED, OUTPUT);
}

int cm = 0;

void loop() {
  if (Sensor.Read() > -1) {
    cm = Sensor.Distance;
  }
  if (cm > 0) {
    Serial.println(cm);
    tone(12, (MAX_CM - cm) * 50);
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
    noTone(12);
  }
}

