// PB_Joystick_Control.ino
// NKP_V2 (ESP32) + PB Controller app, Joystick mode
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
#include "NKP_V2Motor_drive.h"  // copied from KBIDE boards/NKP_V2/include (motor() via I2C 0x20)
#include <PBGamepad.h>    // https://github.com/Amornthep-ra/PB-BLE
#include <PBJoystick.h>

// ---- BLE settings (change before use) ----
// Board ID : 1-32 English letters/digits, unique per robot
// Pair Code: 8-16 English letters/digits
const char *BLE_NAME  = "PB-NKPV201";
const char *BOARD_ID  = "NKPV201";
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
  left  = left  * MAX_SPEED / 100;
  right = right * MAX_SPEED / 100;
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

  stopMotors();

  Serial.printf("Joystick mode: %d (%s)\n", JOY_MODE, modeName());
}

void loop() {
  PB_JoystickDual_updateAxes();   // includes PBGamepad_poll()

  // Stop requested by the BLE task (disconnect / timeout)
  if (bleStopRequest) {
    bleStopRequest = false;
    lastL = lastR = 9999;
    stopMotors();
  }

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
