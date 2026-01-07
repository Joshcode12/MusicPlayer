/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "app_fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "vs1053.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum Event {
    EVENT_NONE,
    EVENT_NEXT,
    EVENT_PREV
} Event_t;

typedef struct EventQueue {
    volatile uint8_t head;
    volatile uint8_t tail;
    Event_t buffer[EVENT_QUEUE_SIZE];
} EventQueue_t;

typedef struct State {
    void (*init)(void);
    void (*run)(Event_t event);
} State_t;

typedef struct Encoder {
    TIM_HandleTypeDef* htim;
    int32_t last_counter;
    int32_t accum;
    int8_t steps;
} Encoder_t;

typedef enum ErrorType {
    ERROR_NONE,
    ERROR_DISK,
    ERROR_RESET
} ErrorType_t;

typedef enum DisplayMode {
    DISPLAY_MODE_PLAYBACK,
    DISPLAY_MODE_ERROR
} DisplayMode_t;

typedef struct MP3Info {
    char title[64];
    uint16_t duration;
} MP3Info_t;

typedef struct Flags {
    bool is_playing : 1;
} Flags_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_PWM_TIM htim3
#define LED_PWM_SLOW 63999u
#define LED_PWM_FAST 63u
#define LED_PAUSE_CHANNEL TIM_CHANNEL_1
#define LED_PLAY_CHANNEL TIM_CHANNEL_2
#define COUNTS_PER_DETENT 4
#define SD_CHECK_INTERVAL 1000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Event Queue */
EventQueue_t event_queue = {0};

/* State */
const State_t STATE_PLAYBACK;
const State_t STATE_ERROR;

const State_t* current_state = &STATE_PLAYBACK;

/* Hardware */
Encoder_t encoder;
Flags_t flags = {0};
FATFS fatfs;
FIL file;

/* Audio */
uint16_t current_track = 1, max_tracks = 0;;
uint8_t volume = 50;
MP3Info_t mp3_info = {
    .title = "STARTUP...",
    .duration = 0
};

/* Error */
ErrorType_t current_error = ERROR_NONE;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Event queue */
void push_event(Event_t e);
Event_t pop_event(void);

/* State functions */
void change_state(const State_t* new_state);

void playback_init(void);
void playback_run(Event_t event);

void error_init(void);
void error_run(Event_t event);

/* Display */
void display_init(void);
void display_update(DisplayMode_t mode);

/* Rotary encoder */
void encoder_init(Encoder_t* enc, TIM_HandleTypeDef* htim);
void encoder_poll(Encoder_t* enc);

/* Audio */
uint8_t* track_path(uint16_t track_num);
bool stream_chunk(void);
void open_file(void);
bool get_mp3_info(void);

