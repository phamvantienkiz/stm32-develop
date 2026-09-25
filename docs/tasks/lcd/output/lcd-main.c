/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Reference implementation for LCD + Gyroscope using BSP
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

/* BẮT BUỘC: Thêm các header của BSP. 
   Lưu ý: Bạn phải copy các file thư viện này từ gói Firmware STM32CubeF4
   (Drivers/BSP/STM32F429I-Discovery) vào thư mục dự án của bạn. */
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* L3GD20 Registers */
#define L3GD20_CTRL_REG1_ADDR 0x20
#define L3GD20_OUT_X_L_ADDR   0x28

/* Khoảng thời gian làm mới màn hình (50ms = 20Hz) */
#define LCD_REFRESH_MS        50
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
volatile uint32_t uptime_seconds = 0;
uint32_t last_lcd_update = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Nguyên mẫu hàm IO của BSP Gyro (được định nghĩa trong stm32f429i_discovery.c) */
extern void    GYRO_IO_Init(void);
extern void    GYRO_IO_Write(uint8_t *pBuffer, uint8_t WriteAddr, uint16_t NumByteToWrite);
extern void    GYRO_IO_Read(uint8_t *pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead);

void L3GD20_Init(void);
void L3GD20_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);
void UI_DrawStaticLayout(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief Khởi tạo cảm biến L3GD20 bằng các hàm BSP IO
  */
void L3GD20_Init(void)
{
  /* 1. Khởi tạo bus SPI5 và chân CS (PC1) thông qua BSP */
  GYRO_IO_Init();
  
  /* 2. Ghi thanh ghi CTRL_REG1 (Bật nguồn, bật trục X, Y, Z) */
  uint8_t ctrl1 = 0x0F; // PD=1, Xen=1, Yen=1, Zen=1
  GYRO_IO_Write(&ctrl1, L3GD20_CTRL_REG1_ADDR, 1);
}

/**
  * @brief Đọc 3 trục từ cảm biến L3GD20
  */
void L3GD20_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
  uint8_t buffer[6];
  
  /* Đọc 6 byte liên tiếp bắt đầu từ OUT_X_L. 
     BSP đã bọc sẵn Bit 0x80 (Read) và Bit 0x40 (Auto-increment) bên trong hàm GYRO_IO_Read nếu cần, 
     hoặc ta tự thêm vào địa chỉ. Trong hàm của ST, cờ thường được tự động thêm. */
  
  /* Tuy nhiên, an toàn nhất là tự OR bit Đọc (0x80) và Đọc nhiều (0x40) = 0xC0 */
  GYRO_IO_Read(buffer, (L3GD20_OUT_X_L_ADDR | 0xC0), 6);
  
  *x = (int16_t)((buffer[1] << 8) | buffer[0]);
  *y = (int16_t)((buffer[3] << 8) | buffer[2]);
  *z = (int16_t)((buffer[5] << 8) | buffer[4]);
}

/**
  * @brief Vẽ khung giao diện tĩnh (Chỉ vẽ 1 lần lúc startup)
  */
void UI_DrawStaticLayout(void)
{
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  
  BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
  BSP_LCD_SetFont(&Font20);
  
  BSP_LCD_DisplayStringAt(0, 10, (uint8_t*)"STM32 GYROSCOPE", CENTER_MODE);
  
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DrawHLine(10, 35, 220);
  
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
  BSP_LCD_DisplayStringAt(10, 60, (uint8_t*)"AXIS X:", LEFT_MODE);
  BSP_LCD_DisplayStringAt(10, 100, (uint8_t*)"AXIS Y:", LEFT_MODE);
  BSP_LCD_DisplayStringAt(10, 140, (uint8_t*)"AXIS Z:", LEFT_MODE);
  
  BSP_LCD_DrawHLine(10, 180, 220);
  BSP_LCD_DisplayStringAt(10, 200, (uint8_t*)"UPTIME:", LEFT_MODE);
}

/**
  * @brief Ngắt Timer 1s (Hardware timebase)
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    uptime_seconds++;
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
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  
  /* USER CODE BEGIN 2 */
  
  /* -------------------------------------------------------------
     Khởi tạo hiển thị LCD và SDRAM thông qua bộ BSP
     Lưu ý: Không khởi tạo SPI5, LTDC, FMC bằng CubeMX ở trên. 
     Hàm BSP_LCD_Init() sẽ tự động gọi các HAL_..._MspInit trong stm32f429i_discovery.c
     ------------------------------------------------------------- */
  BSP_LCD_Init();
  
  /* Bật Layer 1 (Background) và trỏ bộ đệm tới SDRAM */
  BSP_LCD_LayerDefaultInit(1, 0xD0000000); // 0xD0000000 = LCD_FRAME_BUFFER
  BSP_LCD_SelectLayer(1);
  
  /* Vẽ giao diện tĩnh */
  UI_DrawStaticLayout();
  
  /* Khởi tạo cảm biến Gyro */
  L3GD20_Init();
  
  /* Bật Timer 1s */
  HAL_TIM_Base_Start_IT(&htim3);
  
  printf("System initialized. LCD & Gyro ready.\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char str_buf[64];
  
  while (1)
  {
    /* Cập nhật LCD ở tần số định trước (Tránh nhấp nháy màn hình) */
    if ((HAL_GetTick() - last_lcd_update) >= LCD_REFRESH_MS)
    {
      last_lcd_update = HAL_GetTick();
      
      int16_t gx, gy, gz;
      L3GD20_ReadXYZ(&gx, &gy, &gz);
      
      /* Chuẩn bị text màu xanh lá, nền đen để in đè số liệu */
      BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
      BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
      BSP_LCD_SetFont(&Font16);
      
      /* CỰC KỲ QUAN TRỌNG: Dùng format "%6d" để đệm khoảng trắng (padding)
         Cách này giúp các ký tự số bị ngắn đi (ví dụ từ -100 xuống 99) 
         sẽ bị khoảng trắng đè lên và xóa đi, chống rác màn hình mà 
         không cần phải gọi BSP_LCD_Clear(). */
      sprintf(str_buf, "%6d dps", gx);
      BSP_LCD_DisplayStringAt(90, 60, (uint8_t*)str_buf, LEFT_MODE);
      
      sprintf(str_buf, "%6d dps", gy);
      BSP_LCD_DisplayStringAt(90, 100, (uint8_t*)str_buf, LEFT_MODE);
      
      sprintf(str_buf, "%6d dps", gz);
      BSP_LCD_DisplayStringAt(90, 140, (uint8_t*)str_buf, LEFT_MODE);
      
      /* Cập nhật thời gian Uptime */
      BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
      sprintf(str_buf, "%6lu s", uptime_seconds);
      BSP_LCD_DisplayStringAt(90, 200, (uint8_t*)str_buf, LEFT_MODE);
      
      /* Log ra UART */
      printf("X:%d Y:%d Z:%d\r\n", gx, gy, gz);
    }
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/* Lưu ý: Để printf chạy được qua UART1, hãy đảm bảo bạn đã override hàm __io_putchar() 
   giống như ở các bài học trước, hoặc thêm hàm fputc() ở cuối file. */
/* USER CODE BEGIN 4 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END 4 */
