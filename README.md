# Garden Monitoring Station

This project implements a gardening edge-device application for autonomous environment monitoring and control. It features a modular architecture, leveraging a microcontroller for real-time sensor data acquisition and output device management. The system allows for easy scaling and integration with server-side logging for data analysis.

## Key Features
- **Autonomous Irrigation:** Manages watering schedules based on soil moisture levels.
- **Environment Detection:** Monitors critical environmental parameters:
  - Air temperature and humidity
  - Light intensity
  - Soil moisture content
- **Intelligent Output Control:** Manages output devices (e.g., grow lights, water pumps) based on environmental conditions and predefined thresholds.
- **Modular Architecture:** Designed for easy expansion to multiple monitoring stations.
- **Network Connectivity:** Integrates a Wi-Fi module for remote monitoring and data logging (utilizing MQTT for communication).
- **Logging Mechanism:** Collects sensor data for further analysis on a server-side database (external project component).
- **User Interface:** Provides local status display via an OLED screen.

## Hardware Components
### Supported Platforms
Click one of followings to check hardware-wiring diagram :
- [STM32F446RETx microcontroller](./doc/hw/stm32f446-integration.svg)

### Key Peripheral Devices
- **Sensors:**
  - **DHT11:** Measures air temperature and humidity.
  - **LDR03:** Detects light intensity.
  - **Capacitive Soil Moisture Sensor:** Monitors soil moisture levels without corrosion issues.
- **Actuators/Output Devices:**
  - **SRD-05VDC-SL-C Relays:** Control a water pump for irrigation and a grow light bulb.
  - **SSD1315 OLED Display:** Provides local real-time display of system status and sensor readings (connected via SPI).
- **Network Module:**
  - **ESP-12S (ESP8266):** Handles Wi-Fi connectivity, enabling remote communication via UART and supporting MQTT protocol.

## Software Architecture
- **Application Layer:** Contains high-level logic for managing air conditioning (`aircond_track`) and daylight (`daylight_track`), processing sensor data, and controlling output devices.
- **Middleware Layer:** Provides abstraction for system-level services such as task management (leveraging FreeRTOS for multi-tasking operations) and network communication interfaces (e.g., ESP-AT parser).
- **Platform Abstraction Layer:** Offers a hardware-agnostic interface for interacting with the specific embedded system boards and connected sensors/actuators. This includes drivers for GPIO, SPI, and sensor-specific initialization/readout functions.
- **Utilities Layer:** Includes common utility functions like time tracking, string conversions, and general data manipulation.

## Quick Start
To get started with building and running the Garden Monitoring Station application, follow these steps:

#### Clone the MQTT Client Library
This project relies on [external MQTT client C library](https://github.com/metalalive/MQTT_Client/). Clone it to your local file system (if not done yet) and note its path.

#### Build Parameters
**MQC_PROJ_HOME**
The build system needs to know the location of the MQTT client library. Set parameter `MQC_PROJ_HOME` along with make commands that indicates root directory of your cloned `mqtt-client` repository.

```bash
make <target-command> MQC_PROJ_HOME=/path/to/your/mqtt-client/
```

#### Build the Application
Navigate to the root directory of this project and run the build command:
```bash
make build_exe
```
This will compile all necessary components and create application image at `build/garden_monitor_edge.elf`.

#### Run / Debug (Optional)
To run or debug the application on your target hardware, you can use the following commands:
- Start a debugging server connecting to target embedded board: `make dbg_server`
- load built image to target embedded board, control execution state on it through debugging client : `make dbg_client`
- once new image is flashed to your target board, you can restart the execution next time by shutdown then powering up again (without debugging client intervention)

#### Build Helper Documentation
run `make help` for detail

## LICENSE
[MIT](./LICENSE)
