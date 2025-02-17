# Icy Tower-like Game for STM32L476RG

## Overview
This project is an **Icy Tower-like** game developed for the **STM32L476RG** microcontroller. The game utilizes an **OLED screen** with the **SSD1306 driver** to display graphics. It was developed as part of a **practical class on computer architecture**.

## Features
✔ **Platform-based jumping mechanics** similar to Icy Tower.  
✔ **OLED/LCD screen rendering** using SSD1306 driver.  
✔ **Score tracking and game-over detection.**  
✔ **Menu navigation and input handling via UART.**  
✔ **Uses real-time interrupts for efficient execution.**  

## Hardware & Technology Stack
- **Microcontroller:** STM32L476RG
- **Display:** OLED with SSD1306 driver
- **Programming Language:** C
- **Communication:**
  - **USART2** (for UART communication and input handling)
  - **SPI/I2C** (for screen communication)
- **Real-time Processing:**
  - **Interrupts** (for UART input handling and GPIO button presses)
  - **Timers** (for controlling game speed and physics updates)

## Installation & Setup
### Prerequisites
- **STM32CubeIDE** (for compiling and flashing code)
- **OLED/LCD screen** connected via SPI/I2C
- **STM32L476RG** board

### Steps
1. Clone the repository:
   ```sh
   git clone https://github.com/your-repo/IcyTower_STM32.git
   ```
2. Open the project in **STM32CubeIDE**.
3. Compile and flash the firmware to the board.
4. Reset the board and start playing!

## Usage Guide
### Game Controls (via UART)
- `W` - Jump
- `A` - Move Left
- `D` - Move Right
- `Restart Button` - Restart Game
- `U` , `J` - Naviagate within the menu
- `K`- choose option

### Display & Game Mechanics
- The game world scrolls upwards as the player jumps on platforms.
- Platforms are randomly generated at different heights.
- If the player falls below the screen, **Game Over** is triggered.
