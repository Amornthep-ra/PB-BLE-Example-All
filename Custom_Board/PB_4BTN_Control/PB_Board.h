// PB_Board.h  --  motor settings for YOUR OWN board (ESP32 / ESP32-C3)
//
// This is the only file you need to edit to use the PB examples with your board.
// The .ino calls just 3 functions from here:
//   boardMotorInit()          -> set up motor pins (called once in setup)
//   boardSetMotors(L, R)      -> drive left / right side, -100..100
//   boardFailsafeStop()       -> stop motors; called from the BLE task on disconnect
//
// Steps:
//   1) Pick MOTOR_DRIVER that matches your motor driver chip
//   2) Fill in LEFT_PINS / RIGHT_PINS with your GPIO numbers (-1 = not used)
//   3) 4 motors? set DRIVE_TYPE 4 and fill LEFT2_PINS / RIGHT2_PINS
//   4) Wheel spins the wrong way? set INVERT_LEFT / INVERT_RIGHT to true
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// 1) Motor driver type
// ---------------------------------------------------------------------------
#define DRIVER_IN1_IN2     1  // 2 PWM pins per motor      : DRV8833, L9110S, MX1508, TB6612 (PWM on IN1/IN2)
#define DRIVER_DIR_PWM     2  // DIR pin + PWM pin per motor: DRV8835 (PH/EN), many robot boards
#define DRIVER_IN1_IN2_EN  3  // IN1 + IN2 + EN (PWM) pins  : L298N, TB6612FNG (AIN1/AIN2/PWMA)

#define MOTOR_DRIVER  DRIVER_IN1_IN2   // <-- choose 1, 2 or 3

// ---------------------------------------------------------------------------
// 2) Pins (GPIO numbers). -1 = not used / not connected yet
// ---------------------------------------------------------------------------
//   type 1: { IN1(PWM), IN2(PWM), -1      }
//   type 2: { DIR,      PWM,      -1      }
//   type 3: { IN1,      IN2,      EN(PWM) }
const int LEFT_PINS[3]   = { -1, -1, -1 };   // left motor  (2WD) / front-left  (4WD)
const int RIGHT_PINS[3]  = { -1, -1, -1 };   // right motor (2WD) / front-right (4WD)

// ---------------------------------------------------------------------------
// 3) Drive type: 2 = 2WD, 4 = 4WD (second pair below is used only for 4WD)
// ---------------------------------------------------------------------------
#define DRIVE_TYPE  2   // <-- 2 or 4
const int LEFT2_PINS[3]  = { -1, -1, -1 };   // rear-left  (4WD only)
const int RIGHT2_PINS[3] = { -1, -1, -1 };   // rear-right (4WD only)

// Optional: a pin that must be HIGH to enable the driver (STBY on TB6612, EN on some boards)
const int DRIVER_ENABLE_PIN = -1;

// ---------------------------------------------------------------------------
// 4) Direction fix and PWM
// ---------------------------------------------------------------------------
const bool INVERT_LEFT  = false;   // true = reverse left side
const bool INVERT_RIGHT = false;   // true = reverse right side
const int  PWM_FREQ = 1000;        // Hz
const int  PWM_RES  = 8;           // bits (8 -> duty 0..255)

// ===========================================================================
// Normally you do not need to edit below this line
// ===========================================================================

static inline bool pbPinsSet(const int p[3]) {
#if MOTOR_DRIVER == DRIVER_IN1_IN2_EN
  return p[0] >= 0 && p[1] >= 0 && p[2] >= 0;
#else
  return p[0] >= 0 && p[1] >= 0;
#endif
}

static inline void pbMotorPinsInit(const int p[3]) {
  if (!pbPinsSet(p)) return;
  for (int i = 0; i < 3; i++) {
    if (p[i] >= 0) { pinMode(p[i], OUTPUT); digitalWrite(p[i], LOW); }
  }
#if MOTOR_DRIVER == DRIVER_IN1_IN2
  ledcAttach(p[0], PWM_FREQ, PWM_RES);
  ledcAttach(p[1], PWM_FREQ, PWM_RES);
#elif MOTOR_DRIVER == DRIVER_DIR_PWM
  ledcAttach(p[1], PWM_FREQ, PWM_RES);
#else
  ledcAttach(p[2], PWM_FREQ, PWM_RES);
#endif
}

// speed -100..100 ; 0 = stop (coast)
static inline void pbMotorWrite(const int p[3], int speed) {
  if (!pbPinsSet(p)) return;
  speed = constrain(speed, -100, 100);
  const uint32_t duty = (uint32_t)abs(speed) * ((1u << PWM_RES) - 1) / 100;
#if MOTOR_DRIVER == DRIVER_IN1_IN2
  ledcWrite(p[0], speed > 0 ? duty : 0);
  ledcWrite(p[1], speed < 0 ? duty : 0);
#elif MOTOR_DRIVER == DRIVER_DIR_PWM
  digitalWrite(p[0], speed < 0 ? HIGH : LOW);
  ledcWrite(p[1], duty);
#else
  digitalWrite(p[0], speed > 0 ? HIGH : LOW);
  digitalWrite(p[1], speed < 0 ? HIGH : LOW);
  ledcWrite(p[2], duty);
#endif
}

// Left / right side, -100..100
void boardSetMotors(int left, int right) {
  if (INVERT_LEFT)  left  = -left;
  if (INVERT_RIGHT) right = -right;
  pbMotorWrite(LEFT_PINS, left);
  pbMotorWrite(RIGHT_PINS, right);
#if DRIVE_TYPE == 4
  pbMotorWrite(LEFT2_PINS, left);
  pbMotorWrite(RIGHT2_PINS, right);
#endif
}

void boardMotorInit() {
  pbMotorPinsInit(LEFT_PINS);
  pbMotorPinsInit(RIGHT_PINS);
#if DRIVE_TYPE == 4
  pbMotorPinsInit(LEFT2_PINS);
  pbMotorPinsInit(RIGHT2_PINS);
#endif
  boardSetMotors(0, 0);
  if (DRIVER_ENABLE_PIN >= 0) {
    pinMode(DRIVER_ENABLE_PIN, OUTPUT);
    digitalWrite(DRIVER_ENABLE_PIN, HIGH);
  }

  bool ok = pbPinsSet(LEFT_PINS) && pbPinsSet(RIGHT_PINS);
#if DRIVE_TYPE == 4
  ok = ok && pbPinsSet(LEFT2_PINS) && pbPinsSet(RIGHT2_PINS);
#endif
  Serial.printf("[PB_Board] driver type %d, %dWD\n", MOTOR_DRIVER, DRIVE_TYPE);
  if (!ok) Serial.println("[PB_Board] WARNING: motor pins are not set in PB_Board.h -> motors will not move");
}

// Called from the BLE task on disconnect / timeout.
// Keep it short: only GPIO / PWM writes here (no delay, Serial, I2C or BLE calls).
void boardFailsafeStop() {
  boardSetMotors(0, 0);
}
