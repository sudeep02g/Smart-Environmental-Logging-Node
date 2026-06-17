# Smart Environmental Logging Node
[![Launch in Wokwi](https://img.shields.io/badge/Launch%20in-Wokwi-25c2a0?style=for-the-badge&logo=cpu)](https://wokwi.com/projects/467095552487492609)

A highly optimized, concurrent telemetry firmware platform engineered for the ESP32 microcontroller using Embedded C. This project simulates a sensor network designed to capture environmental metrics with maximum execution efficiency and minimal power consumption.

## 🛠️ Core Engineering Features
* **Non-Blocking Multitasking Architecture:** Replaced all CPU-stalling `delay()` states with high-resolution register timestamp evaluations (`millis()`) to decouple sensor loops, display updates, and data logging tasks into independent execution threads.
* **Low-Power Sampling Scheduling:** Programmed an internal **Hardware Timer Interrupt** to automatically wake the processor and trigger precise ADC / multi-sensor sampling arrays at strict 5-second intervals, minimizing active CPU clock cycles.
* **Asynchronous Event Handling:** Developed a high-priority **External Hardware Interrupt Service Routine (ISR)** mapped to user physical tactile inputs to instantly execute system state overrides with zero main-thread polling lag.
* **Synchronous Bus Topologies:** Integrated an address-specific **I2C peripheral communication driver** to dynamically format, serialize, and draw live telemetry data diagnostics onto an SSD1306 OLED interface.
* **Telemetry Serialization:** Built a custom UART transmission engine to package live telemetry metrics into structured, CSV-compliant data frames for diagnostic logging on a host computer monitor.

## 🗂️ File Structure
* `main.c` / `sketch.ino` - Primary firmware entry point, non-blocking execution scheduler, and peripheral initialization.
* `diagram.json` - Complete hardware layout configuration and pin-routing mappings for the Wokwi execution sandbox.

## 💻 Tech Stack & Tools
* **Language:** Embedded C
* **Target Hardware core:** ESP32 (Dual-Core Tensilica architecture)
* **Peripherals Configuration:** GPIO, Hardware Timers, ADC, UART, I2C, SPI
* **Development & Simulation Tools:** Wokwi Simulator, VS Code, Git
