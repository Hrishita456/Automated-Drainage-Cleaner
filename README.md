# Automated Drainage Cleaner

An embedded and computer-vision based system for automated drainage cleaning, developed as part of the **TA212 Course Project** under **Prof. Sarvesh Mishra, IIT Kanpur**.

The project combines an **Arduino-controlled mechanical cleaning setup** with a **webcam-based waste detection pipeline**. The objective is to detect floating waste in a drainage channel and trigger a cleaning mechanism automatically for collection and removal.

---

## Project Overview

Drainage channels often accumulate floating waste such as plastic wrappers, bottles, leaves, and other debris. Manual cleaning is repetitive, unsafe, and inefficient in many conditions. This project aims to automate that process through a low-cost hardware-software system.

The system consists of:

- an **Arduino-based control unit**
- **DC motors and motor drivers** for actuation
- a **chain-driven conveyor mechanism** for lifting waste
- a **Bluetooth interface** for manual control
- a **webcam-based computer vision module** for waste detection and monitoring

When waste is detected in the designated region of interest, the vision module sends a command to the Arduino, which activates the cleaning and conveyor motors for a fixed cleaning cycle.

---

## Features

- Automated drainage waste detection using computer vision
- Real-time motor actuation using Arduino
- Conveyor-based lifting and collection of floating waste
- Bluetooth/manual override support
- Serial communication between CV pipeline and Arduino
- Modular hardware-software design
- Low-cost prototype using readily available components

---

## Tech Stack

### Embedded / Hardware
- Arduino UNO / Nano
- DC Motors
- L298N motor driver
- HC-05 Bluetooth module
- Mild steel and GI sheet fabricated structure
- Chain-driven conveyor mechanism

### Software
- Arduino IDE (C/C++)
- Python 3
- OpenCV
- PySerial
- NumPy

---

## Repository Structure

```text
automated-drainage-cleaner/
│
├── arduino/
│   └── drainage_cleaner.ino
│
├── cv/
│   └── waste_detection_serial.py
│
└── README.md