/* SD card */
bool sdcard_init(void);
bool sdcard_recover(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
const State_t STATE_PLAYBACK = {
    .init = playback_init,
    .run = playback_run,
};

const State_t STATE_ERROR = {
    .init = error_init,
    .run = error_run,
};

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void) {
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_TIM3_Init();
    MX_TIM2_Init();
    if (MX_FATFS_Init() != APP_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN 2 */
    encoder_init(&encoder, &htim2);
    display_init();

    if (!sdcard_init()) {
        current_error = ERROR_DISK;
        current_state = &STATE_ERROR;
    }

    if (!vs1053_init()) {
        current_error = ERROR_RESET;
        current_state = &STATE_ERROR;
    }

    display_update(DISPLAY_MODE_PLAYBACK);

    current_state->init();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        // update the current encoder value
        encoder_poll(&encoder);

        if (encoder.steps != 0) {
            int16_t new_volume = (int16_t)(volume + encoder.steps * 5);

            if (new_volume < 0) new_volume = 0;
            if (new_volume > 100) new_volume = 100;

            volume = new_volume;
            encoder.steps = 0;
            vs1053_set_volume(volume);
        }

        // run the current state
        const Event_t event = pop_event();
        if (current_state->run) {
            current_state->run(event);
        }
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
    */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 8;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Rising_Callback(const uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
    case BTN_PLAYPAUSE_Pin:
        flags.is_playing ^= true;
        if (flags.is_playing) {
            HAL_TIM_PWM_Stop(&LED_PWM_TIM, LED_PAUSE_CHANNEL);
            HAL_TIM_PWM_Start(&LED_PWM_TIM, LED_PLAY_CHANNEL);
        }
        else {
            HAL_TIM_PWM_Start(&LED_PWM_TIM, LED_PAUSE_CHANNEL);
            HAL_TIM_PWM_Stop(&LED_PWM_TIM, LED_PLAY_CHANNEL);
        }
        break;
    case BTN_NEXT_Pin:
        push_event(EVENT_NEXT);
        break;
    case BTN_PREV_Pin:
        push_event(EVENT_PREV);
        break;
    default:
        Error_Handler();
        break;
    }
}

uint8_t* track_path(uint16_t track_num) {
    static uint8_t track_path_buffer[] = "audio/000.mp3";

    if (track_num > max_tracks)
        track_num = 1;

    if (track_num < 1)
        track_num = max_tracks;

    // Write digits into the buffer
    track_path_buffer[6] = '0' + (track_num / 100);
    track_path_buffer[7] = '0' + ((track_num / 10) % 10);
    track_path_buffer[8] = '0' + (track_num % 10);

    return track_path_buffer;
}

bool stream_chunk(void) {
    uint8_t buffer[1024] = {};
    UINT bytes_read = 0;

    if (f_read(&file, buffer, sizeof(buffer), &bytes_read) != FR_OK) {
        f_close(&file);
        vs1053_start_new_track();
        current_error = ERROR_DISK;
        change_state(&STATE_ERROR);
        return false;
    }

    if (bytes_read == 0)
        return false;

    vs1053_send_data(buffer, bytes_read);
    return true;
}

bool get_mp3_info(void) {
    uint8_t header[10];
    UINT br;

    f_lseek(&file, 0);
    if (f_read(&file, header, 10, &br) != FR_OK || br != 10)
        return false;

    if (memcmp(header, "ID3", 3) != 0)
        return false;

    const uint8_t version = header[3]; // 2, 3, or 4
    const uint32_t tag_size =
        ((uint32_t)header[6] << 21) |
        ((uint32_t)header[7] << 14) |
        ((uint32_t)header[8] << 7) |
        (uint32_t)header[9];

    uint32_t pos = 10;
    const uint32_t end = 10 + tag_size;

    f_lseek(&file, pos);

    while (pos + 6 < end) {
        uint8_t fh[10];
        const UINT need = (version == 2) ? 6 : 10;

        if (f_read(&file, fh, need, &br) != FR_OK || br != need)
            return false;

        if (fh[0] == 0) // padding
            break;

        uint32_t size;
        bool is_title = false;

        if (version == 2) {
            is_title = (memcmp(fh, "TT2", 3) == 0);
            size = (fh[3] << 16) | (fh[4] << 8) | fh[5];
        }
        else {
            is_title = (memcmp(fh, "TIT2", 4) == 0);

            if (version == 4)
                size = (fh[4] << 21) | (fh[5] << 14) | (fh[6] << 7) | fh[7];
            else
                size = (fh[4] << 24) | (fh[5] << 16) | (fh[6] << 8) | fh[7];
        }

        if (size == 0) {
            pos += need;
            f_lseek(&file, pos);
            continue;
        }

        if (is_title) {
            const uint32_t read_len = (size < 64) ? size : 64;
            uint8_t buff[64];

            if (f_read(&file, buff, read_len, &br) != FR_OK || br != read_len)
                return false;

            if (buff[0] == 0) {
                memcpy(mp3_info.title, &buff[1], read_len - 1);
                mp3_info.title[read_len - 1] = '\0';
            }
            else {
                uint32_t start = 1;

                if (read_len >= 3 && buff[1] == 0xFF && buff[2] == 0xFE)
                    start = 3;

                uint32_t j = 0;
                for (uint32_t i = start; i + 1 < read_len && j < 63; i += 2) {
                    if (buff[i] == 0)
                        break;

                    mp3_info.title[j++] = buff[i];
                }
                mp3_info.title[j] = '\0';
            }

            return true;
        }

        pos += need + size;
        f_lseek(&file, pos);
    }

    return false;
}

void open_file(void) {
    const uint8_t* path = track_path(current_track);

    // Try opening current track
    if (f_open(&file, (const TCHAR*)path, FA_READ) != FR_OK) {
        // Wrap to track 1
        current_track = 1;
        path = track_path(current_track);

        if (f_open(&file, (const TCHAR*)path, FA_READ) != FR_OK) {
            f_close(&file);
            vs1053_start_new_track();
            current_error = ERROR_DISK;
            change_state(&STATE_ERROR);
        }
    }

    if (!get_mp3_info()) {
        strncpy(mp3_info.title, "UNKNOWN", sizeof(mp3_info.title) - 1);
        mp3_info.title[sizeof(mp3_info.title) - 1] = '\0';
        mp3_info.duration = 0;
    }
}

void playback_init(void) {
    __HAL_TIM_SET_PRESCALER(&LED_PWM_TIM, LED_PWM_FAST);
    HAL_TIM_PWM_Start(&LED_PWM_TIM, LED_PAUSE_CHANNEL);
    HAL_TIM_PWM_Stop(&LED_PWM_TIM, LED_PLAY_CHANNEL);

    DIR dir;
    FILINFO fno;

    max_tracks = 0;

    FRESULT res = f_opendir(&dir, "audio");
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno); // Read a directory item
            if (res != FR_OK || fno.fname[0] == 0) break;

            if (!(fno.fattrib & (AM_DIR | AM_HID | AM_SYS))) {
                ++max_tracks;
            }
        }
        f_closedir(&dir); // Close the directory
    }

    open_file();
}

