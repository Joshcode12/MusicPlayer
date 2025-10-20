# STM32G031K8 Music Player (Bare-Metal)

A lightweight MP3 player built on the **STM32G031K8 NUCLEO**, featuring:
- VS1053B MP3 decoder chip  
- SD card for file storage  
- SSD1306 OLED display (I²C)  
- Rotary encoder and 3 buttons for navigation  
- File upload via ST-LINK Virtual COM Port (VCP)

This project is written **without HAL**, using only direct register access and minimal drivers.

---

## Hardware Overview

| Component | Interface | Notes |
|------------|------------|-------|
| **STM32G031K8** | MCU | Cortex-M0+, 64 KB Flash, 8 KB RAM |
| **VS1053B** | SPI | Handles MP3 decoding, outputs analog audio |
| **SD Card** | SPI (shared with VS1053B) | Stores `.mp3` files (FAT32 via FatFs or custom FS) |
| **SSD1306 OLED** | I²C | 128×64 display for UI |
| **Rotary Encoder** | GPIO / TIM2 Encoder Mode | Volume & menu control |
| **Buttons (3)** | GPIO / EXTI | Play/Pause, Next, Previous |
