// https://github.com/kachurovskiy/nanoels

/* Change values in this section to suit your hardware. */

// Define your hardware parameters here. Don't remove the ".0" at the end.
#define ENCODER_STEPS 600.0 // 600 step spindle optical rotary encoder
#define MOTOR_STEPS 400.0
#define LEAD_SCREW_TMM 2000.0 // 2mm lead screw   thousandths of mm

// Spindle rotary encoder pins. Nano only supports interrupts on D2 and D3.
// Swap values if the rotation direction is wrong.
#define ENC_A 3 // D3
#define ENC_B 2 // D2

// Stepper pulse and acceleration constants.
#define PULSE_MIN_US round(500.0 * 200.0 / MOTOR_STEPS) // Microseconds to wait after high pulse, min.
#define PULSE_MAX_US round(2000.0 * 200.0 / MOTOR_STEPS) // Microseconds to wait after high pulse, max. Slow start.
// Microseconds remove from waiting time on every step. Acceleration. Use higher value (e.g. 7) if you have lower MOTOR_STEPS.
#define PULSE_DELTA_US round(max(1, 1600.0 / MOTOR_STEPS))
#define INVERT_STEPPER false // change (true/false) if the carriage moves e.g. "left" when you press "right".


// Voltage on A6 when F1-F5 buttons are pressed.
// If those buttons don't work as expected, uncomment "// Serial.println(value);" in getAnalogButton(),
// upload sketch to your Arduino and click each button (topmost is F1) to find the right value for each.
// If values are too close to each other (should be ~170 apart), improve resistor soldering connections.
#define F1_VOLTAGE 850
#define F2_VOLTAGE 679
#define F3_VOLTAGE 509
#define F4_VOLTAGE 339
#define F5_VOLTAGE 169

/* Changing anything below shouldn't be needed for basic use. */

// Analog voltage epsilon allowing for slightly imprecise resistances.
// Should be around (F1_VOLTAGE - F2_VOLTAGE) / 3
#define BUTTON_EPSILON 50

#define LONG_MIN long(-2147483648)
#define LONG_MAX long(2147483647)

#define LOOP_COUNTER_MAX 1500 // 1500 loops without stepper move to start reading buttons
#define TMMPR_MAX 10000 // 10mm

// Ratios between spindle and stepper.
#define ENCODER_TO_STEPPER_STEP_RATIO MOTOR_STEPS / (LEAD_SCREW_TMM * ENCODER_STEPS)   
#define STEPPER_TO_ENCODER_STEP_RATIO LEAD_SCREW_TMM * ENCODER_STEPS / MOTOR_STEPS		

// If time between encoder ticks is less than this, direction change is not allowed.
// Effectively this limits direction change to the time when spindle is <20rpm.
#define DIR_CHANGE_DIFF_MICROS (int) (5000 * ENCODER_STEPS / 600)

// Version of the EEPROM storage format, should be changed when non-backward-compatible
// changes are made to the storage logic, resulting in EEPROM wipe on first start.
#define EEPROM_VERSION 2

// To be incremented whenever a measurable improvement is made.
#define SOFTWARE_VERSION 2

// To be changed whenever a different PCB / encoder / stepper / ... design is used.
#define HARDWARE_VERSION 2

#define DIR 11
#define STEP 12
#define ENA 13

#define B0 4
#define B1 5
#define B2 6
#define B3 7
#define B4 8
#define B5 9
#define B6 10

#define B_LEFT  4
#define B_RIGHT 5
#define B_MINUS 6
#define B_PLUS  7
#define B_ONOFF 8
#define B_STOPL 9
#define B_STOPR 10
#define B_F1 0b1010111
#define B_F2 0b1011011
#define B_F3 0b1011101
#define B_F4 0b1011110
#define B_F5 0b1100111

#define ADDR_EEPROM_VERSION 0 // takes 1 byte
#define ADDR_ONOFF 1 // takes 1 byte
#define ADDR_TMMPR 2 // takes 2 bytes
#define ADDR_POS 4 // takes 4 bytes
#define ADDR_LEFT_STOP 8 // takes 4 bytes
#define ADDR_RIGHT_STOP 12 // takes 4 bytes
#define ADDR_SPINDLE_POS 16 // takes 4 bytes
#define ADDR_OUT_OF_SYNC 20 // takes 2 bytes
#define ADDR_SHOW_ANGLE 22 // takes 1 byte
#define ADDR_SHOW_TACHO 23 // takes 1 byte
#define ADDR_MOVE_STEP 24 // takes 2 bytes
#define ADDR_UNIT_MODE 26 // takes 1 bytes

