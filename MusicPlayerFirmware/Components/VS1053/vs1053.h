//
// Created by joshs on 1/4/2026.
//

#ifndef MUSICPLAYERFIRMWARE_VS1053_H
#define MUSICPLAYERFIRMWARE_VS1053_H

#include <stdbool.h>
#include <stdint.h>

#define VS1053_SCI_READ 0x03  // Serial read address
#define VS1053_SCI_WRITE 0x02 // Serial write address

#define VS1053_REG_MODE 0x00       // Mode control
#define VS1053_REG_STATUS 0x01     // Status of VS1053b
#define VS1053_REG_BASS 0x02       // Built-in bass/treble control
#define VS1053_REG_CLOCKF 0x03     // Clock frequency + multiplier
#define VS1053_REG_DECODETIME 0x04 // Decode time in seconds
#define VS1053_REG_AUDATA 0x05     // Misc. audio data
#define VS1053_REG_WRAM 0x06       // RAM write/read
#define VS1053_REG_WRAMADDR 0x07   // Base address for RAM write/read
#define VS1053_REG_HDAT0 0x08      // Stream header data 0
#define VS1053_REG_HDAT1 0x09      // Stream header data 1
#define VS1053_REG_VOLUME 0x0B     // Volume control

#define VS1053_GPIO_DDR 0xC017   // Direction
#define VS1053_GPIO_IDATA 0xC018 // Values read from pins
#define VS1053_GPIO_ODATA 0xC019 // Values set to the pins

#define VS1053_INT_ENABLE 0xC01A // Interrupt enable

#define VS1053_MODE_SM_DIFF 0x0001     // Differential, 0: normal in-phase audio, 1: left channel inverted
#define VS1053_MODE_SM_LAYER12 0x0002  // Allow MPEG layers I & II
#define VS1053_MODE_SM_RESET 0x0004    // Soft reset
#define VS1053_MODE_SM_CANCEL 0x0008   // Cancel decoding current file
#define VS1053_MODE_SM_EARSPKLO 0x0010 // EarSpeaker low setting
#define VS1053_MODE_SM_TESTS 0x0020    // Allow SDI tests
#define VS1053_MODE_SM_STREAM 0x0040   // Stream mode
#define VS1053_MODE_SM_SDINEW 0x0800   // VS1002 native SPI modes
#define VS1053_MODE_SM_ADPCM 0x1000    // PCM/ADPCM recording active
#define VS1053_MODE_SM_LINE1 0x4000    // MIC/LINE1 selector, 0: MICP, 1: LINE1
#define VS1053_MODE_SM_CLKRANGE 0x8000 // Input clock range, 0: 12..13 MHz, 1: 24..26 MHz

#define VS1053_SCI_AIADDR 0x0A // Indicates the start address of the application code written earlier

// with SCI_WRAMADDR and SCI_WRAM registers.
#define VS1053_SCI_AICTRL0 0x0C  // SCI_AICTRL register 0. Used to access the user's application program
#define VS1053_SCI_AICTRL1 0x0D  // SCI_AICTRL register 1. Used to access the user's application program
#define VS1053_SCI_AICTRL2 0x0E  // SCI_AICTRL register 2. Used to access the user's application program
#define VS1053_SCI_AICTRL3 0x0F  // SCI_AICTRL register 3. Used to access the user's application program
#define VS1053_SCI_WRAM 0x06     // RAM write/read
#define VS1053_SCI_WRAMADDR 0x07 // Base address for RAM write/read

#define VS1053_PARA_PLAYSPEED 0x1E04 // 0,1 = normal speed, 2 = 2x, 3 = 3x etc

#ifndef VS1053_WAIT_FOR_DREQ
#define VS1053_WAIT_FOR_DREQ()  do { /* user handles DREQ externally */ } while (0)
#endif

bool vs1053_init(void);
bool vs1053_soft_reset(void);
void vs1053_set_volume(uint8_t percent);
void vs1053_play(void);
void vs1053_pause(void);
void vs1053_sine_test(void);
void vs1053_flush_end(void);
void vs1053_start_new_track(void);
void vs1053_send_data(const uint8_t* data, uint16_t len);

#endif //MUSICPLAYERFIRMWARE_VS1053_H
