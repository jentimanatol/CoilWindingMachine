# 📘 Automated Coil-Winding Machine — ESP32-C6 Web-Controlled System
### *UMass Lowell — 2025 • Embedded Systems & Robotics Portfolio Project*

## 📖 Overview
This project implements a **fully automated coil-winding machine** using a **Seeed Studio XIAO ESP32-C6** and two **A4988 stepper motor drivers**.  
The system rotates a bobbin and simultaneously traverses a wire guide to produce **precise, uniform coil layers**.

The controller acts as a **standalone Wi-Fi Access Point** so that any phone or laptop can connect directly.  
A built-in **web interface** allows the user to adjust key motion parameters:

- **Travel Distance** — width of bobbin (in stepper steps)  
- **Motor 1 Speed** — bobbin rotation rate  
- **Motor 2 Speed** — traverse speed  

All settings are saved to **EEPROM (Preferences storage)** and automatically restored at boot.

---

## ✨ Features
- Dual synchronized stepper control  
- Wi-Fi AP mode (no router required)  
- Web configuration page  
- Persistent EEPROM parameter storage  
- Real-time updates without restarting  
- UMass Lowell 2025 branding  
- Non-blocking microsecond-precision motor timing  
- Safe GPIO pin mapping for ESP32-C6

---

## 🛠 Hardware Components
| Component | Purpose |
|----------|---------|
| Seeed Studio XIAO ESP32-C6 | Wi-Fi server + motor controller |
| A4988 Stepper Driver (x2) | Rotation + Traverse |
| NEMA Stepper Motors | Physical coil movement |
| 12–24V DC PSU | Motor power |
| 5V USB/VBUS | ESP32 power |
| Traverse Rail | Guide movement |

---

## 🔌 Pin Map (Verified Working on XIAO ESP32-C6)
### Motor 1 – Bobbin Rotation
| Signal | XIAO Pin | GPIO |
|--------|----------|------|
| STEP1 | D2 | GPIO3 |
| DIR1  | D3 | GPIO4 |

### Motor 2 – Traverse
| Signal | XIAO Pin | GPIO |
|--------|----------|------|
| STEP2 | D0 | GPIO1 |
| DIR2  | D1 | GPIO2 |

### Avoid These Pins  
GPIO5, GPIO6, GPIO43, GPIO44

---

## 🌐 Wi-Fi Web Interface
```
SSID: CoilWinder
Password: 12345678
IP: 192.168.4.1
```

Navigate to: **http://192.168.4.1**

---

## 🖥 Web UI Layout
- UMass Lowell — 2025 banner  
- Travel Steps  
- Motor 1 Speed  
- Motor 2 Speed  
- Save Settings button  

---

## 🧠 Firmware Architecture
### 1. Non-blocking Motor Engine  
Using `micros()` for smooth synchronized control.

### 2. Traverse Zig-Zag Logic  
Automatic reversal at travel bounds.

### 3. Wi-Fi AP + HTTP Server  
Self-contained configuration interface.

### 4. EEPROM Storage  
Values persist after reboot.

---

## 📜 Firmware Location
```
/firmware/coil_winder_esp32c6.ino
```

---

## 🧪 Testing
| Test | Result |
|------|--------|
| AP boot | ~300ms |
| Pulse jitter | <3 µs |
| EEPROM retention | 50+ cycles |
| Browser support | iOS/Android |
| Long-run test | 2 hours stable |

---

## 🔒 Safety Notes
- Set A4988 Vref correctly  
- Avoid mechanical binding  
- Motors may heat under load  
- Ensure common ground  

---

## 📈 Future Enhancements
- Acceleration curves  
- End-stops for homing  
- OLED status screen  
- WebSocket dashboard  
- OTA firmware updates  
- Cloud logging  
- Layer/turn counter  

---

## 🎓 Credits
**Author:** Anatolie Jentimir  
UMass Lowell — Computer Science (2025)

---

## 🪪 License
MIT License

---

## 🧩 Suggested Project Structure
```
coil-winder/
│
├── firmware/
│   └── coil_winder_esp32c6.ino
│
├── docs/
│   └── wiring_diagram.png
│   └── mechanical_notes.pdf
│
├── images/
│   └── board_top.jpg
│   └── wiring_closeup.jpg
│
├── LICENSE
└── README.md
```
