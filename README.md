# STM32G031K8 Music Player

A lightweight MP3 player built on the **STM32G031K8 NUCLEO**, featuring:
- VS1053B music decoder chip  
- SD card for file storage  
- SSD1306 OLED display (I²C)  
- Rotary encoder and 3 buttons for navigation  
- File upload via ST-LINK Virtual COM Port (VCP)

VS1053 and SD card are in the [Adafruit Music Maker FeatherWing mosdule](https://learn.adafruit.com/adafruit-music-maker-featherwing).

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

---

## STM32G031K8 Nucleo Pinout

| Function        | Description            | STM32 Pin | Notes                            |
| --------------- | ---------------------- | --------- | -------------------------------- |
| **SPI1_SCK**    | SPI Clock              | `PA5`     | Shared SPI bus                   |
| **SPI1_MISO**   | SPI Data In            | `PA6`     | Shared SPI bus                   |
| **SPI1_MOSI**   | SPI Data Out           | `PA7`     | Shared SPI bus                   |
| **XDCS**        | VS1053B Data Select    | `PA15`    | Active low                       |
| **MP3_CS**      | VS1053B Command Select | `PB0`     | Active low                       |
| **SD_CS**       | SD Card Chip Select    | `PB9`     | Active low                       |
| **DREQ**        | VS1053B Data Request   | `PB1`     | Input (VS1053 → MCU)             |
| **ENC_R**       | Rotary Encoder A       | `PA1`     | TIM2_CH2 (optional encoder mode) |
| **ENC_L**       | Rotary Encoder B       | `PA0`     | TIM2_CH3 (optional encoder mode) |
| **BTN_PLAY**    | Play / Pause Button    | `PB3`     | Active low                       |
| **BTN_NEXT**    | Next Track Button      | `PB5`     | Active low                       |
| **BTN_BACK**    | Previous Track Button  | `PB4`     | Active low                       |
| **OLED_SCL**    | SSD1306 I²C Clock      | `PB6`     | I²C1 SCL                         |
| **OLED_SDA**    | SSD1306 I²C Data       | `PB7`     | I²C1 SDA                         |
| **LED_PAUSED**  | Playback Paused LED    | `PB2`     | ON when paused                   |
| **LED_PLAYING** | Playback Active LED    | `PB8`     | ON when playing                  |
| **GND**         | Ground                 | —         | Common ground                    |
| **3V3**         | Power Supply           | —         | 3.3 V logic                      |

---

## Breadboard Schemantic

<img width="3300" height="2550" alt="image" src="https://github.com/user-attachments/assets/b759768f-c774-4ba0-9978-6a0df93a261f" />

---

## Breadboard image

![20251029_160259](https://github.com/user-attachments/assets/0bd344e6-23b0-4e7c-91ee-bad7e1fd07f4)
