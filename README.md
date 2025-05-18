# Medibox - Smart Medication Management System

![Screenshot (390)](https://github.com/user-attachments/assets/dc9c8cdb-63f6-445a-a014-f9b3d90d1ec1)


## 📌 Table of Contents
- [Project Overview](#-project-overview)
- [Key Features](#-key-features)
- [Hardware Components](#-hardware-components)
- [Software Requirements](#-software-requirements)
- [Installation Guide](#-installation-guide)
- [Configuration](#-configuration)
- [Usage Instructions](#-usage-instructions)
- [Node-RED Dashboard](#-node-red-dashboard)
- [Project Structure](#-project-structure)
- [Demo Video](#-demo-video)
- [License](#-license)

## 🌟 Project Overview

Medibox is an ESP32-based intelligent medication management system developed for the EN2853: Embedded Systems and Applications course. The system combines:

1. **Core Medication Reminder Functionality** 
2. **Enhanced Environmental Monitoring** 
The system helps users manage medication schedules while ensuring optimal storage conditions for medicines.

## ✨ Key Features

### Assignment 1: Core Functionality
- 🕒 **NTP Time Synchronization**: Automatic time updates from internet servers
- ⏰ **Smart Alarms**: Set up to 2 medication reminders with configurable times
- 🌡️ **Environmental Monitoring**: Real-time temperature and humidity tracking
- 🚨 **Alert System**: Visual (OLED/LED) and auditory (buzzer) notifications
- 🖥️ **OLED Interface**: Intuitive menu system for configuration
- 🔄 **Snooze Function**: 5-minute delay option for alarms

### Assignment 2: Enhanced Features
- ☀️ **Light Sensitivity Protection**: LDR sensor monitors light exposure
- 🎚️ **Smart Shading System**: Servo-controlled window with dynamic adjustment
- 📈 **Data Analytics**: Historical environmental data tracking
- 🌐 **Remote Monitoring**: Node-RED dashboard with real-time visualization
- 🔧 **Customizable Parameters**: Adjustable settings for different medications
- ⚙️ **Adaptive Sampling**: Configurable sensor reading intervals

## 🛠️ Hardware Components

| Component | Quantity | Purpose |
|-----------|----------|---------|
| ESP32 Dev Board | 1 | Main microcontroller |
| OLED Display (128x64) | 1 | User interface |
| DHT11 Sensor | 1 | Temperature/Humidity monitoring |
| LDR Sensor | 1 | Light intensity measurement |
| Micro Servo Motor | 1 | Window shade control |
| Buzzer | 1 | Audible alerts |
| Push Buttons | 3 | User input |
| LEDs | 2 | Status indicators |
| Resistors | As needed | Circuit protection |

## 💻 Software Requirements

- **Arduino IDE** (with ESP32 board support)
- **Required Libraries**:
  - `WiFi`
  - `NTPClient`
  - `DHT sensor library`
  - `Adafruit_SSD1306`
  - `PubSubClient` (for MQTT)
- **Node-RED** 
- ![Screenshot (389)](https://github.com/user-attachments/assets/373d56ce-2343-4891-ac89-f72f52f6fd08)

- **MQTT Broker** (test.mosquitto.org or local)
- ![Screenshot (391)](https://github.com/user-attachments/assets/2d75042a-528f-454a-b360-2b443c01ef8b)


## 📥 Installation Guide

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/Chamindu24/medibox.git
   cd medibox
