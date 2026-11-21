# PB-BLE Examples

ตัวอย่างโค้ด Arduino สำหรับบังคับหุ่นยนต์ด้วยแอป **PB Controller** ผ่าน Bluetooth
โดยใช้ไลบรารี [PB-BLE](https://github.com/Amornthep-ra/PB-BLE) แยกโฟลเดอร์ตามบอร์ด

🌐 **หน้าเว็บรวมคู่มือ:** https://amornthep-ra.github.io/PB-BLE-Example-All/

⬇ **[ดาวน์โหลดโค้ดทั้งหมด (ZIP)](https://github.com/Amornthep-ra/PB-BLE-Example-All/archive/refs/heads/main.zip)**
แตกไฟล์ zip ก่อน แล้วเข้าโฟลเดอร์ของบอร์ดที่ใช้

## บอร์ดที่รองรับ

| บอร์ด | ชิป | โหมดในแอป | คู่มือ | โค้ด |
|---|---|---|---|---|
| PicoMini | ESP32-C3 | 4BTN · 8BTN · Joystick | [เปิดคู่มือ](https://amornthep-ra.github.io/PB-BLE-Example-All/PicoMini/) | [`PicoMini/`](PicoMini) |

## โครงสร้าง

```
PB-BLE-Example-All/
├── index.html          ← หน้าเว็บรวม (GitHub Pages)
└── PicoMini/
    ├── index.html      ← คู่มือของบอร์ด PicoMini
    ├── README.md
    ├── PB_4BTN_Control/
    ├── PB_8BTN_Control/
    └── PB_Joystick_Control/
```

### เพิ่มบอร์ดใหม่

1. สร้างโฟลเดอร์ชื่อบอร์ด เช่น `NewBoard/` แล้วใส่โฟลเดอร์ sketch (ชื่อโฟลเดอร์ต้องตรงกับชื่อไฟล์ `.ino`)
2. ใส่คู่มือของบอร์ดเป็น `NewBoard/index.html` และ `NewBoard/README.md`
3. เพิ่มการ์ดบอร์ดใน `index.html` ที่ root และเพิ่มแถวในตารางด้านบน
