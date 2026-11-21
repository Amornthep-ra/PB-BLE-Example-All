// PB_8BTN_Control.ino
// ESPRobotV2 (ESP32) + PB Controller app, 8BTN mode
//
// Buttons (CMD bits): UP=1, DOWN=2, LEFT=4, RIGHT=8
// DRV slider = forward/backward speed (0..100)
// TRN slider = turn speed (0..100)
//
//  CMD | Buttons        | Action            | Left motor     | Right motor
//  ----+----------------+-------------------+----------------+---------------
//   1  | UP             | Forward           | +DRV           | +DRV
//   2  | DOWN           | Backward          | -DRV           | -DRV
//   4  | LEFT           | Spin left         | -TRN           | +TRN
//   8  | RIGHT          | Spin right        | +TRN           | -TRN
//   5  | UP + LEFT      | Curve fwd-left    | +(DRV-TRN)     | +DRV
//   9  | UP + RIGHT     | Curve fwd-right   | +DRV           | +(DRV-TRN)
//   6  | DOWN + LEFT    | Curve back-left   | -(DRV-TRN)     | -DRV
//  10  | DOWN + RIGHT   | Curve back-right  | -DRV           | -(DRV-TRN)
//   0  | (none)         | Stop              | 0              | 0
//  UP+DOWN or LEFT+RIGHT pressed together cancel each other out.

#include <Arduino.h>
#include <Wire.h>
#include "NKP_V2Motor_drive.h"  // copied from KBIDE boards/ESPRobotV2/include (ESPRobotV2 version: direct PWM, 4 motors)
#include <PBGamepad.h>  // https://github.com/Amornthep-ra/PB-BLE

// ---- BLE settings (change before use) ----
// Board ID : 1-32 English letters/digits, unique per robot
// Pair Code: 8-16 English letters/digits
const char *BLE_NAME  = "PB-ESPRobotV201";
const char *BOARD_ID  = "ESPRobotV201";
const char *PAIR_CODE = "12345678";


// Bits of CMD (PB_GetButtonsLow)
const uint8_t BTN_UP    = 1;
const uint8_t BTN_DOWN  = 2;
const uint8_t BTN_LEFT  = 4;
const uint8_t BTN_RIGHT = 8;

// ---- Drive type ----
// 2 = 2WD : M1 = left wheel, M2 = right wheel           (same as KBIDE block "Move 2WD")
// 4 = 4WD : M1 + M3 = left side, M2 + M4 = right side   (same as KBIDE block "Move 4WD")
#define DRIVE_TYPE  2   // <-- 2 or 4

// One side of the robot, speed -100..100
void sideMotor(bool leftSide, int speed) {
  motor(leftSide ? 1 : 2, speed);
#if DRIVE_TYPE == 4
  motor(leftSide ? 3 : 4, speed);
#endif
}

int lastL = 9999, lastR = 9999;

void drive(int left, int right) {
  if (left == lastL && right == lastR) return;   // write only when changed
  sideMotor(true,  left);   // left side  (2WD: M1, 4WD: M1 + M3)
  sideMotor(false, right);  // right side (2WD: M2, 4WD: M2 + M4)
  lastL = left;
  lastR = right;
}

void stopMotors() {
  drive(0, 0);
}

// ---- ESPRobotV2 motor pins (same as ESPRobotV2.h) ----
//                         M1A M1B M2A M2B M3A M3B M4A M4B
const uint8_t MOTOR_PINS[8] = { 19, 18,  5, 17, 16,  4,  2, 15 };
const uint8_t MOTOR_CHAN[8] = {  6,  7,  4,  5,  3,  2,  0,  1 };

void ESPRobotV2_MotorInit() {
  for (int i = 0; i < 8; i++) {
    pinMode(MOTOR_PINS[i], OUTPUT);
    ledcAttachChannel(MOTOR_PINS[i], 5000, 8, MOTOR_CHAN[i]);
    ledcWriteChannel(MOTOR_CHAN[i], 255);   // both inputs HIGH = stop
  }
}

// Hardware stop (called from the BLE task on disconnect / timeout)
void ESPRobotV2_MotorFailsafeStop() {
  for (uint8_t ch = 0; ch < 8; ch++) ledcWriteChannel(ch, 255);
}

void setup() {
  Serial.begin(115200);
  ESPRobotV2_MotorInit();

  // Hardware stop when BLE disconnects / times out (runs in BLE task)
  PBGamepad_setEmergencyStopCallback(ESPRobotV2_MotorFailsafeStop);
  PBGamepad_init(BLE_NAME, BOARD_ID, PAIR_CODE);

  lastL = lastR = 9999;
  stopMotors();
}

void loop() {
  PBGamepad_poll();

  // No fresh control data (not connected / not Start / timeout) -> stop
  if (!PBGamepad_isControlFresh()) {
    stopMotors();
    delay(10);
    return;
  }

  uint8_t cmd = PB_GetButtonsLow() & 0x0F;   // use only UP/DOWN/LEFT/RIGHT
  int drv = PB_GetDriveSpeed();              // DRV 0..100
  int trn = PB_GetTurnSpeed();               // TRN 0..100

  // Opposite buttons cancel each other
  if ((cmd & BTN_UP) && (cmd & BTN_DOWN))    cmd &= ~(BTN_UP | BTN_DOWN);
  if ((cmd & BTN_LEFT) && (cmd & BTN_RIGHT)) cmd &= ~(BTN_LEFT | BTN_RIGHT);

  int inner = max(drv - trn, 0);  // inner wheel speed for curves

  switch (cmd) {
    case BTN_UP:               drive( drv,    drv);    break;  // forward
    case BTN_DOWN:             drive(-drv,   -drv);    break;  // backward
    case BTN_LEFT:             drive(-trn,    trn);    break;  // spin left
    case BTN_RIGHT:            drive( trn,   -trn);    break;  // spin right
    case BTN_UP | BTN_LEFT:    drive( inner,  drv);    break;  // curve forward-left
    case BTN_UP | BTN_RIGHT:   drive( drv,    inner);  break;  // curve forward-right
    case BTN_DOWN | BTN_LEFT:  drive(-inner, -drv);    break;  // curve backward-left
    case BTN_DOWN | BTN_RIGHT: drive(-drv,   -inner);  break;  // curve backward-right
    default:                   stopMotors();           break;  // released
  }

  delay(10);
}