#define MODECOUNT 3
#define UNIT_MM 0
#define UNIT_IN 1
#define UNIT_TPI 2

#define RPM_UPDATE_THRESHOLD 2
#define RPM_BULK ENCODER_STEPS

// Uncomment to print out debug info in Serial.
#define DEBUG

// Uncomment to run the self-test of the code instead of the actual program.
//#define TEST
#ifdef TEST
bool mockDigitalPins[20] = {0};
int mockDigitalPinToggleOnRead = -1;
int mockDigitalRead(int x) {
  int value = mockDigitalPins[x];
  if (x == mockDigitalPinToggleOnRead) {
    mockDigitalPins[x] ^= 1;
  }
  return value;
}
#define DREAD(x) mockDigitalRead(x)
#else
#include <FastGPIO.h>
#define DREAD(x) FastGPIO::Pin<x>::isInputHigh()
#endif

#ifndef TEST
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);
long lcdHash = 0;
#endif

#include <EEPROM.h>

unsigned long buttonTime = 0;
long loopCounter = 0;
bool isOn = false;
unsigned long resetMillis = 0;
bool resetOnStartup = false;

int tmmpr = 0; // thousandth of a mm per rotation
int savedTmmpr = 0; // tmmpr saved in EEPROM
int tmmprPrevious = 0;

long pos = 0; // relative position of the stepper motor, in steps
long savedPos = 0; // value saved in EEPROM

long leftStop = 0; // left stop value of pos
long savedLeftStop = 0; // value saved in EEPROM
bool leftStopFlag = true; // prevent toggling the stop while button is pressed.

long rightStop = 0; // right stop value of pos
long savedRightStop = 0; // value saved in EEPROM
bool rightStopFlag = true; // prevent toggling the stop while button is pressed.

volatile unsigned long spindleEncTime = 0; // micros() of the previous spindle update
volatile unsigned long spindleEncTimeDiff = 0; // micros() since the previous spindle update
volatile unsigned long spindleEncTimeDiffBulk = 0; // micros() between RPM_BULK spindle updates
volatile unsigned long spindleEncTimeAtIndex0 = 0; // micros() when spindleEncTimeIndex was 0
volatile int spindleEncTimeIndex = 0; // counter going between 0 and RPM_BULK - 1
volatile int spindleDeltaPrev = 0; // Previously detected spindle direction
volatile long spindlePos = 0; // Spindle position
long savedSpindlePos = 0; // spindlePos value saved in EEPROM
long spindleLeftStop = 0;
long spindleRightStop = 0;

volatile int spindlePosSync = 0;
int savedSpindlePosSync = 0;

int stepDelayUs = PULSE_MAX_US;
bool stepDelayDirection = true; // To reset stepDelayUs when direction changes.
bool stepDirectionInitialized = false;
unsigned long stepStartMs = 0;
int stepperEnableCounter = 0;

bool showAngle = false; // Whether to show 0-359 spindle angle on screen
bool showTacho = false; // Whether to show spindle RPM on screen
bool savedShowAngle = false; // showAngle value saved in EEPROM
bool savedShowTacho = false; // showTacho value saved in EEPROM
int shownRpm = 0;

int moveStep = 0; // hundredth of a mm
int savedMoveStep = 0; // moveStep saved in EEPROM

byte unitMode = UNIT_MM;
byte savedUnitMode = UNIT_MM; // unitMode
byte pitchTableIdx=0;
byte moveStepTableIdx=0;

//list of commonly used pitches  if list is less than the array size, end with a 0
const int pitchTable[MODECOUNT][7] = {{100,1000,2000,0}, {25,254,1270,2540,0}, {3175, 2540,2116,1814,1587,1411, 1270}}; //0.1mm, 1mm, 2mm  / 0.005, 0.01", .05, .1" / 8,10,12,14,16,18,20 tpi
const int moveStepTable[MODECOUNT][3] ={{10,100,1000}, {25,254,1270}, {25,254,1270}};  //0.01, 0.1, 1.0mm / ~ 0.001, 0.01, 0.05in
const char *showUnit[MODECOUNT] = {"mm", "\"" , "\""};
const char *pitchUnit[MODECOUNT] = {"mm", "\"" , "tpi"};
const int unitDiv[MODECOUNT] = {1000,25400,25400};
const int decimals[MODECOUNT] = {2,3,3};