void playback_run(const Event_t event) {
    switch (event) {
    case EVENT_NEXT:
        f_close(&file);
        vs1053_start_new_track();

        ++current_track;
        open_file();
        break;
    case EVENT_PREV:
        f_close(&file);
        vs1053_start_new_track();

        --current_track;
        open_file();
        break;
    case EVENT_NONE:
    default:
        break;
    }

    display_update(DISPLAY_MODE_PLAYBACK);

    if (!flags.is_playing)
        return;

    if (!stream_chunk()) {
        // End of file reached → go to next track
        f_close(&file);
        vs1053_start_new_track();

        ++current_track;
        open_file();
    }
}

void error_init(void) {
    f_close(&file);
    flags.is_playing = false;

    __HAL_TIM_SET_PRESCALER(&LED_PWM_TIM, LED_PWM_SLOW);
    HAL_TIM_PWM_Start(&LED_PWM_TIM, LED_PAUSE_CHANNEL);
    HAL_TIM_PWM_Stop(&LED_PWM_TIM, LED_PLAY_CHANNEL);
}

void error_run(const Event_t event) {
    (void)event;

    static uint32_t last_check = 0;

    display_update(DISPLAY_MODE_ERROR);

    switch (current_error) {
    case ERROR_NONE:
        change_state(&STATE_PLAYBACK);
        break;
    case ERROR_DISK:
        if (HAL_GetTick() - last_check >= SD_CHECK_INTERVAL) {
            last_check = HAL_GetTick();
            if (sdcard_recover()) {
                current_error = ERROR_NONE;
                change_state(&STATE_PLAYBACK);
            }
        }
        break;
    case ERROR_RESET:
        HAL_TIM_PWM_Start(&LED_PWM_TIM, LED_PLAY_CHANNEL);
        break;
    default:
        Error_Handler();
        break;
    }
}

void display_init(void) {
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_UpdateScreen();
}

