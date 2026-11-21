#include <Arduino.h>

constexpr uint8_t PICOMINI_SW1_PIN = 9;

int motor1A = 21;
int motor1B = 20;
int motor2A = 10;
int motor2B = 8;

const int MOTOR_PWM_FREQ = 2000;
const int MOTOR_PWM_RES  = 8;

#ifndef PICOMINI_MOTOR_DIAGNOSTICS
#define PICOMINI_MOTOR_DIAGNOSTICS 0
#endif

const int SERVO_FREQ   = 50;
const int SERVO_RES    = 14;
const int SERVO_TAU_US = 20000;
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;

int _sensorPins[20];
int _NumofSensor = 0;
int _min_sensor_values[20];
int _max_sensor_values[20];
double _lastPosition = 0;
int _Sensitive  = 20;
double errors = 0, output = 0, integral = 0, derivative = 0, previous_error = 0;
uint8_t FrontLineColor = 0;
uint8_t BackLineColor = 0;

static bool _srv5_attached = false;
static bool _srv6_attached = false;
static bool _srv7_attached = false;
static bool _motor1_pwm_attached = false;
static bool _motor2_pwm_attached = false;

static inline uint32_t _usToDuty(int us) {
  if (us < 0) us = 0;
  if (us > SERVO_TAU_US) us = SERVO_TAU_US;
  return map(us, 0, SERVO_TAU_US, 0, (1 << SERVO_RES) - 1);
}

static int _servoChannelForPin(uint8_t pin) {
  switch (pin) {
    case 5: return 0;
    case 6: return 1;
    case 7: return 2;
    default: return -1;
  }
}

static bool _servoEnsureAttached(uint8_t pin) {
  int ch = _servoChannelForPin(pin);
  if (ch < 0) return false;

  bool* flag = (pin == 5) ? &_srv5_attached : (pin == 6) ? &_srv6_attached : &_srv7_attached;
  if (!*flag) {
    ledcAttach(pin, SERVO_FREQ, SERVO_RES);
    ledcWrite(pin, _usToDuty(1500)); // center 90°
    *flag = true;
  }
  return true;
}

// เรียกแบบ servo(5, 90);
void servo(uint8_t pin, int angle) {
  if (!_servoEnsureAttached(pin)) return;
  angle = constrain(angle, 0, 180);
  int us = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  ledcWrite(pin, _usToDuty(us));
}

void servoDetach(uint8_t pin) {
  int ch = _servoChannelForPin(pin);
  if (ch < 0) return;
  ledcDetach(pin);
  if (pin == 5) _srv5_attached = false;
  else if (pin == 6) _srv6_attached = false;
  else _srv7_attached = false;
}