int getApproxRpm() {
  int rpm = 0;
  int diff = spindleEncTimeDiffBulk / RPM_BULK;
  if (diff > 0 && showTacho && (micros() - spindleEncTime < 100000)) {
    rpm = round(60000000.0 / (diff * ENCODER_STEPS));
    if (abs(rpm - shownRpm) < RPM_UPDATE_THRESHOLD) {
      rpm = shownRpm;
    }
  }
  return rpm;
}

void updateDisplay() {
#ifndef TEST
  // Avoid updating the LCD when nothing has changed, it flickers.
  int rpm = getApproxRpm();
  long newLcdHash = (showAngle ? spindlePos : 0) + pos * 2 + tmmpr * 3 + isOn * 4 + leftStop / 5
                    + rightStop / 6 + spindlePosSync * 7 + resetOnStartup * 8 + showAngle * 9
                    + showTacho * 10 + moveStep * 11 + rpm * 12 + unitMode * 13;
  if (newLcdHash == lcdHash) {
    return;
  }

  lcdHash = newLcdHash;
  lcd.clear();

  // First row.
  lcd.setCursor(0, 0);
  lcd.print(isOn ? "ON " : "OFF");
 
  //if (moveStep != MOVE_STEP_1) {
    lcd.print(" Step: ");
    lcd.print(moveStep * 1.0 / unitDiv[unitMode], decimals[unitMode]);
    lcd.print(showUnit[unitMode]);
  //}
  if (spindlePosSync) {
    lcd.print(" SYN");
  }
  if (resetOnStartup) {
    lcd.print(" LTW");
  }
  
   if (leftStop != LONG_MAX && rightStop != LONG_MIN) {
    lcd.print(" LR");
  } else if (leftStop != LONG_MAX) {
    lcd.print(" L");
  } else if (rightStop != LONG_MIN) {
    lcd.print("  R");
  }

  // Second row.
  lcd.setCursor(0, 1);
  lcd.print("Pitch:    ");
  if(unitMode == UNIT_TPI){ //tpi
    lcd.print(25400 / (tmmpr * 1.0), 1);
  }
  else {//in, mm, tmppr
    lcd.print(tmmpr * 1.0 / unitDiv[unitMode], decimals[unitMode]);
  }
  
  lcd.print(pitchUnit[unitMode]);

  // Third row.
  lcd.setCursor(0, 2);
  lcd.print("Position: ");
  lcd.print(pos * LEAD_SCREW_TMM / MOTOR_STEPS / unitDiv[unitMode], decimals[unitMode]);
  lcd.print(showUnit[unitMode]);

  // Fourth row.
  lcd.setCursor(0, 3);
  if (showAngle) {
    lcd.print("Angle:    ");
    lcd.print(round(((spindlePos % (int) ENCODER_STEPS + (int) ENCODER_STEPS) % (int) ENCODER_STEPS) * 360 / ENCODER_STEPS));
    lcd.print((char) 223);
  } else if (showTacho) {
    lcd.print("Tacho:    ");
    lcd.print(rpm);
    shownRpm = rpm;
    lcd.print("rpm");
  }
  else {
    lcd.print("-- NanoELS H" + String(HARDWARE_VERSION) + " V" + String(SOFTWARE_VERSION) + " --");
  }
#endif
}

void saveInt(int i, int v) {
  // Can't concatenate all in one line due to compiler problems, same throughout the code.
#ifdef DEBUG
  Serial.print("Saving int at ");
  Serial.print(i);
  Serial.print(" = ");
  Serial.println(v);
#endif
  EEPROM.write(i, v >> 8 & 0xFF);
  EEPROM.write(i + 1, v & 0xFF);
}
int loadInt(int i) {
  // 255 is the default value when nothing was written before.
  if (EEPROM.read(i) == 255 && EEPROM.read(i + 1) == 255) {
    return 0;
  }
  return (EEPROM.read(i) << 8) + EEPROM.read(i + 1);
}
void saveLong(int i, long v) {
#ifdef DEBUG
  Serial.print("Saving long at ");
  Serial.print(i);
  Serial.print(" = ");
  Serial.println(v);
#endif
  EEPROM.write(i, v >> 24 & 0xFF);
  EEPROM.write(i + 1, v >> 16 & 0xFF);
  EEPROM.write(i + 2, v >> 8 & 0xFF);
  EEPROM.write(i + 3, v & 0xFF);
}
long loadLong(int i) {
  long p0 = EEPROM.read(i);
  long p1 = EEPROM.read(i + 1);
  long p2 = EEPROM.read(i + 2);
  long p3 = EEPROM.read(i + 3);
  // 255 is the default value when nothing was written before.
  if (p0 == 255 && p1 == 255 && p2 == 255 && p3 == 255) {
    return 0;
  }
  return (p0 << 24) + (p1 << 16) + (p2 << 8) + p3;
}

