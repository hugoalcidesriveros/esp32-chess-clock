
# ♟️ AndesChess | Smart Chess Clock

### Open Hardware · Embedded Systems · ESP32 · PCB Design

An electronic chess clock designed from the ground up, combining embedded firmware, magnetic sensing, custom electronics and mechanical design.

![Status](https://img.shields.io/badge/Status-Active_Development-orange?style=for-the-badge)
![Platform](https://img.shields.io/badge/Platform-ESP32-2C3E50?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C%2B%2B-00599C?style=for-the-badge)
![PCB](https://img.shields.io/badge/PCB-KiCad_9-314CB6?style=for-the-badge)

---

## Overview

**AndesChess Smart Chess Clock** is a custom electronic chess clock built around the ESP32 microcontroller.

The project aims to develop a reliable, portable chess clock with independent OLED displays, precise time management and a magnetic lever-based control system.

Rather than relying on conventional player push buttons, the design explores a mechanical lever mechanism combined with magnetic reed switches.

The development process covers the complete product lifecycle:

- Embedded firmware development
- Electronic circuit design
- Custom PCB development
- Magnetic sensor integration
- Mechanical enclosure design
- Battery-powered operation
- Hardware testing and validation

The long-term goal is to create a compact, practical and reproducible chess clock suitable for everyday play and chess club environments.

---

## Hardware Architecture

| Component | Specification | Purpose |
|---|---|---|
| Microcontroller | ESP32 DevKit V1, 30-pin | Main controller |
| Display 1 | 0.96" OLED, I2C | Player 1 timer |
| Display 2 | 0.96" OLED, I2C | Player 2 timer |
| Sensor 1 | Magnetic reed switch | Player 1 lever |
| Sensor 2 | Magnetic reed switch | Player 2 lever |
| Central control | External push button | Clock control |
| PCB | Custom 2-layer board | Electronic integration |
| Battery (planned) | 18650 Li-ion, 3000 mAh | Portable power |
| Voltage regulator (planned) | 3.3 V buck-boost | Stable power supply |
| Enclosure | Custom mechanical design | Component protection and assembly |

### Magnetic Lever System

The clock uses a lever-based mechanical control concept.

Each player operates a physical lever containing a magnet. The corresponding reed switch detects the magnetic field and provides a digital input to the ESP32.

This architecture eliminates the need for conventional electrical player buttons beneath the levers.

**Expected advantages:**

- Simple electrical sensing
- Minimal mechanical contact wear
- Traditional chess-clock interaction
- Compact electronic integration

The final lever mechanism and magnetic activation distances require physical validation.

---

## System Architecture

```text
                    ANDESCHESS CLOCK
                           |
                    +-------------+
                    |    ESP32    |
                    | Controller  |
                    +------+------+
                           |
             +-------------+-------------+
             |                           |
       +-----+-----+               +-----+-----+
       |  OLED 1   |               |  OLED 2   |
       | Player 1  |               | Player 2  |
       +-----------+               +-----------+

                    INPUT SYSTEM
                          |
             +------------+------------+
             |            |            |
          Reed SW1      Reed SW2    Central Button
             |            |            |
          Player 1      Player 2     Clock Control
             |            |
          Magnet 1      Magnet 2
             |            |
           Lever 1       Lever 2


                 POWER SYSTEM (PLANNED)
                          |
                   Li-ion 18650
                          |
               Charging & Protection
                          |
                  3.3 V Buck-Boost
                          |
                    ESP32 + OLEDs
```

The architecture is under active development and may change as hardware validation progresses.

---

## Custom PCB Design

The electronic board is being developed using **KiCad 9.0**.

A custom ESP32 footprint and reed switch footprint have been created for the project.

### PCB Specifications

| Parameter | Design |
|---|---|
| Board dimensions | 150 × 65 mm |
| Layers | 2 copper layers |
| Top copper | F.Cu |
| Bottom copper | B.Cu |
| Signal trace width | 0.30 mm |
| Ground distribution | GND copper zones |
| Mounting | Mechanical mounting holes |
| Design software | KiCad 9.0 |

### Component Placement

The PCB is organized around a centrally mounted ESP32.

- **Center:** ESP32 DevKit
- **Left:** Player 1 reed switch
- **Right:** Player 2 reed switch
- **Upper area:** OLED connectors
- **Lower area:** Central button connector

The reed switches are positioned approximately 120 mm apart, measured between their footprint centers.

The OLED displays and central control button are mounted separately from the main PCB and connected using cables.

The PCB layout, footprints and mechanical clearances are still undergoing verification.

---

## ESP32 Pin Configuration

Current prototype signal assignments:

| Function | ESP32 GPIO |
|---|---|
| Player 1 reed switch | GPIO25 |
| Player 2 reed switch | GPIO26 |
| Central control button | GPIO27 |
| OLED communication | I2C |

The OLED connections use SDA, SCL, VCC and GND.

Final GPIO assignments, I2C configuration and hardware connections should be verified against the firmware and latest schematic.

---

## Firmware Development

The firmware is developed in **C++**, using the Arduino framework with PlatformIO.

### Development Environment

| Tool | Purpose |
|---|---|
| Visual Studio Code | Development IDE |
| PlatformIO | Build and upload environment |
| Arduino Framework | ESP32 firmware framework |
| C++ | Main programming language |
| Serial Monitor | Debugging and diagnostics |

### Current Prototype Progress

- [x] ESP32 development environment configured
- [x] Initial OLED display tested
- [x] Basic countdown timer implemented
- [x] Physical button inputs tested
- [x] Player turn switching tested
- [ ] Complete dual OLED integration
- [ ] Integrate magnetic reed switches
- [ ] Implement configurable time controls
- [ ] Implement increment support
- [ ] Validate timer accuracy and reliability
- [ ] Implement complete clock control logic

Some functions are currently validated on the development prototype and are not yet integrated into the final PCB.

---

## Portable Power System

A rechargeable power system is being evaluated for future hardware revisions.

### Proposed Architecture

```text
      18650 Li-ion Battery
              |
      Charging & Protection
              |
        Power Management
              |
      Buck-Boost Regulator
              |
          Stable 3.3 V
              |
        ESP32 + Displays
```

The design is intended to use a single 18650 lithium-ion cell.

A custom 3.3 V buck-boost regulator is being investigated to provide a stable supply throughout the battery discharge range.

Charging, protection, power-path management and safe operation alongside the ESP32 USB connection must be validated before integration.

**Status:** Power electronics research and circuit design.

---

## Mechanical Design

The clock enclosure is being designed as a custom mechanical assembly.

### Design Objectives

- Compact form factor
- Rear USB accessibility
- Two elevated OLED displays
- Magnetic player levers
- Internal PCB mounting
- Protected electronic components
- Accessible central controls
- Future rechargeable battery compartment

The mechanical development process includes CAD modeling, dimensional verification and component fit testing.

The enclosure and PCB will be developed together to ensure correct alignment between magnets and reed switches.

---

## Development Roadmap

### Phase 1 — Embedded Prototype

- [x] Configure ESP32 environment
- [x] Test physical input controls
- [x] Implement basic timer functionality
- [x] Validate player switching
- [x] Test initial OLED display

### Phase 2 — Electronics

- [x] Create custom ESP32 schematic symbol
- [x] Design custom ESP32 footprint
- [x] Design reed switch footprint
- [x] Create electronic schematic
- [x] Perform initial electrical rule checks
- [x] Define PCB dimensions
- [x] Place main PCB components
- [ ] Complete PCB routing
- [ ] Validate PCB design rules
- [ ] Verify physical component footprints
- [ ] Prepare manufacturing files

### Phase 3 — Power Management

- [ ] Select battery charging architecture
- [ ] Design custom 3.3 V regulator
- [ ] Build and test regulator prototype
- [ ] Integrate battery protection
- [ ] Validate power consumption
- [ ] Integrate power system into the main PCB

### Phase 4 — Mechanical Integration

- [ ] Finalize enclosure CAD
- [ ] Manufacture prototype enclosure
- [ ] Integrate magnetic lever mechanism
- [ ] Verify sensor activation distances
- [ ] Assemble complete hardware prototype

### Phase 5 — Validation

- [ ] Complete dual-display integration
- [ ] Test clock timing accuracy
- [ ] Test battery autonomy
- [ ] Validate switch reliability
- [ ] Perform extended operation tests
- [ ] Finalize technical documentation

---

## Repository Organization

The following structure is the proposed organization as development progresses:

```text
esp32-chess-clock/
|
+-- firmware/             # ESP32 source code
+-- hardware/
|   +-- schematics/       # Electronic schematics
|   +-- pcb/              # KiCad PCB design
|   +-- footprints/       # Custom footprints
|   +-- power/            # Power system development
|
+-- mechanical/           # Enclosure and CAD files
+-- docs/                 # Technical documentation
+-- images/               # Project images
|
+-- README.md
```

Repository directories and documentation will be updated as project files are published.

---

## Engineering Approach

This project follows an iterative hardware and firmware development process:

**Design → Prototype → Test → Validate → Improve**

The focus is not only on building a working chess clock, but also on developing a documented and maintainable embedded system.

Special attention is given to:

- Signal integrity
- Electrical safety
- PCB manufacturability
- Mechanical integration
- Firmware reliability
- Power efficiency
- Reproducible hardware development

---

## Project Status

**Active Development — PCB Design & Hardware Integration**

The embedded prototype has demonstrated basic timer operation and player switching.

The current development stage focuses on PCB integration, magnetic sensing, mechanical design and portable power architecture.

This README describes the ongoing engineering process. Features and hardware specifications may change before the first complete production-ready revision.

---

## Technology Stack

![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=flat-square&logo=espressif&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![PlatformIO](https://img.shields.io/badge/PlatformIO-F5822A?style=flat-square&logo=platformio&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00878F?style=flat-square&logo=arduino&logoColor=white)
![KiCad](https://img.shields.io/badge/KiCad-314CB6?style=flat-square&logo=kicad&logoColor=white)
![VS Code](https://img.shields.io/badge/VS_Code-007ACC?style=flat-square&logo=visualstudiocode&logoColor=white)

---

### ♟️ AndesChess — Hardware & Embedded Development

*Engineering a modern chess clock, one iteration at a time.*