void display_update(const DisplayMode_t mode) {
    const uint32_t current_time = HAL_GetTick();
    static uint32_t last_update = 0;
    static uint8_t scroll_offset = 0;
    static uint8_t scroll_reset_track = 0;

    const uint16_t current_sec = vs1053_get_current_decode_time();

    if (last_update != 0 && (current_time - last_update) < 250)
        return;

    last_update = current_time;

    if (scroll_reset_track != current_track) {
        scroll_offset = 0;
        scroll_reset_track = current_track;
    }

    char buffer[19];

    static const char* const title_msg[] = {
        [DISPLAY_MODE_PLAYBACK] = "PLAYBACK >>",
        [DISPLAY_MODE_ERROR] = "|  ERROR  |"
    };

    static const char* const error_msg[] = {
        [ERROR_DISK] = "Reinsert the card",
        [ERROR_RESET] = "Reset the device",
    };

    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString((char*)title_msg[mode], Font_7x10, White);

    switch (mode) {
    case DISPLAY_MODE_PLAYBACK:
        ssd1306_SetCursor(84, 0);
        ssd1306_WriteString(flags.is_playing ? "PLAY" : "PAUSE", Font_7x10, White);

        // Volume
        buffer[0] = 'V';
        buffer[1] = 'o';
        buffer[2] = 'l';
        buffer[3] = 'u';
        buffer[4] = 'm';
        buffer[5] = 'e';
        buffer[6] = ':';
        buffer[7] = ' ';

        if (volume == 100) {
            buffer[8] = '1';
            buffer[9] = '0';
            buffer[10] = '0';
            buffer[11] = '%';
            buffer[12] = '\0';
        }
        else {
            buffer[8] = (volume / 10) + '0';
            buffer[9] = (volume % 10) + '0';
            buffer[10] = '%';
            buffer[11] = '\0';
        }

        ssd1306_SetCursor(0, 12);
        ssd1306_WriteString(buffer, Font_7x10, White);

        // Scrolling title
        ssd1306_SetCursor(0, 24);
        const uint8_t title_len = strlen(mp3_info.title);

        if (title_len <= 18) {
            ssd1306_WriteString(mp3_info.title, Font_7x10, White);
        }
        else {
            static uint8_t scroll_timer = 0;
            static uint8_t pause_counter = 0;

            // Pause for 2 seconds at start (4 x 500ms updates)
            if (pause_counter < 4) {
                pause_counter++;
                memcpy(buffer, mp3_info.title, 18);
                buffer[18] = '\0';
                ssd1306_WriteString(buffer, Font_7x10, White);
            }
            else {
                // Scroll every 500ms (every 2nd update)
                if (++scroll_timer >= 2) {
                    scroll_timer = 0;
                    if (++scroll_offset > title_len - 18) {
                        scroll_offset = 0;
                        pause_counter = 0; // Reset pause when wrapping
                    }
                }

                memcpy(buffer, &mp3_info.title[scroll_offset], 18);
                buffer[18] = '\0';
                ssd1306_WriteString(buffer, Font_7x10, White);
            }
        }

        // Current time
        uint16_t hours = current_sec / 3600;
        uint8_t minutes = (current_sec % 3600) / 60;
        uint8_t seconds = current_sec % 60;

        buffer[0] = (hours / 10) + '0';
        buffer[1] = (hours % 10) + '0';
        buffer[2] = ':';
        buffer[3] = (minutes / 10) + '0';
        buffer[4] = (minutes % 10) + '0';
        buffer[5] = ':';
        buffer[6] = (seconds / 10) + '0';
        buffer[7] = (seconds % 10) + '0';
        buffer[8] = '\0';

        ssd1306_SetCursor(0, 36);
        ssd1306_WriteString(buffer, Font_7x10, White);

        // Duration
        hours = mp3_info.duration / 3600;
        minutes = (mp3_info.duration % 3600) / 60;
        seconds = mp3_info.duration % 60;

        buffer[0] = (hours / 10) + '0';
        buffer[1] = (hours % 10) + '0';
        buffer[2] = ':';
        buffer[3] = (minutes / 10) + '0';
        buffer[4] = (minutes % 10) + '0';
        buffer[5] = ':';
        buffer[6] = (seconds / 10) + '0';
        buffer[7] = (seconds % 10) + '0';
        buffer[8] = '\0';

        ssd1306_SetCursor(0, 48);
        ssd1306_WriteString(buffer, Font_7x10, White);
        break;

    case DISPLAY_MODE_ERROR:
        ssd1306_SetCursor(0, 22);
        ssd1306_WriteString((char*)error_msg[current_error], Font_7x10, White);
        break;
    default:
        Error_Handler();
        break;
    }

    ssd1306_UpdateScreen();
}

void encoder_init(Encoder_t* enc, TIM_HandleTypeDef* htim) {
    enc->htim = htim;
    enc->last_counter = (int32_t)__HAL_TIM_GET_COUNTER(htim);
    enc->accum = 0;
    HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
}

void encoder_poll(Encoder_t* enc) {
    const int32_t current_counter = (int32_t)__HAL_TIM_GET_COUNTER(enc->htim);
    const int32_t delta = current_counter - enc->last_counter;
    enc->last_counter = current_counter;
    enc->accum += delta;
    const int8_t detents = (int8_t)(enc->accum / COUNTS_PER_DETENT);
    enc->accum -= detents * COUNTS_PER_DETENT;
    enc->steps = detents;
}

bool sdcard_init(void) {
    if (disk_initialize(0) == 0 && f_mount(&fatfs, "", 1) == FR_OK) {
        return true;
    }
    return false;
}

bool sdcard_recover(void) {
    // set all the spi pins to high
    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    // unmount and reset the drive state
    f_mount(NULL, "", 0);
    extern Disk_drvTypeDef disk;
    disk.is_initialized[0] = 0;

    if (disk_initialize(0) != 0) return false;
    if (f_mount(&fatfs, "", 1) != FR_OK) return false;

    return true;
}

void change_state(const State_t* new_state) {
    if (current_state != new_state) {
        current_state = new_state;
        if (current_state->init) {
            current_state->init();
        }
    }
}

void push_event(const Event_t e) {
    const uint8_t head = event_queue.head;
    const uint8_t next = (head + 1) & (EVENT_QUEUE_SIZE - 1);
    if (next != event_queue.tail) {
        event_queue.buffer[head] = e;
        __DMB();
        event_queue.head = next;
    }
}

Event_t pop_event(void) {
    const uint8_t tail = event_queue.tail;
    if (tail == event_queue.head) return EVENT_NONE;
    const Event_t e = event_queue.buffer[tail];
    __DMB();
    event_queue.tail = (tail + 1) & (EVENT_QUEUE_SIZE - 1);
    return e;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();

#ifdef DEBUG
    __BKPT(0);
#endif

    while (1) {
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        for (volatile uint32_t i = 0; i < 2000000; i++);
    }
    /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
