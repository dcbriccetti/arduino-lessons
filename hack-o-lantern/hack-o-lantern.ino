#include "SRF05.h"
#define RED 6
#define GREEN 10
#define BLUE 3
#define TRIGGER 4
#define ECHO 5
#define SPEAKER 12
#define MAX_CM 150
#define PLAYMODE_CONTINUOUS 1
#define PLAYMODE_ARPEGGIO 2
#define SMOOTHING_MS 1500

SRF05 Sensor(TRIGGER, ECHO, MAX_CM, 0);

class Effect {
  public:
    virtual void advance(int cm) = 0;

    void setColor(int r, int g, int b) {
      analogWrite(RED, r);
      analogWrite(GREEN, g);
      analogWrite(BLUE, b);
    }
};

class HueEffect: public Effect {  // Suggested by and first implemention by Sam Haese
  private:
    const int rgbs[28][3] = {
{255, 0, 0}, {255, 51, 0}, {255, 102, 0}, {255, 153, 0}, {255, 204, 0}, {255, 255, 0}, {204, 255, 0}, {153, 255, 0}, {102, 255, 0}, {51, 255, 0}, {0, 255, 0}, {0, 255, 51}, {0, 255, 102}, {0, 255, 153}, {0, 255, 204}, {0, 255, 255}, {0, 204, 255}, {0, 153, 255}, {0, 102, 255}, {0, 51, 255}, {0, 0, 255}, {51, 0, 255}, {102, 0, 255}, {153, 0, 255}, {204, 0, 255}, {255, 0, 255}, {255, 0, 204}, {255, 0, 153}
    };
  public:
    void advance(int cm) {
      if (cm == 0) {
        setColor(0, 0, 0);
        return;
      }
      const int *rgb = rgbs[map(cm, 0, MAX_CM, 0, sizeof rgbs / sizeof rgbs[0] - 1)];
      setColor(rgb[0], rgb[1], rgb[2]);
    }
};

class VbrEffect: public Effect {
  private:
    long lastStateChangeMs = 0;
    bool on = true;
  public:
    void advance(int cm) {
      if (cm == 0) {
        on = false;
        return;
      }
      const int t = map(cm, 0, MAX_CM, 0, 1000);
      if (millis() > lastStateChangeMs + t) {
        on = !on;
        lastStateChangeMs = millis();
      }
      if (on) {
        setColor(150, 250, 0);
      } else {
        setColor(0, 0, 0);
      }
    }
};

class DimGreenEffect: public Effect {
    void advance(int cm) {
      if (cm > 0) {
        const int v = map(cm, 0, MAX_CM, 255, 0);
        setColor(0, v, 0);
      } else {
        setColor(0, 0, 0);
      }
    }
};

class GreenYellowRedEffect: public Effect {
  public:
    void advance(int cm) {
      if (cm > 0 && cm <= MAX_CM) {
        if (cm > MAX_CM * 2 / 3)
          setColor(0, 255, 0);
        else if (cm > MAX_CM / 3)
          setColor(150, 255, 0);
        else
          setColor(255, 0, 0);
      } else {
        setColor(40, 20, 0);
      }
    }
};

class DistanceSmoother {
  private:
    int lastSd = 0;
    long lastSdMillis = 0;
  public:
    int smoothedDistance() {
      Sensor.Read();
      int cm = (int) Sensor.Distance;
      if (cm > 0 && cm <= MAX_CM) {
        lastSd = cm;
        lastSdMillis = millis();
      } else {
        if (millis() - lastSdMillis < SMOOTHING_MS) {
          return lastSd;
        }
      }
      return cm;
    }
};

Effect *effect = new HueEffect();
DistanceSmoother distanceSmoother = DistanceSmoother();

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  Sensor.Unlock = true;
  Serial.begin(9600);
}

void loop() {
  int cm = distanceSmoother.smoothedDistance();
  if (cm > 0) {
    Serial.print(cm);
    Serial.print("\n");
  }
  effect->advance(cm);

  int potValue = 500; // analogRead(A0);
  if (cm > 0) {
    play(PLAYMODE_ARPEGGIO, cm, potValue);
    const int v = map(cm, 0, MAX_CM, 255, 0);
  } else {
    noTone(SPEAKER);
  }
  delay(200);
}

void play(int mode, float cm, int potValue) {
  int noteLength;
  const int maxNoteLength = 200;

  switch (mode) {
    case PLAYMODE_CONTINUOUS:
      tone(SPEAKER, (MAX_CM - cm) * potValue / 50);
      break;

    case PLAYMODE_ARPEGGIO:
      noteLength = map(cm, 0, MAX_CM, 200, 500); // maxNoteLength);
      tone(SPEAKER, 262); // C 4
      delay(noteLength);
      //tone(SPEAKER, 311); // E-flat 4
      tone(SPEAKER, 262 * 2); // C 5
      delay(noteLength);
      noTone(SPEAKER);
      delay(2 * noteLength);
  }
}

