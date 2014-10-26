#define trig 9
#define echo 10
#define led1 11
#define led2 12
float soundSpeedMicrosecondsPerCm = 29.387;

void setup() {
  Serial.begin(9600);
  pinMode(trig, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(echo, INPUT);
}

void loop() {
  digitalWrite(trig, HIGH);
  delayMicroseconds(2);
  digitalWrite(trig, LOW);
  unsigned long oneWayTripTimeMicroseconds = pulseIn(echo, HIGH) / 2;
  float cm = oneWayTripTimeMicroseconds / soundSpeedMicrosecondsPerCm;
  Serial.println(cm);
  digitalWrite(led2, cm < 40 ? HIGH : LOW);
  int b;
  if      (cm <  10) b = 0;
  else if (cm <  20) b = 1;
  else if (cm <  40) b = 2;
  else if (cm < 100) b = 3;
  else if (cm < 200) b = 4;
  else if (cm < 400) b = 5;
  else               b = 20;
  blink(b);
  delay(100);
}

int delayedCycles = 0;
boolean on = false;

void blink(int delayCycles) {
  if (++ delayedCycles >= delayCycles) {
    delayedCycles = 0;
    on = ! on;
    digitalWrite(led1, on ? HIGH : LOW);
  }
}  

