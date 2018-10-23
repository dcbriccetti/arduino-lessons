// Hack-o’-Lantern Arduino Pumpkin-Hacking Project
// Dave Briccetti, October, 2018
// Uses https://github.com/Martinsos/arduino-lib-hc-sr04 library for the rangefinder

#include <HCSR04.h>
#define RED 9
#define GREEN 10
#define BLUE 11
#define TRIGGER 4
#define ECHO 5
#define SPEAKER 12
#define MAX_CM 150
#define SMOOTHING_MS 1000
#define TIME_PER_MODE_MS 10000
#define COMMON_ANODE

struct Rgb {
  int red; int green; int blue;
};
const static Rgb white    = {220, 255, 255};
const static Rgb green    = {  0, 255,   0};
const static Rgb orange   = {220, 165,   0};
const static Rgb yellow   = {180, 255,   0};
const static Rgb dimYellow = { 40,  10,   0};
const static Rgb red      = {255,   0,   0};
const static Rgb blue     = {  0,   0, 255};
const static Rgb black    = {  0,   0,   0};

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

    void setColor(Rgb rgb) {
      setColor(rgb.red, rgb.green, rgb.blue);
    }
};

class HueForDistanceEffect: public Effect {  // Suggested by, and earlier implementation from, Sam Haese
  private:
    const int rgbsOfHues[25][3] = {
      {255, 0, 0}, {255, 51, 0}, {255, 102, 0}, {255, 153, 0}, {255, 204, 0}, {255, 255, 0}, {204, 255, 0}, {153, 255, 0}, {102, 255, 0}, {51, 255, 0}, {0, 255, 0}, {0, 255, 51}, {0, 255, 102}, {0, 255, 153}, {0, 255, 204}, {0, 255, 255}, {0, 204, 255}, {0, 153, 255}, {0, 102, 255}, {0, 51, 255}, {0, 0, 255}, {51, 0, 255}, {102, 0, 255}, {153, 0, 255}, {204, 0, 255}
    };
  public:
    void advance(int cm) {
      if (cm == 0) {
        setColor(black);
        noTone(SPEAKER);
        return;
      }
      const int *rgb = rgbsOfHues[map(cm, 0, MAX_CM, sizeof rgbsOfHues / sizeof rgbsOfHues[0] - 1, 0)];
      setColor(rgb[0], rgb[1], rgb[2]);
      tone(SPEAKER, map(cm, 0, MAX_CM, 2000, 1000));
    }
};

class PeriodEffect: public Effect {
  protected:
    long periodStartMs = 0;
    long periodLen = 0;

    bool periodExpired() {
      return periodStartMs + periodLen < millis();
    }
    float periodPosition() {
      return (millis() - periodStartMs) / (float) periodLen;
    }
};

class ColorsAndTonesSpeedWithClosenessEffect: public PeriodEffect {
  private:
    int numCycleColors = 0;
    Rgb * cycleColors;
    int * frequencies;
  public:
    ColorsAndTonesSpeedWithClosenessEffect(int numCycleColors, Rgb * cycleColors, int * frequencies):
      numCycleColors(numCycleColors), cycleColors(cycleColors), frequencies(frequencies) {}
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
      for (int i = 0; i < numCycleColors; ++i) {
        if (ppos < (float) (i + 1) / numCycleColors) {
          setColor(cycleColors[i]);
          tone(SPEAKER, frequencies[i]);
          break;
        }
      }
    }
};

class DimGreenEffect: public Effect {
    void advance(int cm) {
      if (cm > 0) setColor(0, map(cm, 0, MAX_CM, 255, 0), 0);
      else setColor(black);
    }
};

class ColorForDistanceEffect: public Effect {
  private:
    int numCycleColors = 0;
    Rgb * cycleColors;
    Rgb * idleColor;
  public:
    ColorForDistanceEffect(int numCycleColors, Rgb * cycleColors, Rgb * idleColor):
      numCycleColors(numCycleColors), cycleColors(cycleColors), idleColor(idleColor) {}
    void advance(int cm) {
      noTone(SPEAKER);
      bool inSegment = false;
      if (cm > 0) {
        int segDist = MAX_CM / numCycleColors;
        for (int i = 0; ! inSegment && i < numCycleColors; ++i) {
          if (cm < (i + 1) * segDist) {
            setColor(cycleColors[i]);
            inSegment = true;
          }
        }
      }
      if (! inSegment)
        setColor(*idleColor);
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

Rgb colors1[3] = {red, yellow, green};
Rgb colors2[3] = {yellow, orange, white};
int freqs[3] = {523 /* C */, 622 /* E-flat */, 740 /* F-sharp */};

Effect *effects[] = {
  new DimGreenEffect(),
  new ColorsAndTonesSpeedWithClosenessEffect(sizeof colors2 / sizeof colors2[0], colors2, freqs),
  new HueForDistanceEffect(),
  new ColorForDistanceEffect(sizeof colors1 / sizeof colors1[0], colors1, &dimYellow)
};
#define NUM_EFFECTS (sizeof effects / sizeof effects[0])

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
  if (cm > 0 && true) {
    Serial.print(cm);
    Serial.print("\n");
  }
  effects[(millis() / TIME_PER_MODE_MS) % NUM_EFFECTS]->advance(cm);
  delay(idler.delay());
}

