#include "SRF05.h"
#define TRIGGER 4
#define ECHO 5
#define LED 6
#define SPEAKER 12
#define MAX_CM 100
#define PLAYMODE_CONTINUOUS 1
#define PLAYMODE_ARPEGGIO 2

SRF05 Sensor(TRIGGER, ECHO, MAX_CM, 0);

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  Sensor.Unlock = true;
}

void loop() {
  Sensor.Read();
  float cm = Sensor.Distance;
  int potValue = 500; // analogRead(A0);
  if (cm > 0 && cm < MAX_CM) {
    Serial.print(cm);
    Serial.print(" ");
    Serial.println(potValue);
    play(PLAYMODE_ARPEGGIO, cm, potValue);
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
    noTone(SPEAKER);
  }
  delay(10);
}

void play(int mode, float cm, int potValue) {
  int noteLength;
  const int maxNoteLength = 200;
  
  switch (mode) {
    case PLAYMODE_CONTINUOUS:
      tone(SPEAKER, (MAX_CM - cm) * potValue / 50);
      break;
      
    case PLAYMODE_ARPEGGIO:
      noteLength = map(cm, 0, MAX_CM, 20, maxNoteLength);
      tone(SPEAKER, 262); // C 4
      delay(noteLength);
      tone(SPEAKER, 311); // E-flat 4
      delay(noteLength);
      noTone(SPEAKER);
      delay(2 * noteLength);
  }
}