// Called on a FALLING interrupt for the spindle rotary encoder pin.
// Keeps track of the spindle position.
void spinEnc() {
  unsigned long microsNow = micros();
  if (spindleEncTimeIndex == 0) {
    spindleEncTimeDiffBulk = microsNow - spindleEncTimeAtIndex0;
    spindleEncTimeAtIndex0 = microsNow;
  }
  spindleEncTimeIndex = (spindleEncTimeIndex + 1) % int(RPM_BULK);

  int delta;
  spindleEncTimeDiff = microsNow - spindleEncTime;
  if (spindleEncTimeDiff > DIR_CHANGE_DIFF_MICROS) {
    delta = DREAD(ENC_B) ? -1 : 1;
    spindleDeltaPrev = delta;
  } else {
    // Spindle is going fast, unlikely to change direction momentarily.
    delta = spindleDeltaPrev;
  }
  spindlePos += delta;
  spindleEncTime += spindleEncTimeDiff;

  if (spindlePosSync != 0) {
    spindlePosSync += delta;
    if (spindlePosSync == 0 || spindlePosSync == ENCODER_STEPS) {
      spindlePosSync = 0;
      spindlePos = spindleFromPos(pos);
    }
  }
}

#ifdef TEST
void setup() {
  Serial.begin(9600);
}
#else
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(B0, INPUT_PULLUP);
  pinMode(B1, INPUT_PULLUP);
  pinMode(B2, INPUT_PULLUP);
  pinMode(B3, INPUT_PULLUP);
  pinMode(B4, INPUT_PULLUP);
  pinMode(B5, INPUT_PULLUP);
  pinMode(B6, INPUT_PULLUP);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);
  digitalWrite(STEP, HIGH);

  // Wipe EEPROM if this is the first start after uploading a new build.
  if (EEPROM.read(ADDR_EEPROM_VERSION) != EEPROM_VERSION) {
    for (int i = 0; i < 256; i++) {
      EEPROM.write(i, 255); // 255 is the default value.
    }
    EEPROM.write(ADDR_EEPROM_VERSION, EEPROM_VERSION);
    saveLong(ADDR_LEFT_STOP, savedLeftStop = leftStop = LONG_MAX);
    saveLong(ADDR_RIGHT_STOP, savedRightStop = rightStop = LONG_MIN);
    saveInt(ADDR_MOVE_STEP, moveStepTable[UNIT_MM][0]);
    EEPROM.write(ADDR_UNIT_MODE, UNIT_MM);
  }

  isOn = EEPROM.read(ADDR_ONOFF) == 1;
  savedTmmpr = tmmpr = loadInt(ADDR_TMMPR);
  savedPos = pos = loadLong(ADDR_POS);
  savedLeftStop = leftStop = loadLong(ADDR_LEFT_STOP);
  savedRightStop = rightStop = loadLong(ADDR_RIGHT_STOP);
  savedSpindlePos = spindlePos = loadLong(ADDR_SPINDLE_POS);
  savedSpindlePosSync = spindlePosSync = loadInt(ADDR_OUT_OF_SYNC);
  savedShowAngle = showAngle = EEPROM.read(ADDR_SHOW_ANGLE) == 1;
  savedShowTacho = showTacho = EEPROM.read(ADDR_SHOW_TACHO) == 1;
  savedMoveStep = moveStep = loadInt(ADDR_MOVE_STEP);
  savedUnitMode = unitMode = EEPROM.read(ADDR_UNIT_MODE);

  stepperEnable(isOn);

  attachInterrupt(digitalPinToInterrupt(ENC_A), spinEnc, FALLING);

  lcd.begin(20, 4);
  updateDisplay();

  Serial.begin(9600);
  Serial.print("NanoEls H");
  Serial.print(HARDWARE_VERSION);
  Serial.print(" V");
  Serial.println(SOFTWARE_VERSION);

  preventMoveOnStart();
}
#endif

void preventMoveOnStart() {
  // Sometimes, especially if ELS was run outside above max RPM before, pos and spindlePos
  // will be out of sync causing immediate stepper movement if isOn. This could be dangerous
  // and surely won't be expected by the operator.
  long newPos = posFromSpindle(spindlePos, true);
  if (isOn && newPos != pos) {
#ifdef DEBUG
    Serial.println("Losing the thread");
#endif
    resetOnStartup = true;
    markAsZero();
  }
}

