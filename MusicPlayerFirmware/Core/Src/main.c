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
#include "ssd1306.h"
#include "ssd1306_fonts.h"
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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_PWM_TIM htim3
#define LED_PWM_SLOW 63999u
#define LED_PWM_FAST 63u
#define LED_PAUSE_CHANNEL TIM_CHANNEL_1
#define LED_PLAY_CHANNEL TIM_CHANNEL_2
#define COUNTS_PER_DETENT 4
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Event queue */
void push_event(Event_t e);
Event_t pop_event(void);

/* State functions */
void playback_init(void);
void playback_run(Event_t event);

void error_init(void);
void error_run(Event_t event);

/* Display */
void display_init(void);

/* Rotary encoder */
void encoder_init(Encoder_t* enc, TIM_HandleTypeDef* htim);
void encoder_poll(Encoder_t* enc);

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
    encoder_init(&encoder, &htim3);
    display_init();

    current_state->init();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {

        // update the current encoder value
        encoder_poll(&encoder);

        const Event_t event = pop_event();
        if (current_state->run) {
            current_state->run(event);
        }

        HAL_Delay(10);
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
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
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

void playback_init(void) {
    __HAL_TIM_SET_PRESCALER(&LED_PWM_TIM, LED_PWM_FAST);
    HAL_TIM_PWM_Start(&LED_PWM_TIM, LED_PAUSE_CHANNEL);
    HAL_TIM_PWM_Stop(&LED_PWM_TIM, LED_PLAY_CHANNEL);
}

void playback_run(const Event_t event) {
    switch (event) {
    case EVENT_NONE:
        break;
    case EVENT_NEXT:
        break;
    case EVENT_PREV:
        break;
    }
}

void error_init(void) {
    __HAL_TIM_SET_PRESCALER(&LED_PWM_TIM, LED_PWM_SLOW);
    HAL_TIM_PWM_Start(&LED_PWM_TIM, LED_PAUSE_CHANNEL);
    HAL_TIM_PWM_Stop(&LED_PWM_TIM, LED_PLAY_CHANNEL);
}

void error_run(const Event_t event) {
    (void)event;
}

void display_init(void) {
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0,0);
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
