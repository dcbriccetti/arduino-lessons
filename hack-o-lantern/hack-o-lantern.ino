// Hack-o’-Lantern Arduino Pumpkin-Hacking Project
// Dave Briccetti, October, 2018
// Uses https://github.com/Martinsos/arduino-lib-hc-sr04 library for the rangefinder

#include <HCSR04.h>
#define RED 6
#define GREEN 10
#define BLUE 3
#define TRIGGER 4
#define ECHO 5
#define SPEAKER 12
#define MAX_CM 150
#define SMOOTHING_MS 1000

// Abstract base class for multicolor LED effects
class Effect {
  public:
    virtual void advance(int cm) = 0;

    void setColor(int r, int g, int b) {
      analogWrite(RED, r);
      analogWrite(GREEN, g);
      analogWrite(BLUE, b);
    }
};

class HueEffect: public Effect {  // Suggested by, and earlier implementation from, Sam Haese
  private:
    const int rgbsOfHues[28][3] = {
      {255, 0, 0}, {255, 51, 0}, {255, 102, 0}, {255, 153, 0}, {255, 204, 0}, {255, 255, 0}, {204, 255, 0}, {153, 255, 0}, {102, 255, 0}, {51, 255, 0}, {0, 255, 0}, {0, 255, 51}, {0, 255, 102}, {0, 255, 153}, {0, 255, 204}, {0, 255, 255}, {0, 204, 255}, {0, 153, 255}, {0, 102, 255}, {0, 51, 255}, {0, 0, 255}, {51, 0, 255}, {102, 0, 255}, {153, 0, 255}, {204, 0, 255}, {255, 0, 255}, {255, 0, 204}, {255, 0, 153}
    };
  public:
    void advance(int cm) {
      if (cm == 0) {
        setColor(0, 0, 0);
        return;
      }
      const int *rgb = rgbsOfHues[map(cm, 0, MAX_CM, 0, sizeof rgbsOfHues / sizeof rgbsOfHues[0] - 1)];
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

// Contributed by Jared Solomon
class RedBlueBlinkEffect: public Effect {
  public:
    void advance(int cm) {
      if (cm > 0 && cm <= 50) {
        setColor(255, 0, 0);
        int d = map(cm, 0, MAX_CM, 10, 200);
        delay(d);
        setColor(0, 0, 255);
        delay(d);
        setColor(0, 0, 0);
      }
      if (cm > 150) {
        setColor(0, 0, 0);
      }
    }
};

UltraSonicDistanceSensor distanceSensor(TRIGGER, ECHO);

class DistanceSmoother {
  private:
    int lastSd = 0;
    long lastSdMillis = 0;

  public:
    int smoothedDistance() {
      int cm = (int) distanceSensor.measureDistanceCm();
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

class SoundMaker {
  public:
    virtual void play(int distanceCm) = 0;
};

class PitchFromDistanceSoundMaker: public SoundMaker {
    void play(int cm) {
      if (cm > 0)
        tone(SPEAKER, map(cm, 0, MAX_CM, 1000, 100));
      else noTone(SPEAKER);
    }
};

class ArpeggioSoundMaker: public SoundMaker {
    void play(int cm) {
      if (cm > 0) {
        const int noteLength = map(cm, 0, MAX_CM, 50, 500);
        tone(SPEAKER, 262); // C 4
        delay(noteLength);
        tone(SPEAKER, 311); // E-flat 4
        delay(noteLength);
        tone(SPEAKER, 524); // C 5
        delay(noteLength);
        noTone(SPEAKER);
        delay(2 * noteLength);
      }
      else noTone(SPEAKER);
    }
};

Effect *effect = new RedBlueBlinkEffect();
SoundMaker *soundMaker = new ArpeggioSoundMaker(); // PitchFromDistanceSoundMaker();
DistanceSmoother distanceSmoother = DistanceSmoother();

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int cm = distanceSmoother.smoothedDistance();
  if (cm > 0) {
    Serial.print(cm);
    Serial.print("\n");
  }
  effect->advance(cm);
  soundMaker->play(cm);
  delay(200);
}

