#include <Arduino.h>

const int OUT_PINS[8] = { 9, 10, 11, 12, 13, 41, 44, 45 };

#define IN1_PIN  46
#define IN2_PIN  47
#define IN3_PIN  48
#define IN4_PIN  21
#define IN5_PIN  22
#define IN6_PIN  23
#define IN7_PIN  1
#define IN8_PIN  2

inline void outOn  (int idx) { digitalWrite(OUT_PINS[idx], LOW);  }
inline void outOff (int idx) { digitalWrite(OUT_PINS[idx], HIGH); }
inline void outToggle(int idx) {
  digitalWrite(OUT_PINS[idx], !digitalRead(OUT_PINS[idx]));
}

void setup() {
  Serial.begin(115200);

  // outputs
  for(int i=0;i<8;i++){
    pinMode(OUT_PINS[i], OUTPUT);
    outOff(i); // all off
  }

  // simple LED / relay test
  for(int i=0;i<8;i++){
    outOn(i);
    delay(200);
    outOff(i);
    delay(150);
  }

  Serial.println("Moto32 ready.");
}

void loop() {
  // example: OUT4 = brake light
  if(digitalRead(IN1_PIN) == LOW)
    outOn(3);
  else
    outOff(3);

  // example: OUT7 = horn
  if(digitalRead(IN2_PIN) == LOW)
    outOn(6);
  else
    outOff(6);

  delay(2);
}