// Saves all positions in EEPROM, should be called infrequently to reduce EEPROM wear.
void saveIfChanged() {
  if (tmmpr != savedTmmpr) {
    saveInt(ADDR_TMMPR, savedTmmpr = tmmpr);
  }
  if (pos != savedPos) {
    saveLong(ADDR_POS, savedPos = pos);
  }
  if (leftStop != savedLeftStop) {
    saveLong(ADDR_LEFT_STOP, savedLeftStop = leftStop);
  }
  if (rightStop != savedRightStop) {
    saveLong(ADDR_RIGHT_STOP, savedRightStop = rightStop);
  }
  if (spindlePos != savedSpindlePos) {
    saveLong(ADDR_SPINDLE_POS, savedSpindlePos = spindlePos);
  }
  if (spindlePosSync != savedSpindlePosSync) {
    saveInt(ADDR_OUT_OF_SYNC, savedSpindlePosSync = spindlePosSync);
  }
  if (showAngle != savedShowAngle) {
    EEPROM.write(ADDR_SHOW_ANGLE, savedShowAngle = showAngle);
  }
  if (showTacho != savedShowTacho) {
    EEPROM.write(ADDR_SHOW_TACHO, savedShowTacho = showTacho);
  }
  if (moveStep != savedMoveStep) {
    saveInt(ADDR_MOVE_STEP, savedMoveStep = moveStep);
  }
  if (unitMode != savedUnitMode) {
    EEPROM.write(ADDR_UNIT_MODE, savedUnitMode = unitMode);
  }  
}

// Checks if some button was recently pressed. Returns true if not.
bool checkAndMarkButtonTime() {
  if (millis() > buttonTime + 300) {
    buttonTime = millis();
    return true;
  }
  return false;
}

// Loose the thread and mark current physical positions of
// encoder and stepper as a new 0. To be called when tmmpr changes
// or ELS is turned on/off. Without this, changing tmmpr will
// result in stepper rushing across the lathe to the new position.
void markAsZero() {
  noInterrupts();
  if (leftStop != LONG_MAX) {
    leftStop -= pos;
  }
  if (rightStop != LONG_MIN) {
    rightStop -= pos;
  }
  pos = 0;
  spindlePos = 0;
  spindlePosSync = 0;
  interrupts();
}

void setTmmpr(int value) {
  tmmpr = value;
  markAsZero();
  updateDisplay();
}


//no longer used as version info moved to main screen when mode rpm and angle are both off
void splashScreen() {
#ifndef TEST
  lcd.clear();
  lcd.setCursor(6, 1);
  lcd.print("NanoEls");
  lcd.setCursor(6, 2);
  lcd.print("H" + String(HARDWARE_VERSION) + " V" + String(SOFTWARE_VERSION));
  lcdHash = 0;
  delay(2000);
#endif
}

void reset() {
  resetOnStartup = false;
  leftStop = LONG_MAX;
  rightStop = LONG_MIN;
  setTmmpr(0);
  moveStep = moveStepTable[UNIT_MM][0];
  //splashScreen();
}

// Called when left/right stop restriction is removed while we're on it.
// Prevents stepper from rushing to a position far away by waiting for the right
// spindle position and starting smoothly.
void setOutOfSync() {
  if (!isOn) {
    return;
  }
  spindlePosSync = ((spindlePos - spindleFromPos(pos)) % (int) ENCODER_STEPS + (int) ENCODER_STEPS) % (int) ENCODER_STEPS;
#ifdef DEBUG
  Serial.print("spindlePosSync ");
  Serial.println(spindlePosSync);
#endif
}

// Check if the - or + buttons are pressed.
void checkPlusMinusButtons() {

  bool minus = DREAD(B_MINUS) == LOW;
  bool plus = DREAD(B_PLUS) == LOW;

  // Speed up scrolling when needed.
  int delta=0;
  int tpi = 0;
  if (unitMode == UNIT_MM) delta = abs(tmmprPrevious - tmmpr) >= 1000 ? 1000 : 10; //mm
  else if (unitMode == UNIT_IN) delta = abs(tmmprPrevious - tmmpr) >= 1270 ? 1270 : 25; //in
  else if (unitMode == UNIT_TPI){// tpi
    tpi = (25400/tmmpr);
    if (minus && checkAndMarkButtonTime()){
      tpi--;
      if (tpi >= 0 && tpi <= 3) tpi=-3;
    }
    if (plus && checkAndMarkButtonTime()){
      tpi++;
      if (tpi <=0 && tpi >= -3) tpi=3;
    }
    setTmmpr(25400/tpi);
  } 

  if (minus && checkAndMarkButtonTime()) {
    if (tmmpr > -TMMPR_MAX) {
      setTmmpr(max(-TMMPR_MAX, tmmpr - delta));
    }
  } else if (plus && checkAndMarkButtonTime()) {
    if (tmmpr < TMMPR_MAX) {
      setTmmpr(min(TMMPR_MAX, tmmpr + delta));
    }
  } else if (!minus && !plus) {
    tmmprPrevious = tmmpr;
  }
}

