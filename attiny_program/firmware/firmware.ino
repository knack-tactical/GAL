#include <avr/interrupt.h>
#include <avr/sleep.h>
#define time_t unsigned long

// Pin Variables
int rotarySwitch = A0;
int vis = 6;
int ir = 7;
int illum = 8;
int buttonPin = 1;
int indicator = 2;

// PWM Variables
int numSteps = 8;
float threshold = 1024.0 / numSteps;

// Programming
bool programming = false;
int blinkSpeed = 100; // ms between blink state changes
bool buttonPressed = false;

// Brightness Settings
int brightPct = 100;
int dimPct = 20;
float b = brightPct / 100.0 * 255;
float d = dimPct / 100.0 * 255;
//                // VH,     VL,  O, AL,  DL,  AH,     IH,     DH,     P
//int visVal[9] =   {bright, dim, 0, 0,   0,   0,      0,      0,      0};
//int irVal[9] =    {0,      0,   0, dim, dim, bright, 0,      bright, 0};
//int illumVal[9] = {0,      0,   0, 0,   dim, 0,      bright, bright, 0};
//int ppsVal[9] =   {0,      0,   0, 0,   1,   2,      4,      8,      0};

                // P  DH  IH  AH  DL  AL  O  VL  VH
int visVal[9] =   {0, 0,  0,  0,  0,  0,  0, d,  b};
int irVal[9] =    {0, b,  0,  b,  d,  d,  0, 0,  0};
int illumVal[9] = {0, b,  b,  0,  d,  0,  0, 0,  0};
int ppsVal[9] =   {0, 8,  4,  2,  1,  0,  0, 0,  0};
int programPos = 0;

// Illum Settings
int pulsePerSec = 0;
int flashRate, flashRateBase = 1000;
float illumBrightness;
time_t timeout = 30500; // sec until button press times out
int timeout_count_limit = 10, timeout_count = 0;
time_t current_t = 0;
bool doIllumLoop = false;
time_t currentMillis, previousMillis = 0;
int illumState = illumBrightness;

// Double click variables
int dblPressInterval = 250; // max ms between clicks to enable double-press-to-hold
time_t dblDownTime = 0;
bool dblPressed = false;
time_t sleepTimeout = 300; // ms until sleep

///////////////////////////////
///        Interrupt        ///
///////////////////////////////

ISR(PCINT0_vect) {
  current_t = millis();
  noInterrupts();
  timeout_count = 0;
  if (!digitalRead(buttonPin)) { // Button Pressed
    if (!buttonPressed) { // This prevents multiple interrupts
      if (current_t - dblDownTime < dblPressInterval && current_t > dblPressInterval) { 
        dblPressed = true;
      } else {
        dblPressed = false;
      }
      dblDownTime = current_t;
      buttonPressed = true;
      digitalWrite(indicator, HIGH);
      setOutputs();
    }
  } else {
    if (programming) {
      getPulsePerSec();
      programming = false;
    }
    buttonPressed = false;
    if (!dblPressed) {
      doIllumLoop = false;
      analogWrite(vis, 0);
      analogWrite(ir, 0);
      analogWrite(illum, 0);
      digitalWrite(indicator, LOW);
    }
  }
  interrupts();
}

//////////////////////////////
///      Setup & Loop      ///
//////////////////////////////

void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(indicator, OUTPUT);
  GIMSK = 0b00010000;
  PCMSK0 = 0b00000010;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void loop() {
  if ((buttonPressed || dblPressed) && doIllumLoop ) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= flashRate) {
      previousMillis = currentMillis;
      illumState = (illumState == 0) ? illumBrightness : 0;
      analogWrite(illum, illumState);
    }
  }
  if (millis() - current_t >= timeout) {
    current_t = millis();
    timeout_count++;
  }
  if ((!buttonPressed && !dblPressed) || timeout_count >= timeout_count_limit ) {
//    analogWrite(vis, 0);
//    analogWrite(ir, 0);
//    analogWrite(illum, 0);
//    digitalWrite(indicator, LOW);
    dblPressed = false;
    if (millis() - current_t >= sleepTimeout) {
      sleep_mode();
    }
  }
}

///////////////////////////////
///      Rotary Switch      ///
///////////////////////////////

int getRotaryPos() {
  float switchValFlt = round(analogRead(rotarySwitch) / threshold);
  int val = switchValFlt;
  return val;
}

void setOutputs() {
  int pos = getRotaryPos();
  analogWrite(vis, visVal[pos]);
  analogWrite(ir, irVal[pos]);
  illumBrightness = illumVal[pos];
  if (pos == programPos) {
    analogWrite(illum, illumBrightness);
    program();
    return;
  }

  if (pulsePerSec == 0) {
    analogWrite(illum, illumBrightness);
  } else { // Set up illum flashing
    if (illumBrightness != 0) {
      doIllumLoop = true;
    }
  }
}

///////////////////////////////
///       Programming       ///
///////////////////////////////

void program() {
  programming = true;
  blinkIndicator(1, true);
  // Wait 5 sec, cancel otherwise
  interrupts();
  time_t t = millis();
  while (programming) {
    if (millis() - t >= 5000) {
      analogWrite(vis, 0);
      analogWrite(ir, 0);
      analogWrite(illum, 0);
      digitalWrite(indicator, LOW);
      pulsePerSec = 0;
      break;
    }
  }
  programming = false;
}

void getPulsePerSec() {
  pulsePerSec = ppsVal[getRotaryPos()];
  if (pulsePerSec != 0) {
    flashRate = flashRateBase / (2 * pulsePerSec);
    blinkIndicator(pulsePerSec, false);
  }
}

void blinkIndicator(int blinkNum, bool endHigh) {
  int count = 0;
  bool blinkState = digitalRead(indicator), buttonPressed_ = buttonPressed;
  interrupts();
  time_t currentMillis, previousMillis = millis();
  int stateChanges = (2*blinkNum) - endHigh + blinkState;
  while (count < stateChanges && buttonPressed_ == buttonPressed) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= blinkSpeed) {
      previousMillis = currentMillis;
      blinkState = !blinkState;
      digitalWrite(indicator, blinkState);
      count++;
    }
  }
}
