# IoT-Based Smart Sanitation and Automated Odor Control System

This project is an integrated **IoT and Embedded Systems** solution designed to automate sanitation processes and manage indoor air quality (odor control) in public or private restrooms. It combines real-time data tracking via **Firebase** with autonomous hardware triggers.

---

## 🚀 Key Features

* **Autonomous Hardware Control:** Utilizes an Arduino-based system (`sprey_final.ino`) to manage spray mechanisms and sensors.
* **Real-Time Cloud Integration:** Python-driven backend (`kopya.py`) for seamless communication with **Firebase Realtime Database**.
* **Smart Scheduling & Triggers:** Automated odor control based on occupancy or predefined time intervals.
* **Configuration Management:** Uses localized `iot.json` for secure service account management.

---

## 🛠️ Technical Stack

* **Microcontroller:** Arduino (ESP8266 / NodeMCU compatible).
* **Programming Languages:** C++ (Arduino Sketch) and Python 3.x.
* **Cloud Platform:** Firebase Realtime Database.
* **Libraries:** `firebase-admin` (Python), `ESP8266WiFi`, and `FirebaseArduino` (C++).

---

## 📂 Project Structure

```text
├── sprey_final.ino      # Arduino source code for hardware control
├── kopya.py             # Python script for IoT data orchestration
├── iot.json             # Firebase Service Account Credentials (Excluded from Git)
└── .gitignore           # Security rules to prevent sensitive data leaks

## ⚙️ How It Works

The system operates through a synchronized workflow between hardware and cloud services:

* **Detection:** The Arduino system continuously monitors environmental triggers or occupancy status using integrated sensors.
* **Communication:** The Python script acts as a bridge, fetching real-time status updates from or pushing data to the **Firebase** database.
* **Action:** Upon receiving a specific signal—either triggered by local sensors or a remote command from the cloud—the Arduino activates the automated sanitation/spray module.
