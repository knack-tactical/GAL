/*  The code for the GALv3
 *  by KNACK
 */

#include <avr/interrupt.h>
#include <avr/sleep.h>
#define time_t unsigned long
#define round(N) ((N) >=0)? (int)((N)+0.5) : (int)((N)-0.5)

/*                    ATtiny84  Pinout
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

/*                   ATtiny84 Pin Map
 *  IC PIN  | ATtiny PIN  | ARDUINO PIN | SPECS           | FUNCTION
 *  1         VCC           VCC           VCC               VCC
 *  2         PB0           10            XTAL1             N/C
 *  3         PB1           9             XTAL2             N/C
 *  4         PB3           11            RESET             FLASH_RESET_PIN_10
 *  5         PB2           8             INT0, PWM         INDICATOR
 *  6         PA7           7/A7          ADC, PWM          ILLUM_PWM
 *  7         PA6           6/A6          ADC, PWM, MOSI    FLASH_MOSI_PIN11, VIS_LASER_TTL
 *  8         PA5           5/A5          ADC, PWM, MISO    FLASH_MISO_PIN12, IR_LASER_TTL
 *  9         PA4           4/A4          ADC, SCK          FLASH_SCK_PIN13, BUTTON
 *  10        PA3           3/A3          ADC               SW_2
 *  11        PA2           2/A2          ADC, AIN1         SW_8
 *  12        PA1           1/A1          ADC, AIN0         SW_1
 *  13        PA0           0/A0          ADC, AREF         SW_4
 *  14        GND           GND           GND               GND
 */


/*  Functions needed:
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
 *  INDICATOR:            digitalWrite()      PB2
 *  ILLUM_PWM:            PWM (PA5-7, PB2)    PA7
 */


// Pin Variables 
#define SW_1            1
#define SW_2            3
#define SW_4            0
#define SW_8            2
// #define VIS_PIN         6
#define IR_PIN          5
#define ILLUM_PIN       7
#define BUTTON_PIN      4
#define INDICATOR_PIN   8

// Program Constants
#define SHUTOFF_TIMEOUT       30500 // ms until button press timeout counter increments | ~30s
#define TIMEOUT_COUNT_LIMIT   10    // number of timeout counter increments until shutdown | 10 x 30s = 5min
#define BLINK_SPEED_SLOW      45    // ms between blink state changes
#define SLEEP_TIMEOUT         500   // ms delay until allowed to sleep
#define LOW_BATTERY_MV        2625  // Maximum level for a low battery warning in mV (accounting for voltage drop)
#define DBL_PRESS_MAX         300   // max ms between clicks to enable double-press-to-hold
#define DBL_PRESS_MIN         25    // min ms between clicks to enable double-press-to-hold (debounce)

#define ILLUM_HIGH  100   // % of power for the illuminator high mode
#define ILLUM_MED   50    // % of power for the illuminator medium mode
#define ILLUM_LOW   20    // % of power for the illuminator low mode
#define LASER_HIGH  100   // % of power for the laser high mode (5mW)
#define LASER_MED   50    // % of power for the laser medium mode (2.5mW)
#define LASER_LOW   5     // % of power for the laser low mode (0.25mW)

// Brightness Settings
const short il_h = round(ILLUM_HIGH / 100.0 * 255);
const short il_m = round(ILLUM_MED  / 100.0 * 255);
const short il_l = round(ILLUM_LOW  / 100.0 * 255);
const short la_h = round(LASER_HIGH / 100.0 * 255);
const short la_m = round(LASER_MED  / 100.0 * 255);
const short la_l = round(LASER_LOW  / 100.0 * 255);

//            Positions:    0,    1,     2,     3,     4,     5,     6,     7,     8,     9
//            Modes:        OFF,  DH,    OFF,   IH,    OFF,   AH,    OFF,   DM,    DL,    AL
const short ir_val[10] =    {0,    la_h,  0,     0,     0,     la_h,  0,     la_m,  la_l,  la_l};
const short illum_val[10] = {0,    il_h,  0,     il_h,  0,     0,     0,     il_m,  il_l,  0};

