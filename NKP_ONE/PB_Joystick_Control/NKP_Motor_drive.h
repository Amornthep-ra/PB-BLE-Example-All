#pragma once

static inline int nkp_motor_clamp_speed(int speed) {
  if (speed > 100) return 100;
  if (speed < -100) return -100;
  return speed;
}

static inline uint8_t nkp_motor_duty_from_speed(int speed) {
  const int clamped = nkp_motor_clamp_speed(speed);
  const int magnitude = clamped < 0 ? -clamped : clamped;
  return static_cast<uint8_t>((magnitude * 255 + 50) / 100);
}

static inline void nkp_pwm_write(uint8_t pin, uint8_t channel, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, duty);
#else
  ledcWrite(channel, duty);
#endif
}

static inline void motor(int channel, int signedSpeed) {
  const int speed = nkp_motor_clamp_speed(signedSpeed);
  const uint8_t duty = nkp_motor_duty_from_speed(speed);
  const uint8_t inverseDuty = 255 - duty;

  if (channel == 1) {
    if (speed > 0) {
      nkp_pwm_write(4, 7, 255);
      nkp_pwm_write(2, 6, inverseDuty);
    } else if (speed < 0) {
      nkp_pwm_write(4, 7, inverseDuty);
      nkp_pwm_write(2, 6, 255);
    } else {
      nkp_pwm_write(4, 7, 255);
      nkp_pwm_write(2, 6, 255);
    }
  } else if (channel == 2) {
    if (speed > 0) {
      nkp_pwm_write(17, 5, 255);
      nkp_pwm_write(16, 4, inverseDuty);
    } else if (speed < 0) {
      nkp_pwm_write(17, 5, inverseDuty);
      nkp_pwm_write(16, 4, 255);
    } else {
      nkp_pwm_write(17, 5, 255);
      nkp_pwm_write(16, 4, 255);
    }
  }
}

static inline void motor(int channel, int direction, int speed) {
  int magnitude = nkp_motor_clamp_speed(speed);
  if (magnitude < 0) magnitude = -magnitude;

  if (direction == 1) {
    motor(channel, magnitude);
  } else if (direction == 2) {
    motor(channel, -magnitude);
  } else {
    motor(channel, 0);
  }
}

static inline void MT(int speeda, int speedb, int time_speed) {
  motor(1, 1, speeda);
  motor(2, 1, speedb);
  delay(time_speed);
}

static inline void ao() {
  motor(1, 0);
  motor(2, 0);
}

static inline void aot(int timer) {
  ao();
  delay(timer);
}

static inline void fd(int speed) {
  motor(1, 1, speed);
  motor(2, 1, speed);
}

static inline void bk(int speed) {
  motor(1, 2, speed);
  motor(2, 2, speed);
}

static inline void sl(int speed) {
  motor(1, 2, speed);
  motor(2, 1, speed);
}

static inline void sr(int speed) {
  motor(1, 1, speed);
  motor(2, 2, speed);
}
