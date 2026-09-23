/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Reference implementation for Sensor Task (SPI + UART + TIM3)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

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

/* L3GD20 Gyroscope Registers & Masks */
#define L3GD20_WHO_AM_I_ADDR  0x0F
#define L3GD20_CTRL_REG1_ADDR 0x20
#define L3GD20_OUT_X_L_ADDR   0x28

#define SPI_READ_BIT          0x80
#define SPI_MULTIPLE_BIT      0x40
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi5;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* UART Rx Variables */
uint8_t rx_byte;
char rx_buffer[RX_BUF_SIZE];
volatile uint8_t rx_index = 0;
volatile bool line_ready = false;

/* LED Timer Variables */
uint32_t led_green_start = 0;
bool led_green_active = false;
uint32_t led_red_start = 0;
bool led_red_active = false;

/* Hardware Timer Variables */
volatile uint32_t uptime_seconds = 0;
volatile bool heartbeat_ready = false;

/* Gyro Stream Variables */
bool stream_active = false;
uint32_t last_stream_time = 0;
#define STREAM_INTERVAL_MS 200 // Gửi dữ liệu Gyro mỗi 200ms

/* Strings */
const char msg_welcome[] = "\r\n--- HW Timer, UART & Gyroscope Ready ---\r\n";
const char msg_done[] = "DONE\r\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI5_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
bool GYRO_Init(void);
void GYRO_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief Cấu hình khởi tạo Cảm biến L3GD20 qua SPI5
  * @retval true nếu tìm thấy cảm biến, false nếu thất bại
  */
bool GYRO_Init(void)
{
  uint8_t tx_data[2];
  uint8_t rx_data[2];

  /* 1. Kiểm tra WHO_AM_I (Xác nhận kết nối) */
  tx_data[0] = L3GD20_WHO_AM_I_ADDR | SPI_READ_BIT; // 0x8F
  tx_data[1] = 0x00; // Dummy byte để tạo clock đọc về

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET); // Kéo CS xuống LOW
  HAL_SPI_TransmitReceive(&hspi5, tx_data, rx_data, 2, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);   // Kéo CS lên HIGH

  uint8_t who_am_i = rx_data[1];
  if (who_am_i != 0xD4 && who_am_i != 0xD3) 
  {
    return false; // Không tìm thấy L3GD20 hoặc I3G4250D
  }

  /* 2. Bật nguồn và trục XYZ (Ghi vào CTRL_REG1) */
  tx_data[0] = L3GD20_CTRL_REG1_ADDR; // Ghi thì không Set bit 0x80
  tx_data[1] = 0x0F; // PD=1, Xen=1, Yen=1, Zen=1

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi5, tx_data, 2, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);

  return true;
}

/**
  * @brief Đọc 6 byte dữ liệu từ OUT_X_L đến OUT_Z_H
  */
void GYRO_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
  /* Địa chỉ thanh ghi đầu tiên + Lệnh Đọc + Lệnh Tự Tăng (Auto-increment) */
  uint8_t tx_cmd = L3GD20_OUT_X_L_ADDR | SPI_READ_BIT | SPI_MULTIPLE_BIT; // 0xE8
  uint8_t rx_buf[7] = {0}; // Chứa 1 byte rác + 6 byte data

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
  
  /* Gửi lệnh 1 byte */
  HAL_SPI_Transmit(&hspi5, &tx_cmd, 1, HAL_MAX_DELAY);
  /* Đọc về 6 byte (khi gọi TransmitReceive hay Receive, STM32 tự tạo clock) */
  HAL_SPI_Receive(&hspi5, &rx_buf[1], 6, HAL_MAX_DELAY);
  
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);

  /* Ép kiểu và ghép byte (Số có dấu bù 2) */
  *x = (int16_t)((rx_buf[2] << 8) | rx_buf[1]);
  *y = (int16_t)((rx_buf[4] << 8) | rx_buf[3]);
  *z = (int16_t)((rx_buf[6] << 8) | rx_buf[5]);
}