// ================== Motor control (ใช้ LEDC ชุดมอเตอร์) ==================
void motor(int pin, float speed_Motor) {
  if (speed_Motor > 100) speed_Motor = 100;
  if (speed_Motor < -100) speed_Motor = -100;

  int duty = (int)round(abs(speed_Motor) * 2.55f);  // 0..255 (8-bit)

  if (pin == 1) { // ล้อซ้าย
    digitalWrite(motor1A, (speed_Motor >= 0) ? HIGH : LOW);
    // Re-attach LEDC if it was reset (e.g. by NimBLE init on ESP32-C3)
    if (!ledcWrite(motor1B, duty)) {
      _motor1_pwm_attached = ledcAttach(motor1B, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
      ledcWrite(motor1B, duty);
    }
#if PICOMINI_MOTOR_DIAGNOSTICS
    Serial.printf("[PicoMini] M1 attach=%d duty=%d\n", _motor1_pwm_attached, duty);
#endif
  } else if (pin == 2) { // ล้อขวา
    digitalWrite(motor2A, (speed_Motor >= 0) ? HIGH : LOW);
    if (!ledcWrite(motor2B, duty)) {
      _motor2_pwm_attached = ledcAttach(motor2B, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
      ledcWrite(motor2B, duty);
    }
#if PICOMINI_MOTOR_DIAGNOSTICS
    Serial.printf("[PicoMini] M2 attach=%d duty=%d\n", _motor2_pwm_attached, duty);
#endif
  }
  delay(1);
}

// Overload for block code that passes direction separately (dir: 0=stop, 1=forward, 2=backward, speed 0..100)
void motor(int pin, int dir, float speedAbs) {
  if (speedAbs < 0) speedAbs = -speedAbs;
  float signedSpeed = (dir == 2) ? -speedAbs : (dir == 1 ? speedAbs : 0);
  motor(pin, signedSpeed);
}

// Strong override for PB-Xbox-ESP32 plugin: stop motors on BLE disconnect failsafe.
extern "C" void PBXboxMotorFailsafeStop() {
  digitalWrite(motor1A, LOW);
  digitalWrite(motor2A, LOW);
  ledcWrite(motor1B, 0);
  ledcWrite(motor2B, 0);
}

void PicoMini() {
  // Pull all motor pins LOW before ledcAttach to minimise boot-time motor activation.
  // GPIO 20/21 can float HIGH on the PicoMini PCB causing motor1 to spin briefly
  // during the boot ROM phase. Setting them LOW here shortens that window.
  pinMode(motor1B, OUTPUT); digitalWrite(motor1B, LOW);
  pinMode(motor2B, OUTPUT); digitalWrite(motor2B, LOW);
  pinMode(motor1A, OUTPUT); digitalWrite(motor1A, LOW);
  pinMode(motor2A, OUTPUT); digitalWrite(motor2A, LOW);

  // ตั้งค่า PWM มอเตอร์ (2 kHz / 8-bit)
  _motor1_pwm_attached = ledcAttach(motor1B, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
  _motor2_pwm_attached = ledcAttach(motor2B, MOTOR_PWM_FREQ, MOTOR_PWM_RES);

  motor(1, 0);
  motor(2, 0);

  // // ปุ่ม (ถ้ามี)
  // pinMode(9, INPUT);

  // ตั้ง ADC ความละเอียด 12-bit (0..4095) สำหรับ GPIO0 (ADC-IN)
  analogReadResolution(12);

  // รีเซ็ต min/max ของเซ็นเซอร์
  for (uint8_t i = 0; i < _NumofSensor; i++) {
    _max_sensor_values[i] = 0;
    _min_sensor_values[i] = 4095;
  }
}

void wait_SW1(){
  pinMode(PICOMINI_SW1_PIN, INPUT_PULLUP);
  while(digitalRead(PICOMINI_SW1_PIN) == HIGH){
    delay(1); // กัน watchdog
  }
}

// ================== Debug reads ==================
void readsensors() {
  Serial.print(" A0:  "); Serial.print(analogRead(0));
  Serial.print(" A1:  "); Serial.print(analogRead(1));
  Serial.print(" A2:  "); Serial.print(analogRead(2));
  Serial.print(" A3:  "); Serial.print(analogRead(3));
  Serial.print(" A4:  "); Serial.print(analogRead(4));
  Serial.print(" D5:  "); Serial.print(digitalRead(5));
  Serial.print(" D6:  "); Serial.print(digitalRead(6));
  Serial.print(" D7:  "); Serial.println(digitalRead(7));
}

// ================== ADC via MUX (S0=GPIO1, S1=GPIO2, S2=GPIO3; ADC-IN=GPIO0) ==================
int ADC_read(int analog_CH) {
  int val = 0;
  if (analog_CH < 8 ) {
    pinMode(1, OUTPUT);  // S0
    pinMode(2, OUTPUT);  // S1
    pinMode(3, OUTPUT);  // S2
    static const uint8_t muxChannel[8][3] = {
      {1,1,1},{1,1,0},{1,0,1},{1,0,0},{0,1,1},{0,1,0},{0,0,1},{0,0,0}
    };
    digitalWrite(3, muxChannel[analog_CH][0]);
    digitalWrite(2, muxChannel[analog_CH][1]);
    digitalWrite(1, muxChannel[analog_CH][2]);
    delayMicroseconds(5);               // settle time
    val = analogRead(0);                // ADC-IN (GPIO0 on ADC1)
  }
  return val;
}

void ReadADCAll(){
  for(int i = 0;i<8;i++){
    Serial.print("  ADC"+String(i)+"=");
    Serial.print(ADC_read(i));
  }
  Serial.println("");
}

// ================== Sensor calibration / mapping ==================
void setSensorPins(const int * _pins, int _NumofSensor_) {
  _NumofSensor = _NumofSensor_;
  for (uint8_t i = 0; i < _NumofSensor_; i++) {
    _sensorPins[i] = _pins[i];
    _min_sensor_values[i] = 4095;
    _max_sensor_values[i] = 0;
  }
}
void setSensorMin(const int * _MinSensor) {
  for (uint8_t i = 0; i < _NumofSensor; i++) _min_sensor_values[i] = _MinSensor[i];
}
void setSensorMax(const int * _MaxSensor) {
  for (uint8_t i = 0; i < _NumofSensor; i++) _max_sensor_values[i] = _MaxSensor[i];
}
void setSensitive(const uint16_t  _SensorSensitive) { _Sensitive = _SensorSensitive; }
void setFrontLineColor(const uint16_t  _setFrontLineColor) { FrontLineColor = _setFrontLineColor; }

int refSensor(int ch){ return ( _max_sensor_values[ch] + _min_sensor_values[ch] ) / 2 ; }
int readSensorMinValue(uint8_t _Pin) { return _min_sensor_values[_Pin]; }
int readSensorMaxValue(uint8_t _Pin) { return _max_sensor_values[_Pin]; }

int ReadLightSensor(int analog_CH) {
  int value = 0;
  if(FrontLineColor == 0)
    value = map(ADC_read(_sensorPins[analog_CH]), _min_sensor_values[analog_CH], _max_sensor_values[analog_CH], 0, 100);
  else
    value = map(ADC_read(_sensorPins[analog_CH]), _min_sensor_values[analog_CH], _max_sensor_values[analog_CH], 100, 0);
  return constrain(value, 0, 100);
}

void ReadADCcal(){
  for(int i = 0;i<8;i++){
    Serial.print("  ADC"+String(i)+"=");
    Serial.print(ReadLightSensor(i));
  }
  Serial.println("");
}

void setCalibrate(int cal_round) {
  for (uint8_t i = 0; i < _NumofSensor; i++){
    _max_sensor_values[i] = 0;
    _min_sensor_values[i] = 4095;
  }
  for (int round_count = 0; round_count < cal_round; round_count ++ ) {
    for (uint8_t i = 0; i < _NumofSensor; i++) {
      int v = ADC_read(_sensorPins[i]);
      if (v > _max_sensor_values[i]) _max_sensor_values[i] = min(v, 4095);
    }
    for (uint8_t i = 0; i < _NumofSensor; i++) {
      int v = ADC_read(_sensorPins[i]);
      if (v < _min_sensor_values[i]) _min_sensor_values[i] = max(v, 0);
    }
  }
  Serial.println();
  for(int i=0;i<8;i++){ Serial.print("Min"+String(i)+"="); Serial.print(_min_sensor_values[i]); Serial.print("   "); }
  Serial.println();
  for(int i=0;i<8;i++){ Serial.print("Max"+String(i)+"="); Serial.print(_max_sensor_values[i]); Serial.print("   "); }
}

// ================== Line position / PID ==================
int readline() {
  uint8_t onLine = 0;
  long avg = 0, sum = 0;
  for (uint8_t i = 0; i < _NumofSensor; i++) {
    long value = ReadLightSensor(i);
    if (value > _Sensitive) onLine = 1;
    if (value > 5) { avg += (long)value * (i * 100); sum += value; }
  }
  if (!onLine) {
    return (_lastPosition < (_NumofSensor - 1) * 100 / 2) ? 0 : (_NumofSensor - 1) * 100;
  }
  _lastPosition = (double)avg / (double)sum;
  return (int)_lastPosition;
}

void lineFollow_PID(int RUN_PID_speed , float KP, float KI, float KD) {
  int present_position = readline();
  int setpoint = ((_NumofSensor - 1) * 100) / 2;
  errors     = present_position - setpoint;
  integral  += errors;             // ใช้เมื่อ KI > 0
  derivative = (errors - previous_error);
  integral = constrain(integral, -2000.0, 2000.0); // anti-windup เบา ๆ

  output = (KP * errors) + (KI * integral) + (KD * derivative);
  int motorL = constrain(RUN_PID_speed + (int)output, -100, 100);
  int motorR = constrain(RUN_PID_speed - (int)output, -100, 100);

  motor(1,motorL);
  motor(2,motorR);
  previous_error = errors;
}
