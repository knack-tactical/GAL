#include <avr/interrupt.h>
#include <avr/sleep.h>
#define time_t unsigned long

/*
 *                    ATtiny84  Pinout
 *                         ______
 *                 VCC - 1|o     |14- GND
 *           (10)  PB0 - 2|      |13- PA0  (0/A0)
 *            (9)  PB1 - 3|      |12- PA1  (1/A1)
 *     RESET (11)  PB3 - 4|      |11- PA2  (2/A2)
 *            (8) PB2* - 5|      |10- PA3  (3/A3)
 *         (7/A7) PA7* - 6|      |9 - PA4  (4/A4) SCK
 *    MOSI (6/A6) PA6* - 7|______|8 - PA5* (5/A5) MISO
 *        
 */

/*
 *                  ATtiny84 Pin Map
 *  IC PIN  | ATtiny PIN  | ARDUINO PIN | SPECS           | FUNCTION
 *  1         VCC           VCC           VCC               VCC
 *  2         PB0           10            XTAL1             
 *  3         PB1           9             XTAL2             
 *  4         PB3           11            RESET             FLASH_RESET_PIN_10
 *  5         PB2           8             INT0, PWM         
 *  6         PA7           7/A7          ADC, PWM          ILLUM_PWM
 *  7         PA6           6/A6          ADC, PWM, MOSI    FLASH_MOSI_PIN11, VIS_LASER_TTL
 *  8         PA5           5/A5          ADC, PWM, MISO    FLASH_MISO_PIN12, IR_LASER_TTL
 *  9         PA4           4/A4          ADC, SCK          FLASH_SCK_PIN13, BUTTON
 *  10        PA3           3/A3          ADC               SW_8
 *  11        PA2           2/A2          ADC, AIN1         SW_4
 *  12        PA1           1/A1          ADC, AIN0         SW_2
 *  13        PA0           0/A0          ADC, AREF         SW_1
 *  14        GND           GND           GND               GND
 */


/*
 *  Functions needed:
 *  SW_1:                 digitalRead()
 *  SW_2:                 digitalRead()
 *  SW_4:                 digitalRead()
 *  SW_8:                 digitalRead()
 *  BUTTON:               interrupt (PA0-7)   PA4
 *  FLASH_SCK_PIN13:      SCK                 PA4
 *  FLASH_MISO_PIN12:     MISO                PA6
 *  FLASH_MOSI_PIN11:     MOSI                PA5
 *  FLASH_RESET_PIN_10:   RESET pin           PB3
 *  VIS_LASER_TTL:        PWM (PA5-7, PB2)    PA6
 *  IR_LASER_TTL:         PWM (PA5-7, PB2)    PA5
 *  INDICATOR:            digitalWrite()      
 *  ILLUM_PWM:            PWM (PA5-7, PB2)    PA7
 *  
 */
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
float bright = 100 / 100.0 * 255;
float illum_dim = 20 / 100.0 * 255;
float laser_dim = 5 / 100.0 * 255;

//                // VH,     VL,  O, AL,  DL,  AH,     IH,     DH,     P
//int visVal[9] =   {bright, dim, 0, 0,   0,   0,      0,      0,      0};
//int irVal[9] =    {0,      0,   0, dim, dim, bright, 0,      bright, 0};
//int illumVal[9] = {0,      0,   0, 0,   dim, 0,      bright, bright, 0};
//int ppsVal[9] =   {0,      0,   0, 0,   1,   2,      4,      8,      0};

                // P  DH  IH  AH  DL  AL  O  VL  VH
int visVal[9] =   {0, 0,  0,  0,  0,  0,  0, illum_dim,  bright};
int irVal[9] =    {0, bright,  0,  bright,  laser_dim,  laser_dim,  0, 0,  0};
int illumVal[9] = {0, bright,  bright,  0,  illum_dim,  0,  0, 0,  0};
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
