/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUF_SIZE 128
#define LED_DURATION_MS 5000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* UART Rx Variables */
uint8_t rx_byte;
char rx_buffer[RX_BUF_SIZE];
volatile uint8_t rx_index = 0;
volatile bool line_ready = false;

/* LED Timer Variables (Software non-blocking timers) */
uint32_t led_green_start = 0;
bool led_green_active = false;
uint32_t led_red_start = 0;
bool led_red_active = false;

/* Hardware Timer Variables (Uptime Check) */
volatile uint32_t uptime_seconds = 0;       // Đếm số giây trôi qua từ TIM3
volatile bool heartbeat_ready = false;      // Cờ báo hiệu cần in heartbeat

/* Strings */
const char msg_welcome[] = "\r\n--- HW Timer & UART Ready ---\r\n";
const char msg_done[] = "DONE\r\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Hàm phục vụ ngắt nhận UART (Callback)
  *         Ưu tiên cao hơn (Preemption Priority = 1)
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    /* Kiểm tra ký tự chốt chuỗi: CR hoặc LF */
    if (rx_byte == '\r' || rx_byte == '\n')
    {
      if (rx_index > 0 && !line_ready) // Bỏ qua chuỗi rỗng hoặc \n thừa
      {
        rx_buffer[rx_index] = '\0'; // Đóng chuỗi thành C-String
        line_ready = true;          // Dựng cờ báo cho Main loop xử lý
      }
    }
    else
    {
      /* Nhận byte bình thường, chỉ ghi khi chưa chốt chuỗi và còn chỗ */
      if (!line_ready)
      {
        if (rx_index < (RX_BUF_SIZE - 1))
        {
          rx_buffer[rx_index++] = rx_byte;
        }
        else
        {
          /* Tràn buffer: tự động đóng chuỗi để tránh ghi đè RAM */
          rx_buffer[RX_BUF_SIZE - 1] = '\0';
          line_ready = true;
        }
      }
    }

    /* BẮT BUỘC: Mồi lại ngắt để hứng byte tiếp theo */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}

/**
  * @brief  Hàm phục vụ ngắt tràn Timer (Period Elapsed Callback)
  *         Ưu tiên thấp hơn (Preemption Priority = 2)
  *         Hàm này tự động nhảy vào mỗi 1 giây (do PSC=15999, ARR=999)
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    uptime_seconds++;         // Tăng biến đếm giây
    heartbeat_ready = true;   // Kích cờ để vòng lặp main in ra Terminal
                              // Tuyệt đối không gọi HAL_UART_Transmit trong ngắt!
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

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
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* Bật ngắt Hardware Timer đếm giây (TIM3) */
  HAL_TIM_Base_Start_IT(&htim3);

  /* In chuỗi khởi động */
  HAL_UART_Transmit(&huart1, (uint8_t*)msg_welcome, strlen(msg_welcome), HAL_MAX_DELAY);

  /* Mồi ngắt UART lần đầu tiên */
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  /* Đảm bảo 2 đèn LED đang tắt */
  HAL_GPIO_WritePin(GPIOG, LD3_GREEN_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOG, LD4_RED_Pin, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char tx_buf[128]; // Buffer cục bộ để tạo chuỗi in ra màn hình
  
  while (1)
  {
    /* -----------------------------------------------------------
     * 1. XỬ LÝ HEARTBEAT (Ngắt Timer gửi tín hiệu ra Terminal)
     * ----------------------------------------------------------- */
    if (heartbeat_ready)
    {
      heartbeat_ready = false; // Hạ cờ
      
      /* In ra heartbeat chứng minh MCU không bị treo */
      snprintf(tx_buf, sizeof(tx_buf), "[HW_TIMER] Heartbeat... Uptime: %lu s\r\n", uptime_seconds);
      HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), HAL_MAX_DELAY);
    }

    /* -----------------------------------------------------------
     * 2. XỬ LÝ LỆNH TỪ UART (Kế thừa bài tập trước)
     * ----------------------------------------------------------- */
    if (line_ready)
    {
      /* Echo chuỗi vừa nhận ngược lại màn hình */
      HAL_UART_Transmit(&huart1, (uint8_t*)"Echo: ", 6, HAL_MAX_DELAY);
      HAL_UART_Transmit(&huart1, (uint8_t*)rx_buffer, strlen(rx_buffer), HAL_MAX_DELAY);
      HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);

      /* Phân tích lệnh điều khiển */
      if (strcmp(rx_buffer, "LED_GREEN") == 0)
      {
        HAL_GPIO_WritePin(GPIOG, LD3_GREEN_Pin, GPIO_PIN_SET);
        led_green_active = true;
        led_green_start = HAL_GetTick(); 
      }
      else if (strcmp(rx_buffer, "LED_RED") == 0)
      {
        HAL_GPIO_WritePin(GPIOG, LD4_RED_Pin, GPIO_PIN_SET);
        led_red_active = true;
        led_red_start = HAL_GetTick();
      }
      else if (strcmp(rx_buffer, "GET_TIME") == 0)
      {
        snprintf(tx_buf, sizeof(tx_buf), "--> Current Uptime: %lu seconds\r\n", uptime_seconds);
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), HAL_MAX_DELAY);
      }
      else if (strcmp(rx_buffer, "CHECK_HW") == 0)
      {
        /* Đọc trực tiếp các thanh ghi cấu trúc của TIM3 để kiểm chứng phần cứng */
        uint32_t cnt = TIM3->CNT;
        uint32_t psc = TIM3->PSC;
        uint32_t arr = TIM3->ARR;
        snprintf(tx_buf, sizeof(tx_buf), "--> HW REG: CNT=%lu, PSC=%lu, ARR=%lu\r\n", cnt, psc, arr);
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), HAL_MAX_DELAY);
      }
      else
      {
        HAL_UART_Transmit(&huart1, (uint8_t*)"Unknown Command\r\n", 17, HAL_MAX_DELAY);
      }

      /* Xóa buffer và hạ cờ để hứng lệnh tiếp theo */
      rx_index = 0;
      line_ready = false; 
    }

    /* -----------------------------------------------------------
     * 3. QUẢN LÝ THỜI GIAN ĐÈN LED (NON-BLOCKING)
     * ----------------------------------------------------------- */
    if (led_green_active)
    {
      if ((HAL_GetTick() - led_green_start) >= LED_DURATION_MS)
      {
        HAL_GPIO_WritePin(GPIOG, LD3_GREEN_Pin, GPIO_PIN_RESET);
        led_green_active = false;
        HAL_UART_Transmit(&huart1, (uint8_t*)msg_done, strlen(msg_done), HAL_MAX_DELAY);
      }
    }

    if (led_red_active)
    {
      if ((HAL_GetTick() - led_red_start) >= LED_DURATION_MS)
      {
        HAL_GPIO_WritePin(GPIOG, LD4_RED_Pin, GPIO_PIN_RESET);
        led_red_active = false;
        HAL_UART_Transmit(&huart1, (uint8_t*)msg_done, strlen(msg_done), HAL_MAX_DELAY);
      }
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
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 15999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, LD3_GREEN_Pin|LD4_RED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LD3_GREEN_Pin LD4_RED_Pin */
  GPIO_InitStruct.Pin = LD3_GREEN_Pin|LD4_RED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

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
#ifdef USE_FULL_ASSERT
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
