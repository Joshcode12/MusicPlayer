* * * * *

STM32 Music Player
==================

A standalone MP3 player built around the STM32G031K8 microcontroller, featuring SD card storage, OLED display, rotary encoder control, and a VS1053b hardware audio decoder. Designed to run on extremely limited Flash/RAM while remaining responsive and fully non‐blocking.

* * * * *

Hardware Components
-------------------

- **MCU**: STM32G031K8 (NUCLEO‐G031K8 board)
- **Audio Decoder**: VS1053b (MP3/AAC/WMA/MIDI hardware decoder)
- **Display**: SSD1306 128×64 OLED (I²C)
- **Storage**: SD Card (SPI, 3‑wire mode)
- **Controls**:
    - Rotary encoder (navigation + volume)
    - 3 push buttons (Play, Next, Previous)
    - Hardware debouncing via RC filters
- **Indicators**:
    - 2 PWM‑controlled LEDs (Play/Pause status)

* * * * *

Features
--------
- MP3 playback via VS1053b
- SD card access using lightweight SPI driver
- OLED UI with playback state and metadata
- Rotary encoder for navigation and volume
- Play / Pause / Next / Previous buttons
- Non‐blocking state machine
- Automatic ID3 tag extraction (title, duration)
- Simple numeric track system (`001.mp3`, `002.mp3`, ...)

* * * * *

SD Card Layout
--------------

Playback is intentionally **non‐recursive** for speed and RAM efficiency.

```
/audio/
    001.mp3
    002.mp3
    003.mp3
```

* * * * *

Pin Configuration
-----------------

### SPI1 --- Audio & SD Card
- PA5 --- SPI1_SCK
- PA6 --- SPI1_MISO
- PA7 --- SPI1_MOSI
- PA4 --- SD_CS
- PA11 --- MP3_CS
- PA12 --- XD_CS
- PB3 --- DREQ (VS1053b Data Request)

### I2C1 --- OLED Display
- PA9 --- I2C1_SCL (400 kHz)
- PA10 --- I2C1_SDA

### TIM2 --- Rotary Encoder
- PA0 --- TIM2_CH1
- PA1 --- TIM2_CH2

### TIM3 --- PWM LEDs
- PB4 --- TIM3_CH1 (Pause LED)
- PB5 --- TIM3_CH2 (Play LED)

### Buttons
- PB0 --- Play/Pause
- PB8 --- Next
- PB9 --- Previous

### Other
- PC6 --- LD3 (Status LED)

* * * * *

Software Architecture
---------------------

### Initialization Order
1. `HAL_Init()`
2. `SystemClock_Config()` --- 64 MHz (HSI + PLL)
3. `MX_GPIO_Init()` --- all CS pins HIGH
4. `MX_DMA_Init()`
5. `MX_SPI1_Init()` --- 2 Mbps
6. `MX_I2C1_Init()` --- 400 kHz
7. `MX_TIM2_Init()` --- encoder mode
8. `MX_TIM3_Init()` --- PWM LEDs
9. `MX_FATFS_Init()` --- filesystem

### DMA Channels
- DMA1_CH1 --- SPI1_RX
- DMA1_CH2 --- SPI1_TX
- DMA1_CH4 --- I2C1_TX

### Libraries
- **HAL** --- STM32 HAL
- **FatFS** --- R0.12c
- **SD Card SPI driver** --- kiwih/cubeide-sd-card (modified for 3‑wire SPI)
- **SSD1306 OLED** --- afiskon/stm32-ssd1306

* * * * *

Playback Architecture
---------------------

### Event Queue
- Lock‐free SPSC ring buffer
- Used for button and encoder events
- Extremely lightweight (8‑entry queue)

### State Machine
- **Playback State**
- **Error State**
- Fully non‐blocking (no `HAL_Delay()` inside states)

### ID3 Tag Extraction
- Reads ID3v1 or ID3v2 tags when loading a track
- Extracts:
    - Title
    - Duration
- Requires only a small buffer (64--128 bytes)

### File Naming
- Firmware generates filenames:
  ```
  audio/%03d.mp3
  ```
- No directory scanning required
- Perfect for low‑RAM systems

* * * * *

Development Phases
------------------

### Phase 1 --- Hardware Bring‐Up
- [x] GPIO + PWM LED test
- [x] Button input
- [x] Rotary encoder reading

### Phase 2 --- Peripherals
- [x] SSD1306 display
- [x] SD card SPI driver
- [x] FatFS mounting

### Phase 3 --- Audio
- [X] VS1053b SPI communication
- [X] MP3 streaming from SD card
- [ ] ID3‐based playlist support

### Phase 4 --- Integration
- [ ] Playback controls
- [ ] Track metadata display
- [ ] Volume control via encoder

* * * * *

Building and Flashing
---------------------

### Build (CMake)

```
mkdir build && cd build
cmake ..
make
```

### Flash (st-flash)

```
st-flash write build/MusicPlayerFirmware.bin 0x8000000
```

### Flash (STM32CubeProgrammer)

Use ST‑Link to flash the `.elf` file.

* * * * *

License
-------

MIT

Author
------

Joshua E

Acknowledgments
---------------
-   STMicroelectronics --- HAL/LL drivers
-   VLSI Solution --- VS1053b documentation
-   Aleksander Alekseev --- SSD1306 library
-   Hammond Pearce --- SD card SPI driver

* * * * *