// Check if the ON/OFF button is pressed.
void checkOnOffButton() {
  if (DREAD(B_ONOFF) == LOW) {
    if (resetMillis == 0) {
      resetMillis = millis();
      isOn = !isOn;
      stepperEnable(isOn);
      EEPROM.write(ADDR_ONOFF, isOn ? 1 : 0);
#ifdef DEBUG
      Serial.print("isOn ");
      Serial.println(isOn);
#endif
      markAsZero();
      updateDisplay();
    } else if (resetMillis > 0 && millis() - resetMillis > 6000) {
      resetMillis = 0;
      reset();
    }
  } else {
    resetMillis = 0;
  }
}

// Check if the left stop button is pressed.
void checkLeftStopButton() {
  if (DREAD(B_STOPL) == LOW) {
    if (leftStopFlag) {
      leftStopFlag = false;
      if (leftStop == LONG_MAX) {
        leftStop = pos;
      } else {
        if (pos == leftStop) {
          // Spindle is most likely out of sync with the stepper because
          // it was spinning while the lead screw was on the stop.
          setOutOfSync();
        }
        leftStop = LONG_MAX;
      }
    }
  } else {
    leftStopFlag = true;
  }
}

// Check if the right stop button is pressed.
void checkRightStopButton() {
  if (DREAD(B_STOPR) == LOW) {
    if (rightStopFlag) {
      rightStopFlag = false;
      if (rightStop == LONG_MIN) {
        rightStop = pos;
      } else {
        if (pos == rightStop) {
          // Spindle is most likely out of sync with the stepper because
          // it was spinning while the lead screw was on the stop.
          setOutOfSync();
        }
        rightStop = LONG_MIN;
      }
    }
  } else {
    rightStopFlag = true;
  }
}

void checkMoveButtons() {
  bool left = DREAD(B_LEFT) == LOW;
  bool right = DREAD(B_RIGHT) == LOW;
  if (!left && !right) {
    return;
  }
  if (spindlePosSync) {
    // Edge case.
    return;
  }
  int sign = left ? 1 : -1;
  stepperEnable(true);
  if (isOn && tmmpr != 0) {
    int posDiff = 0;
    do {
      noInterrupts();
      // Move by moveStep in the desired direction but stay in the thread by possibly traveling a little more.
      spindlePos += ceil(MOTOR_STEPS * moveStep * 1.0 / LEAD_SCREW_TMM * STEPPER_TO_ENCODER_STEP_RATIO / ENCODER_STEPS / abs(tmmpr))
                    * ENCODER_STEPS
                    * sign
                    * (tmmpr > 0 ? 1 : -1);
      long newPos = posFromSpindle(spindlePos, true);
      interrupts();
      posDiff = abs(newPos - pos);
      step(newPos > pos, posDiff);
    } while (posDiff != 0 && (left ? DREAD(B_LEFT) : DREAD(B_RIGHT)) == LOW);
  } else {
    int delta = 0;
    do {
      delta = moveStep * 1.0 / LEAD_SCREW_TMM * MOTOR_STEPS;

      // Don't left-right move out of stops.
      if (leftStop != LONG_MAX && pos + delta * sign > leftStop) {
        delta = leftStop - pos;
      } else if (rightStop != LONG_MIN && pos + delta * sign < rightStop) {
        delta = rightStop - pos;
      }

      step(left, abs(delta));

      if (moveStep != moveStepTable[unitMode][0]) {
        // Allow some time for the button to be released to
        // make it possible to do single steps at smallest measure.
        updateDisplay();
        delay(500);
      }
    } while (delta != 0 && (left ? DREAD(B_LEFT) : DREAD(B_RIGHT)) == LOW);
    if (isOn) {
      // Prevent stepper from jumping back to position calculated from the spindle.
      markAsZero();
    }
  }
  stepperEnable(false);
}

void checkDisplayButton(int button) {
  if (button == B_F1 && checkAndMarkButtonTime()) {
    if (!showAngle && !showTacho) {
      showAngle = true;
    } else if (showAngle) {
      showAngle = false;
      showTacho = true;
    } else {
      showTacho = false;
    }
  }
}



