# ♟️ ESP32 Chess Clock

### Electronic Chess Clock | ESP32 · OLED · Embedded Systems

![Status](https://img.shields.io/badge/Status-In%20Development-orange)
![ESP32](https://img.shields.io/badge/ESP32-Microcontroller-blue)
![C++](https://img.shields.io/badge/Language-C%2B%2B-00599C)
![Hardware](https://img.shields.io/badge/Hardware-IoT-green)

## 📖 Overview

**ESP32 Chess Clock** is an open-source electronic chess clock project designed around the ESP32 microcontroller.

The goal is to build a compact, functional chess clock with independent player displays, accurate time management and an intuitive physical interface.

The project combines embedded programming, electronics, sensor integration and hardware prototyping.

## ⚙️ Hardware

The proposed hardware architecture includes:

| Component | Function |
|---|---|
| ESP32 | Main microcontroller |
| 2 × OLED 0.96" | Independent player time displays |
| Reed switches | Magnetic input detection |
| Magnets | Activation of magnetic switches |
| Push buttons | Clock controls |
| Power supply | System power |

Additional components may be introduced as the prototype evolves.

## 🎯 Project Goals

- Display each player's remaining time independently.
- Implement reliable countdown timers.
- Support turn switching between players.
- Integrate magnetic sensors into the control system.
- Provide a compact and practical hardware design.
- Explore configurable time controls and increments.

## 🏗️ System Architecture

```text
             ┌─────────────────┐
             │      ESP32      │
             │   Controller    │
             └────────┬────────┘
                      │
         ┌────────────┴────────────┐
         │                         │
    ┌────▼─────┐              ┌────▼─────┐
    │  OLED 1  │              │  OLED 2  │
    │ Player 1 │              │ Player 2 │
    └──────────┘              └──────────┘

       Magnetic Sensors / Buttons
                      │
                      ▼
              Time Management
```

*Conceptual architecture. Hardware connections and control logic will be documented during development.*

## 💻 Software

**Main language:** C++

**Platform:** ESP32

The firmware is planned to handle display updates, player input, time calculation and clock state management.

## 🚧 Development Roadmap

- [ ] Define hardware specifications.
- [ ] Connect and test both OLED displays.
- [ ] Implement the countdown timer.
- [ ] Develop player turn-switching logic.
- [ ] Integrate Reed switches.
- [ ] Add configurable time controls.
- [ ] Build the physical enclosure.
- [ ] Test accuracy and reliability.
- [ ] Publish circuit diagrams and firmware.

## 📂 Repository Structure

Planned organization:

```text
esp32-chess-clock/
├── firmware/
├── hardware/
├── docs/
├── images/
└── README.md
```

## 🔬 Project Status

**In development — hardware prototyping and firmware design.**

This repository will document the development process, including hardware iterations, technical decisions, source code and testing.

## 👨‍💻 Technologies

ESP32 · C++ · OLED Displays · Embedded Systems · Electronics · IoT

---

**Built with a focus on chess, electronics and embedded software development.**
