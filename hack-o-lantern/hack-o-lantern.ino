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
#define MAX_CM 200
#define SMOOTHING_MS 1000
#define COMMON_ANODE

const static int white    [3] = {220, 255, 255};
const static int green    [3] = {  0, 255,   0};
const static int orange   [3] = {220, 165,   0};
const static int yellow   [3] = {180, 255,   0};
const static int dimYellow[3] = { 40,  10,   0};
const static int red      [3] = {255,   0,   0};
const static int blue     [3] = {  0,   0, 255};
const static int black    [3] = {  0,   0,   0};

// Abstract base class for multicolor LED effects
class Effect {
  public:
    virtual void advance(int cm) = 0;
    void setColor(int red, int green, int blue) {
#ifdef COMMON_ANODE
      red = 255 - red;
      green = 255 - green;
      blue = 255 - blue;
#endif
      analogWrite(RED, red);
      analogWrite(GREEN, green);
      analogWrite(BLUE, blue);
    }

    void setColor(int rgb[3]) {
      setColor(rgb[0], rgb[1], rgb[2]);
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
        setColor(black);
        return;
      }
      const int *rgb = rgbsOfHues[map(cm, 0, MAX_CM, 0, sizeof rgbsOfHues / sizeof rgbsOfHues[0] - 1)];
      setColor(rgb[0], rgb[1], rgb[2]);
    }
};

class VariableBlinkRateEffect: public Effect {
  private:
    long lastStateChangeMs = 0;
    bool on = true;
  public:
    void advance(int cm) {
      if (cm == 0) {
        on = false;
        setColor(black);
        return;
      }
      const int t = map(cm, 0, MAX_CM, 0, 1000);
      if (millis() > lastStateChangeMs + t) {
        on = !on;
        lastStateChangeMs = millis();
      }
      if (on) setColor(yellow);
      else setColor(black);
    }
};

class PeriodEffect: public Effect {
  protected:
    long periodStartMs = 0;
    long periodLen = 0;

    bool periodExpired() { return periodStartMs + periodLen < millis(); }
    float periodPosition() { return (millis() - periodStartMs) / (float) periodLen; }
};

class VariableRgbEffect: public PeriodEffect {
  private:
    int *c1; int *c2; int *c3; 
  public:
    VariableRgbEffect(int *c1, int *c2, int *c3): c1(c1), c2(c2), c3(c3) {}
    void advance(int cm) {
      if (cm == 0) {
        periodStartMs = 0;
        setColor(black);
        noTone(SPEAKER);
        return;
      }
      if (periodExpired()) {
        periodStartMs = millis();
        periodLen = map(cm, 0, MAX_CM, 10, 5000);
      }
      const float ppos = periodPosition();
      if (ppos < 0.33) {
        setColor(c1);
        tone(SPEAKER, 523); // C
      } else if (ppos < 0.66) {
        setColor(c2);
        tone(SPEAKER, 622); // E-flat
      } else {
        setColor(c3);
        tone(SPEAKER, 740); // F-sharp
      }
    }
};

class DimGreenEffect: public Effect {
    void advance(int cm) {
      if (cm > 0) {
        const int v = map(cm, 0, MAX_CM, 255, 0);
        setColor(0, v, 0);
      } else setColor(black);
    }
};

class GreenYellowRedEffect: public Effect {
  public:
    void advance(int cm) {
      if (cm > 0) {
        if (cm > MAX_CM * 2 / 3)
          setColor(green);
        else if (cm > MAX_CM / 3)
          setColor(yellow);
        else
          setColor(red);
      } else
        setColor(dimYellow);
    }
};

UltraSonicDistanceSensor distanceSensor(TRIGGER, ECHO);

class DistanceSmoother {
  private:
    int lastSd = 0;
    long lastSdMillis = 0;

  public:
    int smoothedDistance() {
      int dcm = (int) distanceSensor.measureDistanceCm();
      int cm = dcm == -1 || dcm > MAX_CM ? 0 : dcm;
      if (cm > 0) {
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

// Calculates inter-ping delay from inactivity length
class Idler {
  private:
    long lastActivityMs = millis();
    int MIN_DELAY = 10;
    int MAX_DELAY = 1000;
    int SLEEP_AFTER = 30000;
  public:
    void update(int cm) {
      if (cm > 0) lastActivityMs = millis();
    }
    int delay() {
      return millis() - lastActivityMs > SLEEP_AFTER ? MAX_DELAY : MIN_DELAY;
    }
};

Effect *effect = new VariableRgbEffect(yellow, orange, white);
DistanceSmoother distanceSmoother = DistanceSmoother();
Idler idler = Idler();

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int cm = distanceSmoother.smoothedDistance();
  idler.update(cm);
  if (cm > 0 && false) {
    Serial.print(cm);
    Serial.print("\n");
  }
  effect->advance(cm);
  delay(idler.delay());
}

