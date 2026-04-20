# Farm Monitor Station -- Hardware Integration Guide

This guide is designed to help you assemble, and integrate the hardware components of this gardening station.

---

## System Overview
The Farm Monitor Station is divided into two main sections:
1.  [The Main Logic Board](./stm32f446-integration.svg) : The "Brain" that processes data from sensors and decides when to turn on the water or fans.
2.  [The Power Distribution Board](./station-power-distribution-compact.svg) : The "Power Plant" that takes raw electricity (12V) and converts it into the steady, safe voltages (5V and 3.3V) required by the sensitive electronics.

---

## The "Brain" (STM32 & ESP8266)
At the center of the project is the **STM32F446** microcontroller. Think of this as the conductor of an orchestra.

* **STM32 (The Conductor):** It reads information from the soil and air, manages the display, and controls the switches.
* **ESP-12F (The Messenger):** This module adds Wi-Fi capabilities, which allows the STM32 to send your garden's data to the internet so you can check on your plants from your phone.
* **HC-SR04 (Ultrasonic Sensor):** In this project it acts as random source for crytography and secure network connection .

---

## The "Senses" (Sensors)
The station uses several sensors to detect the environment:

* [Capacitive Soil Moisture Sensors](./cap-soil-moisture-v2.0-compact.svg) : These measure how much water is in the soil. 
    * *Technical Note:* We use an **Op-Amp (Operational Amplifier)** as a "buffer" for these sensors. Because the sensors have high "impedance" (they are weak at pushing their signal), the Op-Amp acts like a megaphone, making the signal strong enough for the Brain to read accurately.
* **LDR (Light Dependent Resistors):** These sense how much sunlight your plants are getting.
* **DHT11:** A combined sensor that measures the Temperature and Humidity of the air.

---

## The "Hands" (Relays)
To actually do work—like turning on a pump or a light—the station uses **Relays**.
* **What is a Relay?** Think of it as a bridge. The "Brain" uses a tiny amount of electricity to trigger a mechanical switch inside the relay. This allows a small, safe signal to control a large, powerful device like a water pump or a cooling fan.

---

## The "Power Plant" (Power Distribution)
The system is designed to run on a **12V 2A power supply** (similar to a laptop or router plug). However, the "Brain" and "Sensors" cannot handle 12V directly.

* **LM2596 (The Buck Converter):** This efficiently "steps down" the 12V to 5V. 5V is the standard power for the relays and the ultrasonic sensor.
* **AMS1117 (The LDO Regulators):** These further refine the power down to 3.3V. This is the "cleanest" power, used for the Brain (STM32) and the Wi-Fi module.
* **Power Switching:** The system is smart enough to turn off parts of itself when they aren't needed to save energy. It uses **MOSFETs** (electronic high-speed switches) to cut power to specific sensor groups.

---

## Assembly Suggestion

### Step 1: Prepare the Power Board
1.  Connect your 12V DC input to the Power Distribution Board.
2.  Verify with a multimeter that you are getting exactly 5V and 3.3V at the designated output pins.
3.  **Warning:** Do not connect the Main Board until you have confirmed the voltages are correct, or you may damage the microcontroller.

### Step 2: Connect the Sensors
1.  Plug the **Soil Moisture Sensors** into the buffered inputs (U11, U9, U10, U21).
2.  Mount the **DHT11** in a spot with good airflow but away from direct water spray.
3.  Position the **LDRs** where they can clearly see the sky/grow lights.

### Step 3: Wire the Relays
1.  Connect your Water Pump to the "Pump-switch" relay.
2.  Connect your Cooling Fan to the "Fan-switch" relay.
3.  **Safety Tip:** If you are switching high-voltage (110V/220V) appliances, ensure all wires are insulated and the board is mounted in a non-conductive plastic enclosure.

---

## Simple Glossary of Terms

| Term | Meaning in This Project |
| :--- | :--- |
| **ADC** | (Analog-to-Digital Converter) The part of the Brain that turns a sensor's voltage into a number. |
| **Buck Converter** | A device that lowers voltage efficiently without getting too hot. |
| **LDO** | (Low-Dropout Regulator) A device that "smooths out" electricity to provide a very steady voltage. |
| **Impedance** | A measure of how much a circuit resists the flow of electricity. High impedance signals are "weak" and need a buffer (Op-Amp). |
| **Solder Bridge (SB)** | Small pads on the circuit board that can be connected with a drop of solder to change how the board behaves. |

---
*Manual Version 0.1 | April 2026*
