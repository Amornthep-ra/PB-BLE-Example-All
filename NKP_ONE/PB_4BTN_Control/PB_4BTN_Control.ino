// PB_4BTN_Control.ino
// NKP_ONE (ESP32) + PB Controller app, 4BTN mode
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
#include "NKP_Motor_drive.h"  // copied from KBIDE boards/NKP_ONE/include (motor())
#include <PBGamepad.h>  // https://github.com/Amornthep-ra/PB-BLE

// ---- BLE settings (change before use) ----
// Board ID : 1-32 English letters/digits, unique per robot
// Pair Code: 8-16 English letters/digits
const char *BLE_NAME  = "PB-NKPONE01";
const char *BOARD_ID  = "NKPONE01";
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
