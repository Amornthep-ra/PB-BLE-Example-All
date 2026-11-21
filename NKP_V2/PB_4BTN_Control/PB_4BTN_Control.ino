// PB_4BTN_Control.ino
// NKP_V2 (ESP32) + PB Controller app, 4BTN mode
//
// Buttons (CMD bits): UP=1, DOWN=2, LEFT=4, RIGHT=8
// Speed buttons in the app: Lo / Med / Hi  -> picks a row from the speed table below
// (the last selected level is kept until another one is pressed; default = Lo)
//
//  Buttons        | Action            | Left motor     | Right motor
//  ---------------+-------------------+----------------+---------------
//  UP             | Forward           | +SPD           | +SPD
//  DOWN           | Backward          | -SPD           | -SPD
//  LEFT           | Spin left         | -TRN           | +TRN
//  RIGHT          | Spin right        | +TRN           | -TRN
//  UP + LEFT      | Curve fwd-left    | +(SPD-TRN)     | +SPD
//  UP + RIGHT     | Curve fwd-right   | +SPD           | +(SPD-TRN)
//  DOWN + LEFT    | Curve back-left   | -(SPD-TRN)     | -SPD
//  DOWN + RIGHT   | Curve back-right  | -SPD           | -(SPD-TRN)
//  (none)         | Stop              | 0              | 0

#include <Arduino.h>
#include <Wire.h>
#include "NKP_V2Motor_drive.h"  // copied from KBIDE boards/NKP_V2/include (motor() via I2C 0x20)
#include <PBGamepad.h>  // https://github.com/Amornthep-ra/PB-BLE

// ---- BLE settings (change before use) ----
// Board ID : 1-32 English letters/digits, unique per robot
// Pair Code: 8-16 English letters/digits
const char *BLE_NAME  = "PB-NKPV201";
const char *BOARD_ID  = "NKPV201";
const char *PAIR_CODE = "12345678";

// ---- Speed table (0..100), edit to taste ----
//                          Lo   Med  Hi
const int DRIVE_SPEED[3] = { 30,  60, 100 };  // forward / backward
const int TURN_SPEED[3]  = { 25,  35,  50 };  // spin + how tight curves are


// Bits of CMD (PB_GetButtonsLow)
const uint8_t BTN_UP    = 1;
const uint8_t BTN_DOWN  = 2;
const uint8_t BTN_LEFT  = 4;
const uint8_t BTN_RIGHT = 8;

int speedLevel = 0;            // 0 = Lo, 1 = Med, 2 = Hi
// ---- Drive type ----
// 2 = 2WD : M1 = left wheel, M2 = right wheel        (same as KBIDE block "Move 2WD")
// 4 = 4WD : left motor pair + right motor pair       (same as KBIDE block "Move 4WD")
#define DRIVE_TYPE  2   // <-- 2 or 4

// One side of the robot, speed -100..100 (uses the NKP_V2 motor controller states)
void sideMotor(bool leftSide, int speed) {
  uint8_t fwd, back;
#if DRIVE_TYPE == 4
  fwd  = leftSide ? 16 : 18;
  back = leftSide ? 15 : 17;
#else
  fwd  = leftSide ? 0 : 2;
  back = leftSide ? 1 : 3;
#endif
  if (speed >= 0) motor_control(fwd, speed);
  else            motor_control(back, -speed);
}

int lastL = 9999, lastR = 9999;

void drive(int left, int right) {
  static uint32_t lastSend = 0;
  // write when changed, and resend every 100 ms (I2C motor controller)
  if (left == lastL && right == lastR && millis() - lastSend < 100) return;
  sideMotor(true,  left);   // left side  (2WD: M1, 4WD: left pair)
  sideMotor(false, right);  // right side (2WD: M2, 4WD: right pair)
  lastL = left;
  lastR = right;
  lastSend = millis();
}

void stopMotors() {
  drive(0, 0);
}

// ---- NKP_V2: motors are driven by the on-board I2C motor controller (address 0x20) ----
volatile bool bleStopRequest = false;

void NKP_V2_MotorInit() {
  Wire.begin();
  // Same detection as NKP_V2(): no reply on Wire -> controller is on Wire1 (GPIO 16/17)
  if (ADC(0) == 0 && ADC(1) == 0 && ADC(2) == 0 && ADC(4) == 0 &&
      ADC(5) == 0 && ADC(6) == 0 && ADC(7) == 0) {
    select_conection_i2c = 1;
    Wire1.begin(16, 17);
  }
  ao();   // stop all motors
}

// Called from the BLE task on disconnect / timeout.
// Do NOT use I2C here (not thread-safe): only raise a flag, loop() stops the motors.
void NKP_V2_BleStop() {
  bleStopRequest = true;
}

void setup() {
  Serial.begin(115200);
  NKP_V2_MotorInit();

  // Hardware stop when BLE disconnects / times out (runs in BLE task)
  PBGamepad_setEmergencyStopCallback(NKP_V2_BleStop);
  PBGamepad_init(BLE_NAME, BOARD_ID, PAIR_CODE);

  lastL = lastR = 9999;
  stopMotors();
}

void loop() {
  PBGamepad_poll();

  // Stop requested by the BLE task (disconnect / timeout)
  if (bleStopRequest) {
    bleStopRequest = false;
    lastL = lastR = 9999;
    stopMotors();
  }

  // No fresh control data (not connected / not Start / timeout) -> stop
  if (!PBGamepad_isControlFresh()) {
    stopMotors();
    delay(10);
    return;
  }

  // Lo / Med / Hi from the app (0 / 1 / 2), -1 = not selected yet
  int lv = PBGamepad_getSpeed();
  if (lv >= 0 && lv <= 2 && lv != speedLevel) {
    speedLevel = lv;
    Serial.printf("Speed level: %s\n", speedLevel == 0 ? "Lo" : speedLevel == 1 ? "Med" : "Hi");
  }

  int spd = DRIVE_SPEED[speedLevel];
  int trn = TURN_SPEED[speedLevel];

  uint8_t cmd = PB_GetButtonsLow() & 0x0F;   // use only UP/DOWN/LEFT/RIGHT

  // Opposite buttons cancel each other
  if ((cmd & BTN_UP) && (cmd & BTN_DOWN))    cmd &= ~(BTN_UP | BTN_DOWN);
  if ((cmd & BTN_LEFT) && (cmd & BTN_RIGHT)) cmd &= ~(BTN_LEFT | BTN_RIGHT);

  int inner = max(spd - trn, 0);  // inner wheel speed for curves

  switch (cmd) {
    case BTN_UP:               drive( spd,    spd);    break;  // forward
    case BTN_DOWN:             drive(-spd,   -spd);    break;  // backward
    case BTN_LEFT:             drive(-trn,    trn);    break;  // spin left
    case BTN_RIGHT:            drive( trn,   -trn);    break;  // spin right
    case BTN_UP | BTN_LEFT:    drive( inner,  spd);    break;  // curve forward-left
    case BTN_UP | BTN_RIGHT:   drive( spd,    inner);  break;  // curve forward-right
    case BTN_DOWN | BTN_LEFT:  drive(-inner, -spd);    break;  // curve backward-left
    case BTN_DOWN | BTN_RIGHT: drive(-spd,   -inner);  break;  // curve backward-right
    default:                   stopMotors();           break;  // released
  }

  delay(10);
}
