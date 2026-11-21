// PB_Joystick_Control.ino
// ESPRobotV2 (ESP32) + PB Controller app, Joystick mode
//
// Pick a driving style with JOY_MODE below, then upload:
//
//  JOY_MODE | Name              | How to drive
//  ---------+-------------------+------------------------------------------------------
//     1     | ARCADE_DUAL       | Left stick Y = forward/back, Right stick X = turn
//     2     | ARCADE_DUAL_SWAP  | Right stick Y = forward/back, Left stick X = turn
//     3     | ARCADE_LEFT       | Left stick only (Y = forward/back, X = turn)
//     4     | ARCADE_RIGHT      | Right stick only (Y = forward/back, X = turn)
//     5     | TANK              | Left stick Y = left wheel, Right stick Y = right wheel
//     6     | CAR               | Left stick Y = throttle, Right stick X = steering
//           |                   | (no spin in place; steering only slows the inner wheel)
//
// Arcade mix (modes 1-4):  Left = forward + turn, Right = forward - turn
// (scaled down together if either side exceeds 100)

#include <Arduino.h>
#include <Wire.h>
#include "NKP_V2Motor_drive.h"  // copied from KBIDE boards/ESPRobotV2/include (ESPRobotV2 version: direct PWM, 4 motors)
#include <PBGamepad.h>    // https://github.com/Amornthep-ra/PB-BLE
#include <PBJoystick.h>

// ---- BLE settings (change before use) ----
// Board ID : 1-32 English letters/digits, unique per robot
// Pair Code: 8-16 English letters/digits
const char *BLE_NAME  = "PB-ESPRobotV201";
const char *BOARD_ID  = "ESPRobotV201";
const char *PAIR_CODE = "12345678";

// ---- Driving style ----
#define ARCADE_DUAL       1
#define ARCADE_DUAL_SWAP  2
#define ARCADE_LEFT       3
#define ARCADE_RIGHT      4
#define TANK              5
#define CAR               6

#define JOY_MODE  ARCADE_DUAL   // <-- choose 1..6 (or the name)

// ---- Tuning (0..100) ----
const int DEADZONE  = 15;   // stick center zone ignored; speed starts from 0 after it
const int MAX_SPEED = 100;  // cap for every wheel (lower = slower robot)
const int TURN_MAX  = 25;   // modes 1-4: turn amount at full stick
const int CAR_STEER = 70;   // mode 6: how much the inner wheel slows at full steering (%)


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
  left  = left  * MAX_SPEED / 100;
  right = right * MAX_SPEED / 100;
  if (left == lastL && right == lastR) return;   // write only when changed
  sideMotor(true,  left);   // left side  (2WD: M1, 4WD: M1 + M3)
  sideMotor(false, right);  // right side (2WD: M2, 4WD: M2 + M4)
  lastL = left;
  lastR = right;
}

void stopMotors() {
  lastL = lastR = 9999;   // force a write
  drive(0, 0);
}

// -100..100 -> 0 inside deadzone, otherwise rescaled to -100..100
int applyDeadzone(int v) {
  if (abs(v) <= DEADZONE) return 0;
  int mag = (abs(v) - DEADZONE) * 100 / (100 - DEADZONE);
  mag = constrain(mag, 0, 100);
  return (v < 0) ? -mag : mag;
}

// forward / turn in -100..100 -> wheels
void arcade(int forward, int turn) {
  turn = turn * TURN_MAX / 100;
  int left  = PB_BLE_ArcadeMixLeft (forward / 100.0f, turn / 100.0f);
  int right = PB_BLE_ArcadeMixRight(forward / 100.0f, turn / 100.0f);
  drive(left, right);
}

// Car style: no movement without throttle, steering slows the inner wheel
void car(int throttle, int steer) {
  int inner = throttle * (100 - abs(steer) * CAR_STEER / 100) / 100;
  if (steer > 0)      drive(throttle, inner);   // steer right
  else if (steer < 0) drive(inner, throttle);   // steer left
  else                drive(throttle, throttle);
}

const char *modeName() {
  switch (JOY_MODE) {
    case ARCADE_DUAL:      return "ARCADE_DUAL";
    case ARCADE_DUAL_SWAP: return "ARCADE_DUAL_SWAP";
    case ARCADE_LEFT:      return "ARCADE_LEFT";
    case ARCADE_RIGHT:     return "ARCADE_RIGHT";
    case TANK:             return "TANK";
    case CAR:              return "CAR";
    default:               return "UNKNOWN";
  }
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

  stopMotors();

  Serial.printf("Joystick mode: %d (%s)\n", JOY_MODE, modeName());
}

void loop() {
  PB_JoystickDual_updateAxes();   // includes PBGamepad_poll()

  // No fresh control data (not connected / not Start / timeout) -> stop
  if (!PBGamepad_isControlFresh()) {
    if (lastL != 0 || lastR != 0) stopMotors();
    delay(10);
    return;
  }

  // Raw axes: up Y is negative, right X is positive -> flip Y so + = forward
  int lx = applyDeadzone( PB_JoystickDual_getLX100());
  int ly = applyDeadzone(-PB_JoystickDual_getLY100());
  int rx = applyDeadzone( PB_JoystickDual_getRX100());
  int ry = applyDeadzone(-PB_JoystickDual_getRY100());

  switch (JOY_MODE) {
    case ARCADE_DUAL:      arcade(ly, rx); break;
    case ARCADE_DUAL_SWAP: arcade(ry, lx); break;
    case ARCADE_LEFT:      arcade(ly, lx); break;
    case ARCADE_RIGHT:     arcade(ry, rx); break;
    case TANK:             drive(ly, ry);  break;
    case CAR:              car(ly, rx);    break;
    default:               stopMotors();   break;
  }

  delay(10);
}
