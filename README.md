# STM32 Music Player

A standalone MP3 player built with STM32G031K8 microcontroller, featuring SD card storage, OLED display, and rotary encoder control.

## Hardware Components

- **MCU**: STM32G031K8 (NUCLEO-G031K8 board)
- **Audio Decoder**: VS1053b MP3/AAC/WMA/MIDI decoder
- **Display**: SSD1306 128x64 OLED (I2C)
- **Storage**: SD Card (SPI)
- **Controls**: 
  - Rotary encoder (navigation and volume)
  - 3 push buttons (Play, Next, Previous)
  - Hardware debouncing with filter capacitors
- **Indicators**: 2 PWM-controlled LEDs (Play/Pause status)

## Features

- MP3/AAC/WMA audio playback
- SD card file browser
- OLED display with track information
- Rotary encoder navigation and volume
- Play/pause/next/previous controls
- Visual playback indicators (PWM LEDs)

## Pin Configuration

### SPI1 - Audio & SD Card
- PA5: SPI1_SCK
- PA6: SPI1_MISO
- PA7: SPI1_MOSI
- PA4: SD_CS (Chip Select)
- PA11: MP3_CS (VS1053b Chip Select)
- PA12: XD_CS (VS1053b Data Select)
- PB3: DREQ (VS1053b Data Request)

### I2C1 - Display
- PA9: I2C1_SCL (400 kHz)
- PA10: I2C1_SDA

### TIM2 - Rotary Encoder
- PA0: TIM2_CH1 (Encoder A)
- PA1: TIM2_CH2 (Encoder B)

### TIM3 - PWM LEDs
- PB4: TIM3_CH1 (LED_PAUSE)
- PB5: TIM3_CH2 (LED_PLAY)

### GPIO - Buttons
- PB0: BTN_PLAY
- PB8: BTN_NEXT
- PB9: BTN_PREV

### Other
- PC6: LD3 (Green LED - Status)

## Software Architecture

### Peripheral Initialization Order
1. HAL_Init()
2. SystemClock_Config() - 64 MHz from HSI+PLL
3. MX_GPIO_Init() - Set all CS pins HIGH
4. MX_DMA_Init() - DMA before peripherals
5. MX_SPI1_Init() - 2 Mbps
6. MX_I2C1_Init() - 400 kHz Fast Mode
8. MX_TIM2_Init() - Encoder mode
9. MX_TIM3_Init() - PWM outputs
10. MX_RTC_Init() - Real-time clock
11. MX_FATFS_Init() - FAT filesystem

### DMA Configuration
- **DMA1_Channel1**: SPI1_RX (High Priority)
- **DMA1_Channel2**: SPI1_TX (High Priority)
- **DMA1_Channel4**: I2C1_TX (Low Priority)

### Key Libraries
- **HAL**: STM32 Hardware Abstraction Layer
- **FATFS**: R0.12c for filesystem
- **SDCard**: SD card spi driver *[kiwih/cubeide-sd-card](https://github.com/kiwih/cubeide-sd-card)*
- **SSD1306**: OLED driver *[afiskon/stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306)*

## Development Phases

### Phase 1: Basic Communication
- [ ] GPIO and LED PWM test
- [ ] Button input

### Phase 2: Peripherals
- [ ] SSD1306 display initialization
- [ ] Rotary encoder initialization and reading
- [ ] SD card mounting and file operations

### Phase 3: Audio
- [ ] VS1053b SPI communication
- [ ] MP3 playback from SD card
- [ ] DREQ interrupt handling

### Phase 4: Integration
- [ ] File browser
- [ ] Playback controls
- [ ] Track information display

## Building and Flashing

### Prerequisites
- STM32CubeIDE or CMake toolchain
- ST-Link programmer (integrated on NUCLEO board)
- ARM GCC Compiler

### Build
```bash
# Using CMake
mkdir build && cd build
cmake ..
make

# Using STM32CubeIDE
# Import project and build in IDE
```

### Flash
```bash
# Using st-flash
st-flash write build/MusicPlayerFirmware.bin 0x8000000

# Using STM32CubeProgrammer
# Connect via ST-Link and flash .elf file
```


## License

MIT

## Author

Joshua E

## Acknowledgments

- STMicroelectronics for STM32Cube HAL/LL drivers
- VLSI Solution for VS1053b documentation
- Aleksander Alekseev for SSD1306 library
- Hammond Pearce for spi sd card library

---
