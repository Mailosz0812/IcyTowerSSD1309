/* USER CODE BEGIN Header */
// ... (sekcja nagłówkowa bez zmian)
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "sx1509.h"
#include "ssd1306_fonts.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "stm32l4xx_hal_pwr.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    int x_start;
    int x_end;
    int y;
    int active;
} Platform;

typedef enum {
    PLAYER_GROUNDED,
    PLAYER_JUMPING
} PlayerState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_PLATFORMS 20
#define PLATFORM_LENGTH 30
#define PLATFORM_SPACING 25
#define SCROLL_THRESHOLD 20
#define GROUND_LEVEL 63
#define CAMERA_SPEED 1
#define GRAVITY 1
#define JUMP_VELOCITY -12
#define PLAYER_HEIGHT 16
#define PLAYER_WIDTH 16
#define MAX_GAME_SPEED 3.0
#define SCREEN_HEIGHT 64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t rxBuffer[2], scndChar = 0;
int checkMenu = 0, gameStart = 0, gameOver = 0;
int player_x = 60, player_y = GROUND_LEVEL-PLAYER_HEIGHT;
int32_t camera_y = 0;
uint32_t score = 0, highest_score = 0;
int32_t highest_platform_y = GROUND_LEVEL;
float game_speed = 1.0;
int jump_velocity = 0;
char last_key = 0;
PlayerState player_state = PLAYER_GROUNDED;
Platform platforms[MAX_PLATFORMS];
volatile uint8_t restart_flag = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
void generate_platforms();
void update_game_world();
void draw_world();
void reset_game();
void handle_input();
void update_jump_state();
void cleanup_platforms();
void game_over_screen();
int32_t is_on_platform();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch){
    if(ch == '\n'){
        __io_putchar('\r');
    }
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return 1;
}

void startMenudisplay(int x) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(28, 24);
    ssd1306_WriteString("Start Game", Font_7x10, White);
    ssd1306_SetCursor(48, 36);
    ssd1306_WriteString("Exit", Font_7x10, White);
    ssd1306_Line(28, x, 97, x, White);
    ssd1306_UpdateScreen();
}

void generate_platforms() {
    static int32_t last_y = GROUND_LEVEL;

    while (last_y > (camera_y - 128)) {
        for (int i = 0; i < MAX_PLATFORMS; i++) {
            if (!platforms[i].active) {
                platforms[i].x_start = 4 + (rand() % (128 - 8 - PLATFORM_LENGTH));
                platforms[i].x_end = platforms[i].x_start + PLATFORM_LENGTH;
                platforms[i].y = last_y - PLATFORM_SPACING;
                platforms[i].active = 1;
                last_y = platforms[i].y;
                break;
            }
        }
        if (last_y < -1000) break;
    }
}

void cleanup_platforms() {
    for (int i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].active && (platforms[i].y > camera_y + SCREEN_HEIGHT * 2)) {
            platforms[i].active = 0;
        }
    }
}

void update_game_world() {
    game_speed = 1.0 + (score * 0.001);
    if (game_speed > MAX_GAME_SPEED) {
        game_speed = MAX_GAME_SPEED;
    }

    if (player_y < (camera_y + SCROLL_THRESHOLD)) {
        camera_y = player_y - SCROLL_THRESHOLD;
    }

    if (player_y > camera_y + SCREEN_HEIGHT) {
        gameOver = 1;
        gameStart = 0;
        if (score > highest_score) {
            highest_score = score;
        }
    }
}

void draw_world() {
    ssd1306_Fill(Black);

    for (int i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].active) {
            int screen_y = platforms[i].y - camera_y;
            if (screen_y >= -16 && screen_y < 80) {
                ssd1306_Line(platforms[i].x_start, screen_y,
                            platforms[i].x_end, screen_y, White);
            }
        }
    }

    int player_screen_y = player_y - camera_y;
    if (player_screen_y >= -16 && player_screen_y < 80) {
        ssd1306_DrawIcyTowerModel(player_x, player_screen_y);
    }

    ssd1306_Line(4, 0, 4, 63, White);
    ssd1306_Line(124, 0, 124, 63, White);

    char score_str[16];
    snprintf(score_str, sizeof(score_str), "%lu", score);
    ssd1306_SetCursor(90, 0);
    ssd1306_WriteString(score_str, Font_7x10, White);

    ssd1306_UpdateScreen();
}

void game_over_screen() {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(15, 10);
    ssd1306_WriteString("GAME OVER", Font_11x18, White);

    char hi_score_str[20];
    snprintf(hi_score_str, sizeof(hi_score_str), "HI: %lu", highest_score);
    ssd1306_SetCursor(26, 35);
    ssd1306_WriteString(hi_score_str, Font_7x10, White);

    ssd1306_SetCursor(8, 50);
    ssd1306_WriteString("Press RESTART button", Font_6x8, White);
    ssd1306_UpdateScreen();
}

