// PB_8BTN_Control.ino
// NKP_ONE (ESP32) + PB Controller app, 8BTN mode
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
#include "NKP_Motor_drive.h"  // copied from KBIDE boards/NKP_ONE/include (motor())
#include <PBGamepad.h>  // https://github.com/Amornthep-ra/PB-BLE

// ---- BLE settings (change before use) ----
// Board ID : 1-32 English letters/digits, unique per robot
// Pair Code: 8-16 English letters/digits
const char *BLE_NAME  = "PB-NKPONE01";
const char *BOARD_ID  = "NKPONE01";
const char *PAIR_CODE = "12345678";


// Bits of CMD (PB_GetButtonsLow)
const uint8_t BTN_UP    = 1;
const uint8_t BTN_DOWN  = 2;
const uint8_t BTN_LEFT  = 4;
const uint8_t BTN_RIGHT = 8;

int lastL = 9999, lastR = 9999;

void drive(int left, int right) {
  if (left == lastL && right == lastR) return;   // write only when changed
  motor(1, left);   // left wheel
  motor(2, right);  // right wheel
  lastL = left;
  lastR = right;
}

void stopMotors() {
  drive(0, 0);
}

// ---- NKP_ONE motor pins (same as NKP_ONE.h): M1 = GPIO 2/4, M2 = GPIO 16/17 ----
void NKP_MotorInit() {
  const uint8_t pins[4]  = { 2, 4, 16, 17 };
  const uint8_t chans[4] = { 6, 7,  4,  5 };
  for (int i = 0; i < 4; i++) {
    pinMode(pins[i], OUTPUT);
    ledcAttachChannel(pins[i], 1000, 8, chans[i]);
    ledcWrite(pins[i], 255);   // both inputs HIGH = stop
  }
}

// Hardware stop (called from the BLE task on disconnect / timeout)
void NKP_MotorFailsafeStop() {
  ledcWrite(2, 255);
  ledcWrite(4, 255);
  ledcWrite(16, 255);
  ledcWrite(17, 255);
}

void setup() {
  Serial.begin(115200);
  NKP_MotorInit();

  // Hardware stop when BLE disconnects / times out (runs in BLE task)
  PBGamepad_setEmergencyStopCallback(NKP_MotorFailsafeStop);
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
