#include <Servo.h>
const int numServos = 2;              // number of servos
const int sweepsPerServo = 2;         // number of times each servo sweeps in the sequence
Servo servos[numServos];              // create servo objects 
int sweepTos[numServos] = {20,20};    //sweep to angle from 0 degrees
int pins[numServos] = {9,8};
int activeServo = 0;

void setup() {
  for (int i = 0; i < numServos; ++i) {
    servos[i].attach(pins[i]);
  }
}

void loop() {
  for (int i = 0; i < sweepsPerServo; ++i) {
    for (int pos = 0; pos <= 45; pos += 1) {
      servos[activeServo].write(pos);
      delay(15);
    }
    for (int pos = 45; pos >= 0; pos -= 1) {
      servos[activeServo].write(pos);
      delay(15);
    }
  }
  activeServo = (activeServo + 1) % numServos;
}
