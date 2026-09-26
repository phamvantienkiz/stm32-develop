# STM32F429 Development Workspace

This repository contains my real-world projects and experiments developed for the **STM32F429** microcontroller series using **STM32CubeIDE** and **STM32CubeMX**.

## 📂 Projects Overview

The projects are currently located in the `workspace_0.0.1` folder. Here are the main projects included:

- **`BlinkLED`**: A basic "Hello World" project for embedded systems, demonstrating GPIO configuration and toggling an LED on the board.
- **`lcd-gyro-bsp`**: A project integrating the LCD display and the onboard Gyroscope sensor using the Board Support Package (BSP).
- **`Sensor_Gyro_UART`**: Reads data from the Gyroscope sensor and transmits the readings via UART to a computer for monitoring.
- **`UART-Ex`**: Examples and implementations of basic UART communication (Transmit/Receive).
- **`UART_TIM3_CheckHW`**: A hardware verification project utilizing Timer 3 (TIM3) alongside UART for debugging and checking hardware statuses.

## 🛠 Tools & Environment

- **Microcontroller:** STM32F429 (e.g., STM32F429I-DISCO / STM32F429ZI)
- **IDE:** [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
- **Code Generator:** [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)
- **HAL Driver:** STM32 HAL (Hardware Abstraction Layer)

## 🚀 How to use this repository

1. **Clone the repository:**
   ```bash
   git clone https://github.com/phamvantienkiz/stm32-develop.git
   ```
2. **Open with STM32CubeIDE:**
   - Launch STM32CubeIDE.
   - You can either open the `workspace_0.0.1` as your workspace directly, or (recommended) create a new workspace and choose **File > Import > General > Existing Projects into Workspace**, then select the specific project folders you want to import.
3. **Build & Flash:**
   - Click the "Build" button (hammer icon) to compile the code.
   - Click "Run" or "Debug" to flash the `.elf` binary onto your STM32F429 board.

## 🌟 Conclusion & Welcome
Welcome to the world of Embedded Systems! The projects in this repository serve as fundamental stepping stones to get familiar with STM32 microcontrollers and their peripherals. I hope this repository helps you on your learning journey.

Wishing you great success with your embedded projects! Happy Coding! 🚀
