#include <SoftwareSerial.h>

SoftwareSerial mySerial(A2, 11); // RX, TX


void loop() { // run over and over
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
}

#include <Wire.h>
#include <Adafruit_MCP4725.h>
#define time_t unsigned long

Adafruit_MCP4725 dac;

// Pin Variables
int ledPin = A0;
int buttonPin = 3;
int illumPin = A3;
int irPin = A1;
int visPin = A2;
int vccPin = 4;
//int dacValue[9] = {4095, 3583, 3071, 2559, 2047, 1535, 1023, 511, 0};
int dacValue[9] = {0, 511, 1023, 1535, 2047, 2559, 3071, 3583, 4095}; // 5V
//int dacValue[9] = {0, 442, 884, 1326, 1768, 2211, 2652, 3095, 3537}; // 4.32V
String names[9] =  {"VH", "VL", "OFF", "AL", "DL", "AH", "IH", "DH", "PRGM"};
int exp_ir[9] =    { 0,     0,   0,     10,   10,   100,  0,    100,  10};
int exp_vis[9] =   { 100,   10,  0,     0,    0,    0,    0,    0,    10};
int exp_illum[9] = { 0,     0,   0,     0,    10,   0,    100,  100,  10};
int ppsVal[9] =    { 0,     0,   0,     0,    1,    2,    4,    8,    0};


// Settings Flags
bool failHard = false;
bool haveFailed = false;
bool showErrors = true;
bool leaveOn = true;

bool testPos_ = false;
bool testAll = true;
int testPosVal = 1;

bool testProgram_ = true;
bool testPrgmTimeout_ = false;
bool testTimeout_ = false;
bool testDoublePress_ = false;


