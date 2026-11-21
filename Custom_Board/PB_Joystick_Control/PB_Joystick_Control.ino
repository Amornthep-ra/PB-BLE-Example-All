// PB_Joystick_Control.ino
// Custom board (any ESP32 / ESP32-C3) -- set your motor pins in PB_Board.h + PB Controller app, Joystick mode
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
#include "PB_Board.h"   // <-- your motor driver + pins (edit this file)
#include <PBGamepad.h>    // https://github.com/Amornthep-ra/PB-BLE
#include <PBJoystick.h>

// ---- BLE settings (change before use) ----
// Board ID : 1-32 English letters/digits, unique per robot
// Pair Code: 8-16 English letters/digits
const char *BLE_NAME  = "PB-MyRobot01";
const char *BOARD_ID  = "MyRobot01";
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


int lastL = 9999, lastR = 9999;

void drive(int left, int right) {
  left  = left  * MAX_SPEED / 100;
  right = right * MAX_SPEED / 100;
  if (left == lastL && right == lastR) return;   // write only when changed
  boardSetMotors(left, right);   // see PB_Board.h
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

void setup() {
  Serial.begin(115200);
  boardMotorInit();   // see PB_Board.h

  // Hardware stop when BLE disconnects / times out (runs in BLE task)
  PBGamepad_setEmergencyStopCallback(boardFailsafeStop);
  PBGamepad_init(BLE_NAME, BOARD_ID, PAIR_CODE);
  boardMotorInit();   // set up PWM again: BLE start-up may reset PWM on some chips (ESP32-C3)

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
