# ESPRobotV2 (ESP32)

[← กลับไปหน้ารวมทุกบอร์ด](../README.md)

ตัวอย่างโค้ด Arduino สำหรับบังคับหุ่นยนต์ **ESPRobotV2 (ESP32)** ด้วยแอป **PB Controller**
ผ่าน Bluetooth โดยใช้ไลบรารี [PB-BLE](https://github.com/Amornthep-ra/PB-BLE)

## 📖 คู่มือติดตั้งและใช้งาน

**https://amornthep-ra.github.io/PB-BLE-Example-All/ESPRobotV2/**

คู่มืออธิบายตั้งแต่ติดตั้ง Arduino IDE, บอร์ด ESP32, ไลบรารี PB-BLE, การปรับความเร็ว ไปจนถึงการอัปโหลดโค้ด
(หรือดาวน์โหลด [`index.html`](index.html) ไปเปิดในเครื่องได้เลย ใช้งานแบบไม่ต่อเน็ตได้)

## ⬇ ดาวน์โหลดโค้ด

**[ดาวน์โหลด ZIP](https://github.com/Amornthep-ra/PB-BLE-Example-All/archive/refs/heads/main.zip)**
หรือกดปุ่ม **Code › Download ZIP** ที่หน้าแรกของ repo แล้ว **แตกไฟล์ zip ก่อน** จากนั้นเข้าโฟลเดอร์ `ESPRobotV2` แล้วค่อยเปิดไฟล์ `.ino`

## โปรแกรมในชุดนี้

| โฟลเดอร์ | โหมดในแอป | การบังคับ |
|---|---|---|
| [`PB_4BTN_Control`](PB_4BTN_Control) | 4BTN | ปุ่มลูกศร + ปุ่มระดับความเร็ว Lo / Med / Hi |
| [`PB_8BTN_Control`](PB_8BTN_Control) | 8BTN | ปุ่มลูกศร 8 ทิศ ปรับความเร็วจากแถบ DRV / TRN ในแอป |
| [`PB_Joystick_Control`](PB_Joystick_Control) | Joystick | จอย เลือกได้ 6 แบบด้วย `JOY_MODE` (Arcade, Tank, Car ฯลฯ) |

แต่ละโฟลเดอร์มี `NKP_V2Motor_drive.h` (ไฟล์สั่งมอเตอร์ของบอร์ด) อยู่ด้วย ห้ามลบ และต้องอยู่โฟลเดอร์เดียวกับไฟล์ `.ino`

มอเตอร์ 4 ช่อง (M1–M4) สั่งด้วย PWM โดยตรง รองรับทั้ง 2WD และ 4WD
เลือกได้ด้วยบรรทัดนี้ในไฟล์ `.ino`:

```cpp
#define DRIVE_TYPE  2   // 2 = 2WD (M1 ซ้าย, M2 ขวา) | 4 = 4WD (M1 + M3 ซ้าย, M2 + M4 ขวา)
```

## สิ่งที่ต้องใช้

- Arduino IDE 2.x
- **esp32 by Espressif Systems เวอร์ชัน 3.3.11** (ห้ามใช้ 4.x เพราะ compile ไม่ผ่าน)
- ไลบรารี PB-BLE: [ดาวน์โหลด PB-BLE.zip](https://github.com/Amornthep-ra/PB-BLE/releases/latest/download/PB-BLE.zip) แล้วติดตั้งด้วย **Sketch › Include Library › Add .ZIP Library…** (ไม่ต้องแตกไฟล์)
- บอร์ด: **ESP32 Dev Module**
- Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)**

## ก่อนอัปโหลด

แก้ 3 บรรทัดนี้ด้านบนของไฟล์ `.ino` ให้ไม่ซ้ำกับบอร์ดอื่น:

```cpp
const char *BLE_NAME  = "PB-ESPRobotV201"; // ชื่อที่เห็นในแอป
const char *BOARD_ID  = "ESPRobotV201";    // ภาษาอังกฤษ/ตัวเลข 1–32 ตัว
const char *PAIR_CODE = "12345678";     // ภาษาอังกฤษ/ตัวเลข 8–16 ตัว
```

> ⚠️ ยกล้อหุ่นให้ลอยก่อนทดสอบครั้งแรก