void setup() {
  pinMode(ledPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(illumPin, INPUT);
  pinMode(irPin, INPUT);
  pinMode(visPin, INPUT);
  digitalWrite(vccPin, LOW);
  pinMode(vccPin, OUTPUT);
  dac.begin(0x62);
  mySerial.begin(19200);
  
  Serial.begin(9600);
  Serial.println("\n\n############# GAL v2.1 Testing Platform #############\n");
  Serial.println("failHard | showErrors | leaveOn | testPos_ | testAll");
  Serial.print((String)"    " + failHard + "    |     " + showErrors + "      |    " + leaveOn);
  Serial.println((String)"    |    " + testPos_ + "     |    " + testAll);
  Serial.println("testProgram_ | testPrgmTimeout_ | testTimeout_ | testDoublePress_");
  Serial.print((String)"      " + testProgram_ + "      |        " + testPrgmTimeout_ + "         |       ");
  Serial.println((String)testTimeout_ + "      |        " + testDoublePress_ + "\n");
  delay(100);

  digitalWrite(vccPin, HIGH);

  if (testPos_) {
    Serial.println("########## Testing Positions ##########");
    Serial.println("SET\tPOS\tLED\tILLUM\tIR\tVIS");
    Serial.println("----------------------------------------------");
    if (testAll) {
      for (int i = 0; i < 9; i++) {
        if (haveFailed && failHard){
          Serial.println("\n   failHard has stopped the program");
          break;
        }
        testPos(i);
        Serial.println("----------------------------------------------");
      }
    } else { 
      testPos(testPosVal); 
    }
    Serial.println("########## Test Complete ##########\n");
  }
  
  if (testProgram_) {
    Serial.println("########## Testing Programming ##########");
    testProgram();
    while (mySerial.available()) {
      Serial.write(mySerial.read());
    }
    deactivate(0);
    dac.setVoltage(dacValue[7], false); delay(250);
    activate(50);
    deactivate(50);
    activate(500);
    deactivate(0);
    while (mySerial.available()) {
      Serial.write(mySerial.read());
    }
    Serial.println("########## Test Complete ##########\n");
    if (testPrgmTimeout_) {
      Serial.println("########## Testing Programming Timeout ##########");
      testProgramTimeout();
      Serial.println("########## Test Complete ##########\n");
    }
  }

  if (testTimeout_) {
    Serial.println("########## Testing Timeout ##########");
    testTimeout(0, 1);
    testTimeout(1, 1);
    testTimeout(3, 1);
    testTimeout(4, 1);
    testTimeout(5, 1);
    testTimeout(6, 1);
    testTimeout(7, 1);
    Serial.println("########## Test Complete ##########\n");
  }

  if (testDoublePress_) {
    Serial.println("########## Testing Double Press ##########");
    testDoublePress(6);
    Serial.println("########## Test Complete ##########\n");
  }

  if (!leaveOn) {
    deactivate(100);
    digitalWrite(vccPin, LOW);
  }

  Serial.println("############# GAL v2.1 Testing Complete #############\n");
}

void testDoublePress(int pos) {
  int led = 1;
  int illum = exp_illum[pos], ir = exp_ir[pos], vis = exp_vis[pos];
  int led_t, illum_t, ir_t, vis_t;

  Serial.println("  Activating Double Press");
  deactivate(500);
  dac.setVoltage(dacValue[pos], false); delay(250);
  activate(50);
  deactivate(50);
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  activate(500);
  deactivate(250);
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  delay(2000);
  getActual(led_t, illum_t, ir_t, vis_t);
  Serial.print((String)names[pos] + "\t" + pos + "\t");
  Serial.println((String)led + "\t" + illum  + "\t" + ir  + "\t" + vis);
  Serial.println((String)"   Actual\t" + led_t  + "\t" + illum_t  + "\t" + ir_t  + "\t" + vis_t);
  Serial.println("\n  Deactivating Double Press");
  delay(1000);
  activate(50);
  deactivate(500); 
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  getActual(led_t, illum_t, ir_t, vis_t);
  Serial.print((String)names[pos] + "\t" + pos + "\t");
  Serial.println((String)led + "\t" + illum  + "\t" + ir  + "\t" + vis);
  Serial.println((String)"   Actual\t" + led_t  + "\t" + illum_t  + "\t" + ir_t  + "\t" + vis_t);
}


///// HELPER FUNCTIONS ///// 

void activate(int t) {
  pinMode(buttonPin, OUTPUT);
  digitalWrite(buttonPin, LOW);
  if (t>0) delay(t);
}

void deactivate(int t) { 
  pinMode(buttonPin, INPUT_PULLUP); 
  if (t>0) delay(t);
}

int readLedPin() {
  return round(analogRead(ledPin)/512.0);
}

void testAndValidate(int pos) {
  int led = 1;
  int illum = exp_illum[pos], ir = exp_ir[pos], vis = exp_vis[pos];
  int led_t, illum_t, ir_t, vis_t;
  getActual(led_t, illum_t, ir_t, vis_t);

  int illum_d = abs(illum - illum_t);
  int ir_d = abs(ir - ir_t);
  int vis_d = abs(vis - vis_t);

  Serial.println((String)led + "\t" + illum  + "\t" + ir  + "\t" + vis);
  Serial.println((String)"   Actual\t" + led_t  + "\t" + illum_t  + "\t" + ir_t  + "\t" + vis_t);
  if (showErrors) {
    if (illum_d > 10) {
      Serial.println("### ERROR: ILLUM value not correct ###");
      haveFailed = true;
    }
    if (ir_d > 10) {
      Serial.println("### ERROR: IR value not correct ###");
      haveFailed = true;
    }
    if (vis_d > 10) {
      Serial.println("### ERROR: VIS value not correct ###");
      haveFailed = true;
    }
  }
}

void getActual(int& led_t, int& illum_t, int& ir_t, int& vis_t) {
  led_t = readLedPin();
  illum_t = round(getDutyCycle(illumPin));
  ir_t = round(getDutyCycle(irPin));
  vis_t = round(getDutyCycle(visPin));
}

float getDutyCycle(int pin) {
  unsigned long highTime = pulseIn(pin, HIGH, 10000);
  unsigned long lowTime = pulseIn(pin, LOW, 10000);
  unsigned long cycleTime = highTime + lowTime;
  if (cycleTime == 0) {
    if (analogRead(pin) < 250) {
      return 0;
    } else {
      return 100;
    }
  }
  return((float)highTime / float(cycleTime)) * 100.0;
}

float getFreq(int pin) {
  unsigned long highTime = pulseIn(pin, HIGH, 2000000);
  unsigned long lowTime = pulseIn(pin, LOW, 2000000);
  unsigned long cycleTime = highTime + lowTime;
  if (cycleTime == 0) {
    return 0;
  }
  return 1000000.0/cycleTime;
}

void testProgram() {
  deactivate(100);
  for (int i = 7; i < 8; i++) {
    deactivate(100);
    dac.setVoltage(dacValue[8], false);   // PRGM
    Serial.println("Setting to PRGM"); delay(300);
    activate(1000);
    dac.setVoltage(dacValue[i], false);
    Serial.println((String)"  Setting to " + ppsVal[i] + "pps"); delay(300);
    deactivate(i * 250);
    dac.setVoltage(dacValue[7], false);
    Serial.println("  Setting to DH"); delay(300);
    activate(0); 
    Serial.println("  Getting Frequency"); delay(300);
    float freq = getFreq(illumPin);
    Serial.println((String)"  Freq: " + freq); delay(300);
    if (round(freq) != ppsVal[i]) {
      Serial.println((String)"   ##ERROR: " + freq + " != " + ppsVal[i]);
    }
    if (i == 4) {
      deactivate(250);
      Serial.println("Removing pps");
      dac.setVoltage(dacValue[8], false);   // PRGM
      delay(2500);
      activate(250);
      deactivate(2500);
      dac.setVoltage(dacValue[7], false);   // IH
      Serial.println("  Setting to DH"); delay(250);
      activate(0); 
      Serial.println("  Getting Frequency"); delay(250);
      freq = getFreq(illumPin);
      Serial.println((String)"  Freq: " + freq);
      if (round(freq) != 0) {
        Serial.println((String)"   ##ERROR: " + freq + " != 0");
      }
    }
  }
}

void testProgramTimeout() {
  deactivate(100);
  dac.setVoltage(dacValue[8], false);   // PRGM
  Serial.println("Setting to PRGM"); delay(100);
  time_t t_start = millis();
  activate(25);
  Serial.println("  Testing timeout"); delay(2000); 
  dac.setVoltage(dacValue[5], false); 
  Serial.println("  Setting to AH - 2pps");
  bool fail = false;  
  while (true) {
    if (!readLedPin()) {
      break;
    }
    if (millis() - t_start >= 7000) {
      fail = true;
      break;
    }
  }
  time_t t_end = millis();
  if (fail) {
    Serial.println("  #ERROR: Program did not time out");
  } else {
    Serial.println((String)"  Programming sucessfully timed out in " + ((t_end - t_start) / 1000.0) + " seconds");
  }
}

void testTimeout(int pos, int run_num) {
  deactivate(500);
  dac.setVoltage(dacValue[pos], false);
  Serial.println((String)"Testing pos " + pos + " (" + names[pos] + ") " + run_num + " time(s)"); delay(500);
  for (int i = 0; i < run_num; i++) { 
    Serial.println("Activating and waiting");
    time_t start_t = millis();
    activate(25);
    int val = readLedPin();
    while(val > 0) {
      delay(500);
      val = readLedPin();
    }
    time_t end_t = millis();
    Serial.println((String)"Start Time: " + start_t + " | End Time:" + end_t + " | Total Time: " + (end_t - start_t));
    deactivate(500);
    if (i != run_num - 1) {
      Serial.println("Testing again");
    }
  }
}


void testPos(int pos) {
  deactivate(250);
  dac.setVoltage(dacValue[pos], false);
  Serial.print((String)names[pos] + "\t" + pos + "\t");

    // Activate PCB
  activate(250);
  testAndValidate(pos); delay(500);
  if (!leaveOn) {
    deactivate(0);
  }
}