void checkMoveStepButton(int button) {
  if (button == B_F2 && checkAndMarkButtonTime()) {
	moveStepTableIdx = (moveStepTableIdx + 1) % 3;
	moveStep = moveStepTable[unitMode][moveStepTableIdx];
  }
}

// Checks if one of the pitch change button was pressed.
void checkPitchButton(int button) {
  if (button == B_F3 && checkAndMarkButtonTime()) {
    int pitchCount = sizeof(pitchTable[unitMode]) / sizeof(int);
    pitchTableIdx = (pitchTableIdx + 1) % pitchCount; // increment and prevent overflow
    if(pitchTable[unitMode][pitchTableIdx] == 0) pitchTableIdx = 0;  //if past the end of the table values, goto 0
    setTmmpr(pitchTable[unitMode][pitchTableIdx]);
  }
}

void checkModeButton(int button) {
  if (button == B_F5 && checkAndMarkButtonTime()) {
	unitMode = (unitMode + 1) % MODECOUNT;
	//moveStepTableIdx = 0; //set the move step size to the smallest value
	moveStep = moveStepTable[unitMode][moveStepTableIdx];
  }
}

// Checks if one of the pitch shortcut buttons were pressed.
//void checkPitchShortcutButton(int button, int bConst, int pitch) {
//  if (button == bConst && checkAndMarkButtonTime()) {
//    setTmmpr(pitch);
//  }
//}

int getAnalogButton() {
  int value = analogRead(A6);
   //Serial.println(value);
  if ((F1_VOLTAGE + BUTTON_EPSILON > value) && (F1_VOLTAGE - BUTTON_EPSILON < value)) {
    return B_F1;
  } else if ((F2_VOLTAGE + BUTTON_EPSILON > value) && (F2_VOLTAGE - BUTTON_EPSILON < value)) {
    return B_F2;
  } else if ((F3_VOLTAGE + BUTTON_EPSILON > value) && (F3_VOLTAGE - BUTTON_EPSILON < value)) {
    return B_F3;
  } else if ((F4_VOLTAGE + BUTTON_EPSILON > value) && (F4_VOLTAGE - BUTTON_EPSILON < value)) {
    return B_F4;
  } else if ((F5_VOLTAGE + BUTTON_EPSILON > value) && (F5_VOLTAGE - BUTTON_EPSILON < value)) {
    return B_F5;
  }
  return 0;
}

// Called when loop() is not busy running the stepper.
// Should take as little time as possible since it's possible that
// lead screw is ON and stepper has to run in a few milliseconds.
void secondaryWork() {
  int button = getAnalogButton();
  checkDisplayButton(button);
  checkMoveStepButton(button);
  checkPitchButton(button);
  //checkPitchShortcutButton(button, B_F4, F4_PITCH);
  checkModeButton(button);

  if (loopCounter % 8 == 0) {
    // This takes a really long time.
    updateDisplay();
  }
  if (loopCounter % 137 == 0) {
    saveIfChanged();
  }
}

// Moves the stepper.
long step(bool dir, long steps) {
  // Start slow if direction changed.
  if (stepDelayDirection != dir || !stepDirectionInitialized) {
    stepDelayUs = PULSE_MAX_US;
    stepDelayDirection = dir;
    stepDirectionInitialized = true;
    digitalWrite(DIR, dir ^ INVERT_STEPPER ? HIGH : LOW);
#ifdef DEBUG
    Serial.print("Direction change");
#endif
  }

  // Stepper basically has no speed if it was standing for 10ms.
  if (millis() - stepStartMs > 10) {
    stepDelayUs = PULSE_MAX_US;
  }

  for (int i = 0; i < steps; i++) {
    unsigned long t = millis();
    int tDiffMs = stepStartMs == 0 ? 0 : t - stepStartMs;
    // long to int can overflow
    if (tDiffMs < 0 || tDiffMs > PULSE_MAX_US) {
      stepDelayUs = PULSE_MAX_US;
    } else {
      stepDelayUs = min(PULSE_MAX_US, max(PULSE_MIN_US, stepDelayUs - PULSE_DELTA_US));
    }
    stepStartMs = t;

    digitalWrite(STEP, LOW);
    // digitalWrite() is slow enough that we don't need to wait.
    digitalWrite(STEP, HIGH);
    // Don't wait during the last step, it will pass by itself before we get back to stepping again.
    // This condition is the reason moving left-right is limited to 600rpm but with ELS On and spindle
    // gradually speeding up, stepper can go to ~1200rpm.
    if (i < steps - 1) {
      delayMicroseconds(stepDelayUs);
    }
  }
  pos += (dir ? 1 : -1) * steps;
}

// Calculates stepper position from spindle position.
long posFromSpindle(long s, bool respectStops) {
  long newPos = s * ENCODER_TO_STEPPER_STEP_RATIO * tmmpr;

  // Respect left/right stops.
  if (respectStops) {
    if (rightStop != LONG_MIN && newPos < rightStop) {
      newPos = rightStop;
    } else if (leftStop != LONG_MAX && newPos > leftStop) {
      newPos = leftStop;
    }
  }

  return newPos;
}

// Calculates spindle position from stepper position.
long spindleFromPos(long p) {
  return p * STEPPER_TO_ENCODER_STEP_RATIO / tmmpr;
}

void stepperEnable(bool value) {
  if (value) {
    stepperEnableCounter++;
    if (value == 1) {
      digitalWrite(ENA, HIGH);
      // Stepper driver needs some time before it will react to pulses.
      delay(100);
    }
  } else if (stepperEnableCounter > 0) {
    stepperEnableCounter--;
    if (stepperEnableCounter == 0) {
      digitalWrite(ENA, LOW);
    }
  }
}

// What is called in the loop() function in when not in test mode.
void nonTestLoop() {
  checkOnOffButton();
  checkPlusMinusButtons();
  checkLeftStopButton();
  checkRightStopButton();
  checkMoveButtons();

  noInterrupts();
  long spindlePosCopy = spindlePos;
  long spindlePosSyncCopy = spindlePosSync;
  interrupts();

  // Move the stepper if needed.
  long newPos = posFromSpindle(spindlePosCopy, true);
  if (isOn && !spindlePosSyncCopy && newPos != pos) {
    // Move the stepper to the right position.
    step(newPos > pos, abs(newPos - pos));
    if (loopCounter > 0) {
      loopCounter = 0;
    }

    // No long calls on this path or stepper will move unevenly.
    return;
  }

  // Perform auxiliary logic but don't take more than a few milliseconds since
  // stepper just be moving slowly and will need signalling soon.

  // When standing at the stop, ignore full spindle turns.
  // This allows to avoid waiting when spindle direction reverses
  // and reduces the chance of the skipped stepper steps since
  // after a reverse the spindle starts slow.
  if (tmmpr != 0) {
    noInterrupts();
    if (rightStop != LONG_MIN && pos == rightStop) {
      long stopSpindlePos = spindleFromPos(rightStop);
      if (tmmpr > 0) {
        if (spindlePos < stopSpindlePos - ENCODER_STEPS) {
          spindlePos += ENCODER_STEPS;
        }
      } else {
        if (spindlePos > stopSpindlePos + ENCODER_STEPS) {
          spindlePos -= ENCODER_STEPS;
        }
      }
    } else if (leftStop != LONG_MAX && pos == leftStop) {
      long stopSpindlePos = spindleFromPos(leftStop);
      if (tmmpr > 0) {
        if (spindlePos > stopSpindlePos + ENCODER_STEPS) {
          spindlePos -= ENCODER_STEPS;
        }
      } else {
        if (spindlePos < stopSpindlePos - ENCODER_STEPS) {
          spindlePos += ENCODER_STEPS;
        }
      }
    }
    interrupts();
  }

  loopCounter++;
  if (loopCounter > LOOP_COUNTER_MAX) {
#ifdef DEBUG
    if (loopCounter % LOOP_COUNTER_MAX == 0) {
      Serial.print("pos ");
      Serial.print(pos);
      Serial.print(" tmmpr ");
      Serial.print(tmmpr);
      Serial.print(" leftStop ");
      Serial.print(leftStop == LONG_MAX ? "-" : String(leftStop));
      Serial.print(" rightStop ");
      Serial.print(rightStop == LONG_MIN ? "-" : String(rightStop));
      Serial.print(" spindlePos ");
      Serial.println(spindlePos);
      Serial.print(" moveStep ");
      Serial.println(moveStep);
    }
#endif

    // Only check buttons when stepper is surely not running.
    // It might have to run any millisecond though e.g. when leaving the stop
    // so it still should complete within milliseconds.
    secondaryWork();

    // Drop the lost thread warning after some time.
    if (resetOnStartup && loopCounter > 2 * LOOP_COUNTER_MAX) {
      resetOnStartup = false;
    }
  }
}

// In unit testing mode, include test library.
#ifdef TEST
#include <AUnit.h>
#endif

void loop() {
  // In unit testing mode, only run tests.
#ifdef TEST
  setupEach();
  aunit::TestRunner::run();
#else
  nonTestLoop();
#endif
}
