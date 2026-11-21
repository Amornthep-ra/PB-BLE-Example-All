# บอร์ดของคุณเอง (Custom Board · ESP32 / ESP32-C3)

[← กลับไปหน้ารวมทุกบอร์ด](../README.md)

ใช้โฟลเดอร์นี้ถ้าบอร์ดของคุณไม่อยู่ในรายการ แต่ใช้ชิป **ESP32** หรือ **ESP32-C3**

📖 **คู่มือละเอียด:** https://amornthep-ra.github.io/PB-BLE-Example-All/Custom_Board/

## ดาวน์โหลด

- Lib PB-BLE: [PB-BLE.zip](https://github.com/Amornthep-ra/PB-BLE/releases/latest/download/PB-BLE.zip) (ติดตั้งด้วย **Sketch › Include Library › Add .ZIP Library…** ไม่ต้องแตกไฟล์)
- โค้ดตัวอย่าง: [PB-BLE-Example-All.zip](https://github.com/Amornthep-ra/PB-BLE-Example-All/archive/refs/heads/main.zip) (แตกไฟล์ แล้วเข้าโฟลเดอร์ `Custom_Board`)
- ใช้ esp32 core **3.3.11** (ห้ามใช้ 4.x)

## ไฟล์ที่ต้องใช้

| โฟลเดอร์ | โหมดในแอป |
|---|---|
| [`PB_4BTN_Control`](PB_4BTN_Control) | 4BTN (Lo / Med / Hi) |
| [`PB_8BTN_Control`](PB_8BTN_Control) | 8BTN (DRV / TRN) |
| [`PB_Joystick_Control`](PB_Joystick_Control) | Joystick (6 แบบ) |

แต่ละโฟลเดอร์มี `.ino` และ **`PB_Board.h`** ไฟล์ที่ต้องแก้ให้ตรงกับบอร์ดคือ **`PB_Board.h`** เท่านั้น
(ส่วนชื่อ BLE / Board ID / Pair Code / ความเร็ว แก้ใน `.ino` เหมือนบอร์ดอื่น)

## ตั้งค่า PB_Board.h

```cpp
#define MOTOR_DRIVER  DRIVER_IN1_IN2   // 1 = IN1/IN2, 2 = DIR+PWM, 3 = IN1/IN2+EN

const int LEFT_PINS[3]   = { -1, -1, -1 };   // ใส่เลข GPIO ของมอเตอร์ซ้าย
const int RIGHT_PINS[3]  = { -1, -1, -1 };   // ใส่เลข GPIO ของมอเตอร์ขวา

#define DRIVE_TYPE  2                        // 2 = 2WD, 4 = 4WD (+ LEFT2_PINS / RIGHT2_PINS)
const int DRIVER_ENABLE_PIN = -1;            // STBY / Enable (ถ้ามี)
const bool INVERT_LEFT  = false;             // ล้อซ้ายกลับทิศ
const bool INVERT_RIGHT = false;             // ล้อขวากลับทิศ
```

| MOTOR_DRIVER | ขาต่อมอเตอร์ 1 ตัว | ตัวอย่างชิป |
|---|---|---|
| `1` `DRIVER_IN1_IN2` | `{ IN1, IN2, -1 }` (PWM ทั้งสองขา) | DRV8833, L9110S, MX1508 |
| `2` `DRIVER_DIR_PWM` | `{ DIR, PWM, -1 }` | DRV8835 (PH/EN) |
| `3` `DRIVER_IN1_IN2_EN` | `{ IN1, IN2, EN }` | L298N, TB6612FNG |

ถ้ายังไม่ใส่ขา โค้ดจะไม่สั่งมอเตอร์ และ Serial Monitor จะขึ้นคำเตือน `motor pins are not set`

> ⚠️ ยกล้อหุ่นให้ลอยก่อนทดสอบครั้งแรก