/**
  * @brief Ngắt nhận UART (Preemption Priority = 1)
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    if (rx_byte == '\r' || rx_byte == '\n')
    {
      if (rx_index > 0 && !line_ready)
      {
        rx_buffer[rx_index] = '\0';
        line_ready = true;
      }
    }
    else
    {
      if (!line_ready)
      {
        if (rx_index < (RX_BUF_SIZE - 1))
        {
          rx_buffer[rx_index++] = rx_byte;
        }
        else
        {
          rx_buffer[RX_BUF_SIZE - 1] = '\0';
          line_ready = true;
        }
      }
    }
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}

/**
  * @brief Ngắt Timer 1s (Preemption Priority = 2)
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    uptime_seconds++;
    heartbeat_ready = true;
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI5_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  
  /* USER CODE BEGIN 2 */
  
  /* Kéo CS lên mức cao để cảm biến chờ lệnh */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
  
  /* Khởi tạo Gyro */
  if (!GYRO_Init())
  {
    HAL_UART_Transmit(&huart1, (uint8_t*)"Gyro Init FAILED!\r\n", 19, HAL_MAX_DELAY);
  }

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_UART_Transmit(&huart1, (uint8_t*)msg_welcome, strlen(msg_welcome), HAL_MAX_DELAY);
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char tx_buf[128];
  
  while (1)
  {
    /* -----------------------------------------------------------
     * 1. HEARTBEAT
     * ----------------------------------------------------------- */
    if (heartbeat_ready)
    {
      heartbeat_ready = false;
      snprintf(tx_buf, sizeof(tx_buf), "[HW_TIMER] Uptime: %lu s\r\n", uptime_seconds);
      HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), HAL_MAX_DELAY);
    }

    /* -----------------------------------------------------------
     * 2. TỰ ĐỘNG STREAM GYRO (200ms)
     * ----------------------------------------------------------- */
    if (stream_active)
    {
      if ((HAL_GetTick() - last_stream_time) >= STREAM_INTERVAL_MS)
      {
        int16_t gx, gy, gz;
        GYRO_ReadXYZ(&gx, &gy, &gz);
        snprintf(tx_buf, sizeof(tx_buf), "Stream -> X: %d | Y: %d | Z: %d\r\n", gx, gy, gz);
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), HAL_MAX_DELAY);
        
        last_stream_time = HAL_GetTick(); // Cập nhật mốc thời gian
      }
    }

    /* -----------------------------------------------------------
     * 3. XỬ LÝ LỆNH TỪ UART
     * ----------------------------------------------------------- */
    if (line_ready)
    {
      HAL_UART_Transmit(&huart1, (uint8_t*)"Echo: ", 6, HAL_MAX_DELAY);
      HAL_UART_Transmit(&huart1, (uint8_t*)rx_buffer, strlen(rx_buffer), HAL_MAX_DELAY);
      HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, HAL_MAX_DELAY);

      if (strcmp(rx_buffer, "LED_GREEN") == 0)
      {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);
        led_green_active = true;
        led_green_start = HAL_GetTick(); 
      }
      else if (strcmp(rx_buffer, "LED_RED") == 0)
      {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);
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
        snprintf(tx_buf, sizeof(tx_buf), "--> HW REG: CNT=%lu, PSC=%lu, ARR=%lu\r\n", TIM3->CNT, TIM3->PSC, TIM3->ARR);
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), HAL_MAX_DELAY);
      }
      else if (strcmp(rx_buffer, "GET_GYRO") == 0)
      {
        int16_t gx, gy, gz;
        GYRO_ReadXYZ(&gx, &gy, &gz);
        snprintf(tx_buf, sizeof(tx_buf), "Gyro -> X: %d | Y: %d | Z: %d dps\r\n", gx, gy, gz);
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_buf, strlen(tx_buf), HAL_MAX_DELAY);
      }
      else if (strcmp(rx_buffer, "STREAM_ON") == 0)
      {
        stream_active = true;
        last_stream_time = HAL_GetTick();
        HAL_UART_Transmit(&huart1, (uint8_t*)"Gyro Stream STARTED\r\n", 21, HAL_MAX_DELAY);
      }
      else if (strcmp(rx_buffer, "STREAM_OFF") == 0)
      {
        stream_active = false;
        HAL_UART_Transmit(&huart1, (uint8_t*)"Gyro Stream STOPPED\r\n", 21, HAL_MAX_DELAY);
      }
      else
      {
        HAL_UART_Transmit(&huart1, (uint8_t*)"Unknown Command\r\n", 17, HAL_MAX_DELAY);
      }

      rx_index = 0;
      line_ready = false; 
    }

    /* -----------------------------------------------------------
     * 4. QUẢN LÝ THỜI GIAN ĐÈN LED (NON-BLOCKING)
     * ----------------------------------------------------------- */
    if (led_green_active)
    {
      if ((HAL_GetTick() - led_green_start) >= LED_DURATION_MS)
      {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
        led_green_active = false;
        HAL_UART_Transmit(&huart1, (uint8_t*)msg_done, strlen(msg_done), HAL_MAX_DELAY);
      }
    }

    if (led_red_active)
    {
      if ((HAL_GetTick() - led_red_start) >= LED_DURATION_MS)
      {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
        led_red_active = false;
        HAL_UART_Transmit(&huart1, (uint8_t*)msg_done, strlen(msg_done), HAL_MAX_DELAY);
      }
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