// Misc Variables
int timeout_count = 0;            // counter for 5-min timeout
time_t current_time = 0;

bool button_pressed = false;
bool dbl_pressed = false;
time_t dbl_down_time = 0;

float batt_v;
time_t fade_time = 0;
int fade_value = 255;
bool fade_up = false;

///////////////////////////////
///        Interrupt        ///
///////////////////////////////

ISR(PCINT0_vect) {
  current_time = millis();
  noInterrupts();
  if (!digitalRead(BUTTON_PIN)) {
    if (!button_pressed) {
      dbl_pressed = (current_time > DBL_PRESS_MAX && 
                      current_time - dbl_down_time < DBL_PRESS_MAX && 
                      current_time - dbl_down_time > DBL_PRESS_MIN);
      dbl_down_time = current_time;

      button_pressed = true;
      digitalWrite(INDICATOR_PIN, HIGH);
      set_outputs();
      batt_v = read_vcc();
    }
  } else {
    button_pressed = false;
    if (!dbl_pressed) {
      analogWrite(IR_PIN, 0);
      analogWrite(ILLUM_PIN, 0);
      digitalWrite(INDICATOR_PIN, LOW);
    }
  }
  timeout_count = 0;
  interrupts();
}

//////////////////////////////
///      Setup & Loop      ///
//////////////////////////////

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SW_1, INPUT_PULLUP);
  pinMode(SW_2, INPUT_PULLUP);
  pinMode(SW_4, INPUT_PULLUP);
  pinMode(SW_8, INPUT_PULLUP);
  pinMode(INDICATOR_PIN, OUTPUT);
  GIMSK = 0b00010000;
  PCMSK0 = 0b00010000;
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Force set_outputs() if button pressed when device started
  if (!digitalRead(BUTTON_PIN)) {
    button_pressed = true;
    digitalWrite(INDICATOR_PIN, HIGH);
    set_outputs();
  }
}

void loop() {
  // if active, count time active
  if (button_pressed || dbl_pressed) {
    if (millis() - current_time >= SHUTOFF_TIMEOUT) {    
      current_time = millis();
      timeout_count++;
    }

    // If low battery, slow blink indicator when activated
    if (batt_v <= LOW_BATTERY_MV) {
      if (millis() - fade_time >= BLINK_SPEED_SLOW) {
        if (fade_up) {
          fade_value += 5;
          fade_up = !(fade_value >= 255);
        } else {
          fade_value -= 5;
          fade_up = fade_value <= 0;
        }
        analogWrite(INDICATOR_PIN, fade_value);
        fade_time = millis();
      }
    }
  }

  // If not activated or TIMEOUT_COUNT_LIMIT reached, shut off
  if ((!button_pressed && !dbl_pressed) || timeout_count >= TIMEOUT_COUNT_LIMIT ) {
    dbl_pressed = false;
    if (millis() - current_time >= SLEEP_TIMEOUT) {
      analogWrite(IR_PIN, 0);
      analogWrite(ILLUM_PIN, 0);
      digitalWrite(INDICATOR_PIN, LOW);
      sleep_mode();
    }
  }
}

///////////////////////////////
///        Functions        ///
///////////////////////////////

void set_outputs() {
  bool sw_1 = digitalRead(SW_1);
  bool sw_2 = digitalRead(SW_2);
  bool sw_4 = digitalRead(SW_4);
  bool sw_8 = digitalRead(SW_8);

  int pos = (0 << 3) | !sw_8 << 3 | !sw_4 << 2 | !sw_2 << 1 | !sw_1;
  analogWrite(IR_PIN, ir_val[pos]);
  analogWrite(ILLUM_PIN, illum_val[pos]);
}

long read_vcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = 0b00100001;// adc source=1.1 ref; adc ref (base for the 1023 maximum)=Vcc
  delay(2); // Wait for Vref to settle
  ADCSRA |= 1<<ADSC; // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}
