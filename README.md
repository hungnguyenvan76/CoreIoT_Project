# CoreIoT Edge AI & Automation System

**NOTE: The most complete and fully implemented version of this project, including all advanced optimizations and codebase improvements, is located in the `advanced_version` branch.**


## Overview
CoreIoT Edge AI & Automation System is a comprehensive, multi-node Internet of Things (IoT) project designed for real-time environmental monitoring, local AI inference, and cloud-based automated execution. Built on the ESP32 platform, this system seamlessly integrates edge computing (TinyML) with a cloud-based rules engine (ThingsBoard) to provide low-latency anomaly detection and automated environmental risk management.

This project was developed to demonstrate advanced embedded systems programming, including RTOS-based multithreading, time-series data processing, local networking via Captive Portals, and bidirectional cloud communication.

## Key Features
- Distributed Multi-Node Architecture: Utilizes a single codebase compiled into different operational nodes (Telemetry Node and Actuator Node) using PlatformIO build flags.
- Edge AI Inference (TinyML): Implements TensorFlow Lite Micro on the ESP32 to run a Neural Network model. The system uses a sliding-window algorithm on time-series temperature and humidity data to predict anomalies (Fire Risk, Mold Risk, HVAC states) locally with minimal latency.
- Real-Time Operating System (FreeRTOS): Leverages FreeRTOS for efficient multitasking. Thread-safe communication between sensor readings, AI inference, and network transmission is handled via Queues and Semaphores.
- Local Captive Portal & WebSockets: Features an asynchronous WebServer that acts as a Captive Portal for dynamic Wi-Fi and Cloud credential provisioning. Includes a WebSocket-based Dashboard for real-time local monitoring and manual override controls.
- Cloud Integration & RPC (ThingsBoard/CoreIoT): Communicates with a ThingsBoard-based cloud platform via MQTT. Supports telemetry publishing, Shared Attributes for state recovery, and Remote Procedure Calls (RPC) for cloud-to-device actuation commands.

## System Architecture

### Software Stack
- Microcontroller Framework: Arduino Core for ESP32
- Build System: PlatformIO
- Multithreading: FreeRTOS
- Machine Learning: TensorFlow Lite for Microcontrollers
- Networking: AsyncTCP, ESPAsyncWebServer, PubSubClient (MQTT)
- Data Serialization: ArduinoJson

### Hardware Components
- Core: ESP32 Development Boards
- Sensors: DHT20 (Temperature & Humidity via I2C)
- Actuators/Indicators: NeoPixel LED ring, standard LEDs
- Display: I2C LCD 16x2

### Data Flow
1. Data Acquisition: The DHT20 sensor continuously collects environmental data and pushes it to a FreeRTOS Queue.
2. Edge Analysis: The TinyML task peeks at the sensor data, maintains a historical time-series buffer, and runs inference to determine the environmental safety state.
3. Telemetry Publishing: The MQTT task publishes both raw sensor data and AI predictions to the CoreIoT Cloud.
4. Cloud Rules Engine: The Cloud Rulechain analyzes the telemetry and triggers RPC commands to the Actuator Node if an anomaly is detected.
5. Actuation: The Actuator Node receives the RPC command and drives the NeoPixel/LEDs to display visual warnings, or processes manual overrides from the local WebServer.

## Project Structure
- src/main.cpp: Entry point and FreeRTOS task initialization.
- src/tinyml.cpp: TensorFlow Lite setup, sliding window logic, and model execution.
- src/task_core_iot.cpp: MQTT connectivity, Telemetry publishing, and RPC handling.
- src/task_webserver.cpp: Asynchronous WebServer and WebSocket configuration.
- src/task_handler.cpp: WebSocket payload processing and credential parsing.
- src/temp_humi_monitor.cpp: I2C sensor reading and LCD display logic.
- platformio.ini: Environment configuration for Multi-Board builds.

## Setup & Compilation

1. Environment: Install PlatformIO IDE (VSCode Extension).
2. Clone Repository: Clone this project to your local workspace.
3. Multi-Board Compilation:
   - To build for the AI Telemetry Node (Board 1), ensure the build flag `-D is_board_1=true` is set in platformio.ini.
   - To build for the Neo Actuator Node (Board 2), set `-D is_board_1=false`.
4. Upload: Connect the ESP32 and click "Upload" in PlatformIO.
5. Provisioning: On first boot, connect to the ESP32's Wi-Fi Access Point to access the Captive Portal and input your local Wi-Fi and MQTT Cloud credentials.

## Evaluation & Benchmarking
The project includes a dedicated benchmarking routine (`evaluate_tinyml_task`) that tests the TinyML model against synthesized extreme edge cases. The benchmark measures inference latency and classification accuracy across different risk scenarios to validate edge computing performance.

## License
This project is for educational and portfolio demonstration purposes. All rights reserved by the original authors.