int32_t is_on_platform() {
    for (int i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].active &&
            (player_y + PLAYER_HEIGHT == platforms[i].y) &&
            (player_x - PLAYER_WIDTH/2 <= platforms[i].x_end) &&
            (player_x + PLAYER_WIDTH/2 >= platforms[i].x_start)) {
            return platforms[i].y;
        }
    }
    if (player_y + PLAYER_HEIGHT == GROUND_LEVEL) {
        return GROUND_LEVEL;
    }
    return -1;
}

void handle_input() {
    if (last_key == 'a' && player_x > 4) {
        player_x -= 12;
    }
    else if (last_key == 'd' && player_x < 116) {
        player_x += 12;
    }

    if (last_key == 'w' && player_state == PLAYER_GROUNDED) {
        jump_velocity = JUMP_VELOCITY;
        player_state = PLAYER_JUMPING;
    }

    last_key = 0;
}

void update_jump_state() {
    if (player_state == PLAYER_JUMPING) {
        player_y += jump_velocity;
        jump_velocity += GRAVITY;

        int landed = 0;
        int32_t platform_y = is_on_platform();
        if (platform_y != -1) {
            landed = 1;
            player_y = platform_y - PLAYER_HEIGHT;
        }

        if (player_y + PLAYER_HEIGHT >= GROUND_LEVEL) {
            landed = 1;
            player_y = GROUND_LEVEL - PLAYER_HEIGHT;
        }

        if (landed) {
            player_state = PLAYER_GROUNDED;
            jump_velocity = 0;

            if (platform_y != -1) {
                if (platform_y < highest_platform_y) {
                    score += highest_platform_y - platform_y;
                    highest_platform_y = platform_y;
                }
            }
        }
    }
    else {
        if (is_on_platform() == -1) {
            player_state = PLAYER_JUMPING;
            jump_velocity = 0;
        }
    }
}

void reset_game() {
    srand(HAL_GetTick());

    player_x = 60;
    player_y = GROUND_LEVEL - PLAYER_HEIGHT;
    camera_y = 0;
    score = 0;
    game_speed = 1.0;
    jump_velocity = 0;
    highest_platform_y = GROUND_LEVEL;
    player_state = PLAYER_GROUNDED;

    for (int i = 0; i < MAX_PLATFORMS; i++) {
        platforms[i].active = 0;
    }

    platforms[0] = (Platform){4, 124, GROUND_LEVEL, 1};
    generate_platforms();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == USER_BUTTON_Pin) {
        HAL_Delay(50);
        if (HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_RESET) {
            restart_flag = 1;
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    uint8_t fstChar = rxBuffer[0];
    if (huart->Instance == USART2) {
        if (!gameStart && !gameOver) {
            if (fstChar == 'u') {
                startMenudisplay(34);
                checkMenu = 1;
            } else if (fstChar == 'j') {
                startMenudisplay(45);
                checkMenu = 0;
            } else if (fstChar == 'k') {
                if (checkMenu) {
                    gameStart = 1;
                    reset_game();
                    draw_world();
                } else {
                    // Wybrano Exit - przejście w tryb uśpienia
                    ssd1306_Fill(Black);
                    ssd1306_UpdateScreen();
                    HAL_Delay(100);

                    // Wyłącz odbieranie UART
                    HAL_UART_AbortReceive_IT(&huart2);

                    // Konfiguracja wybudzenia przez przycisk
                    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2);
                    HAL_SuspendTick();

                    // Przejście w tryb SLEEP
                    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

                    // Po wybudzeniu
                    HAL_ResumeTick();
                    MX_USART2_UART_Init();
                    MX_SPI1_Init();
                    HAL_UART_Receive_IT(&huart2, rxBuffer, 1);
                    ssd1306_Init();
                    startMenudisplay(34);
                }
            }
        } else {
            last_key = fstChar;
        }
        scndChar = fstChar;
        HAL_UART_Receive_IT(&huart2, rxBuffer, 1);
    }
}
/* USER CODE END 0 */

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_SPI1_Init();

    /* USER CODE BEGIN 2 */
    ssd1306_Init();
    HAL_UART_Receive_IT(&huart2, rxBuffer, 1);
    ssd1306_SetCursor(15,20);
    ssd1306_WriteString("ICY TOWER", Font_11x18, White);
    ssd1306_UpdateScreen();
    HAL_Delay(1000);
    startMenudisplay(34);
    /* USER CODE END 2 */

    while (1) {
        if (gameStart && !gameOver) {
            generate_platforms();
            cleanup_platforms();
            update_game_world();

            handle_input();
            update_jump_state();

            draw_world();

            uint32_t delay = (uint32_t)(50 / game_speed);
            if (delay < 10) delay = 10;
            HAL_Delay(delay);
        }
        else if (gameOver) {
            game_over_screen();
            if (restart_flag) {
                restart_flag = 0;
                gameOver = 0;
                reset_game();
                gameStart = 1;
            }
        }
    }
}

// Reszta kodu pozostaje bez zmian (SystemClock_Config, MX_GPIO_Init, MX_USART2_UART_Init, MX_SPI1_Init)
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_BUTTON_Pin */
  GPIO_InitStruct.Pin = USER_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin PA9 */
  GPIO_InitStruct.Pin = LD2_Pin|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_3 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */


