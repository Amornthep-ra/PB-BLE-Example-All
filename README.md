# PB-BLE Examples

ตัวอย่างโค้ด Arduino สำหรับบังคับหุ่นยนต์ด้วยแอป **PB Controller** ผ่าน Bluetooth
โดยใช้ไลบรารี [PB-BLE](https://github.com/Amornthep-ra/PB-BLE) แยกโฟลเดอร์ตามบอร์ด

🌐 **หน้าเว็บรวมคู่มือ:** https://amornthep-ra.github.io/PB-BLE-Example-All/

## ⬇ ดาวน์โหลด

| ไฟล์ | ลิงก์ | วิธีใช้ |
|---|---|---|
| **Lib PB-BLE** | [PB-BLE.zip](https://github.com/Amornthep-ra/PB-BLE/releases/latest/download/PB-BLE.zip) | **ไม่ต้องแตกไฟล์** ใน Arduino IDE ไปที่ **Sketch › Include Library › Add .ZIP Library…** แล้วเลือกไฟล์นี้ |
| **โค้ดตัวอย่างทั้งหมด** | [PB-BLE-Example-All.zip](https://github.com/Amornthep-ra/PB-BLE-Example-All/archive/refs/heads/main.zip) | **แตกไฟล์ก่อน** แล้วเข้าโฟลเดอร์ของบอร์ดที่ใช้ |

ต้องใช้ Arduino IDE 2.x และบอร์ด **esp32 by Espressif Systems เวอร์ชัน 3.3.11**
(ห้ามใช้ 4.x เพราะ compile ไม่ผ่าน) · หน้า Lib: [github.com/Amornthep-ra/PB-BLE](https://github.com/Amornthep-ra/PB-BLE)

## บอร์ดที่รองรับ

| บอร์ด | ชิป | ขับเคลื่อน | โหมดในแอป | คู่มือ | โค้ด |
|---|---|---|---|---|---|
| PicoMini | ESP32-C3 | 2WD | 4BTN · 8BTN · Joystick | [เปิดคู่มือ](https://amornthep-ra.github.io/PB-BLE-Example-All/PicoMini/) | [`PicoMini/`](PicoMini) |
| NKP_ONE | ESP32 | 2WD | 4BTN · 8BTN · Joystick | [เปิดคู่มือ](https://amornthep-ra.github.io/PB-BLE-Example-All/NKP_ONE/) | [`NKP_ONE/`](NKP_ONE) |
| NKP_V2 | ESP32 | 2WD / 4WD (`DRIVE_TYPE`) | 4BTN · 8BTN · Joystick | [เปิดคู่มือ](https://amornthep-ra.github.io/PB-BLE-Example-All/NKP_V2/) | [`NKP_V2/`](NKP_V2) |
| ESPRobotV2 | ESP32 | 2WD / 4WD (`DRIVE_TYPE`) | 4BTN · 8BTN · Joystick | [เปิดคู่มือ](https://amornthep-ra.github.io/PB-BLE-Example-All/ESPRobotV2/) | [`ESPRobotV2/`](ESPRobotV2) |
| **บอร์ดของคุณเอง** | ESP32 / ESP32-C3 | 2WD / 4WD | 4BTN · 8BTN · Joystick | [เปิดคู่มือ](https://amornthep-ra.github.io/PB-BLE-Example-All/Custom_Board/) | [`Custom_Board/`](Custom_Board) |

## 🛠 ใช้กับบอร์ดของตัวเอง

ถ้าบอร์ดของคุณไม่อยู่ในตาราง แต่ใช้ชิป **ESP32 หรือ ESP32-C3** ให้ใช้โฟลเดอร์ [`Custom_Board/`](Custom_Board)
แล้ว**แก้แค่ไฟล์ `PB_Board.h`** ที่อยู่ข้างไฟล์ `.ino`:

1. เลือกชนิดไดรเวอร์มอเตอร์ `MOTOR_DRIVER`
   - `1` IN1/IN2 (DRV8833, L9110S, MX1508)
   - `2` DIR + PWM
   - `3` IN1/IN2 + EN (L298N, TB6612FNG)
2. ใส่หมายเลข GPIO ใน `LEFT_PINS` / `RIGHT_PINS`
3. 4WD: ตั้ง `DRIVE_TYPE 4` แล้วใส่ `LEFT2_PINS` / `RIGHT2_PINS`
4. ล้อหมุนกลับทิศ: ตั้ง `INVERT_LEFT` / `INVERT_RIGHT` เป็น `true`

📖 คู่มือละเอียดพร้อมตัวอย่างการต่อขา: https://amornthep-ra.github.io/PB-BLE-Example-All/Custom_Board/

## โครงสร้าง

```
PB-BLE-Example-All/
├── index.html          ← หน้าเว็บรวม (GitHub Pages)
├── PicoMini/
│   ├── index.html      ← คู่มือของบอร์ด PicoMini
│   ├── README.md
│   ├── PB_4BTN_Control/
│   ├── PB_8BTN_Control/
│   └── PB_Joystick_Control/
├── NKP_ONE/            ← โครงสร้างเดียวกับ PicoMini (ใช้ NKP_Motor_drive.h)
├── NKP_V2/             ← โครงสร้างเดียวกับ PicoMini (ใช้ NKP_V2Motor_drive.h แบบ I2C)
├── ESPRobotV2/         ← โครงสร้างเดียวกับ PicoMini (ใช้ NKP_V2Motor_drive.h ฉบับ ESPRobotV2 แบบ PWM)
└── Custom_Board/       ← บอร์ดของคุณเอง (แก้ PB_Board.h)
```

### เพิ่มบอร์ดใหม่ (สำหรับผู้ดูแล repo)

1. สร้างโฟลเดอร์ชื่อบอร์ด เช่น `NewBoard/` แล้วใส่โฟลเดอร์ sketch (ชื่อโฟลเดอร์ต้องตรงกับชื่อไฟล์ `.ino`)
2. ใส่คู่มือของบอร์ดเป็น `NewBoard/index.html` และ `NewBoard/README.md`
3. เพิ่มการ์ดบอร์ดใน `index.html` ที่ root และเพิ่มแถวในตารางด้านบน
