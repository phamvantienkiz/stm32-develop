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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Cac thanh ghi cho Gyro
#define L3GD20_CTRL_REG1_ADDR 0x20
#define L3GD20_OUT_X_L_ADDR   0x28

// Thoi gian lam moi man hinh
#define LCD_REFRESH_MS 50
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t uptime_seconds = 0;
uint32_t last_lcd_update = 0;

volatile uint8_t current_screen = 0; // 0 = Text, 1 = Bubble Level
volatile uint8_t screen_changed = 0;
int16_t old_bubble_x = 120;
int16_t old_bubble_y = 160;

// Cac handle ngoai vi
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// dinh nghia ham IO cua BSP Gyro trong stm32f429i_discovery.c
extern void    GYRO_IO_Init(void);
extern void    GYRO_IO_Write(uint8_t *pBuffer, uint8_t WriteAddr, uint16_t NumByteToWrite);
extern void    GYRO_IO_Read(uint8_t *pBuffer, uint8_t ReadAddr, uint16_t NumByteToRead);

void L3GD20_Init(void);
void L3GD20_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);
void UI_DrawStaticLayout(void);
void UI_DrawBubbleLayout(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * Khoi tao cam bie L3GD20 bang cac ham BSP IO
  *
  */
void L3GD20_Init(void) {
	// 1. Khoi tao bus SPI5 va chan CS (PC1) thong qua BSP
	GYRO_IO_Init();

	// 2. Ghi thanh ghi CTRL_REG1 (bat nguon va cac truc X, Y, Z)
	uint8_t ctrl1 = 0x0F; // PD=1, Xen=1, Yen=1, Zen=1
	GYRO_IO_Write(&ctrl1, L3GD20_CTRL_REG1_ADDR, 1);
}

/**
  * Doc 3 truc tu cam bien L3GD20
  */
void L3GD20_ReadXYZ(int16_t *x, int16_t *y, int16_t *z) {
	uint8_t buffer[6];

	// Doc 6 byte lien tiep bat dau tu OUT_X_L.
	// Tu OR bit Doc (0x80) va Doc nhieu (0x40) = 0xC0 */
	GYRO_IO_Read(buffer, (L3GD20_OUT_X_L_ADDR | 0xC0), 6);

	*x = (int16_t) ((buffer[1] << 8) | buffer[0]);
	*y = (int16_t) ((buffer[3] << 8) | buffer[2]);
	*z = (int16_t) ((buffer[5] << 8) | buffer[4]);
}

/**
  * Ve khung giao dien tinh (chi ve 1 lan luc startup)
  */
void UI_DrawStaticLayout(void) {
	BSP_LCD_Clear(LCD_COLOR_BLACK);

	BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_SetFont(&Font20);

	BSP_LCD_DisplayStringAt(0, 10, (uint8_t*) "STM32 GYROSCOPE", CENTER_MODE);

	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_DrawHLine(10, 35, 220);

	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_LIGHTBLUE);
	BSP_LCD_DisplayStringAt(10, 60, (uint8_t*) "AXIS X:", LEFT_MODE);
	BSP_LCD_DisplayStringAt(10, 100, (uint8_t*) "AXIS Y:", LEFT_MODE);
	BSP_LCD_DisplayStringAt(10, 140, (uint8_t*) "AXIS Z:", LEFT_MODE);

	BSP_LCD_DrawHLine(10, 180, 220);
	BSP_LCD_DisplayStringAt(10, 200, (uint8_t*) "UPTIME:", LEFT_MODE);
}

/*
  * Ve giao dien bot nuoc (Bubble Level)
  */
void UI_DrawBubbleLayout(void) {
	BSP_LCD_Clear(LCD_COLOR_BLACK);

	BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
	BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
	BSP_LCD_SetFont(&Font20);
	BSP_LCD_DisplayStringAt(0, 10, (uint8_t*)"BUBBLE LEVEL", CENTER_MODE);

	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	// Ve vong tron tam
	BSP_LCD_DrawCircle(120, 160, 80);
	BSP_LCD_DrawCircle(120, 160, 20);

	// Ve truc chu thap
	BSP_LCD_DrawVLine(120, 50, 220);
	BSP_LCD_DrawHLine(20, 160, 200);
}

/**
  * Ngat Timer 1s (Hardware timebase)
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM3) {
		uptime_seconds++;
	}
}

/**
  * Ngat nut nhan USER (PA0)
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_0) {
		current_screen = !current_screen; // Toggle screen
		screen_changed = 1;
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
  /* -------------------------------------------------------------
       Khởi tạo hiển thị LCD và SDRAM thông qua bộ BSP
       Lưu ý: Không khởi tạo SPI5, LTDC, FMC bằng CubeMX ở trên.
       Hàm BSP_LCD_Init() sẽ tự động gọi các HAL_..._MspInit trong stm32f429i_discovery.c
       ------------------------------------------------------------- */
  BSP_LCD_Init();

    // Bat Layer 1 (Background) va tro bo dem toi SDRAM
    BSP_LCD_LayerDefaultInit(1, 0xD0000000); // 0xD0000000 = LCD_FRAME_BUFFER
    BSP_LCD_SelectLayer(1);

    // Ve giao dien tinh
    UI_DrawStaticLayout();

    // Khoi tao cam bien Gyro
    L3GD20_Init();

    // bat Timer 1s
    HAL_TIM_Base_Start_IT(&htim3);

    printf("System initialized. LCD & Gyro ready.\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	char str_buf[64];
	float bubble_xf = 120.0f;
	float bubble_yf = 160.0f;

  while (1)
  {
		// Cap nhat LCD o tan so dinh truoc
		if ((HAL_GetTick() - last_lcd_update) >= LCD_REFRESH_MS) {
			last_lcd_update = HAL_GetTick();

			// Kiem tra neu co su kien bam nut chuyen man hinh
			if (screen_changed) {
				screen_changed = 0;
				if (current_screen == 0) {
					UI_DrawStaticLayout();
				} else {
					UI_DrawBubbleLayout();
					// Reset bot nuoc ve giua
					bubble_xf = 120.0f;
					bubble_yf = 160.0f;
					old_bubble_x = 120;
					old_bubble_y = 160;
				}
			}

			int16_t gx, gy, gz;
			L3GD20_ReadXYZ(&gx, &gy, &gz);

			if (current_screen == 0) {
				// ==============================
				// MAN HINH 0: TEXT GYROSCOPE
				// ==============================
				BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
				BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
				BSP_LCD_SetFont(&Font16);

				sprintf(str_buf, "%6d dps", gx);
				BSP_LCD_DisplayStringAt(90, 60, (uint8_t*) str_buf, LEFT_MODE);

				sprintf(str_buf, "%6d dps", gy);
				BSP_LCD_DisplayStringAt(90, 100, (uint8_t*) str_buf, LEFT_MODE);

				sprintf(str_buf, "%6d dps", gz);
				BSP_LCD_DisplayStringAt(90, 140, (uint8_t*) str_buf, LEFT_MODE);

				BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
				sprintf(str_buf, "%6lu s", uptime_seconds);
				BSP_LCD_DisplayStringAt(90, 200, (uint8_t*) str_buf, LEFT_MODE);

			} else if (current_screen == 1) {
				// ==============================
				// MAN HINH 1: BUBBLE LEVEL
				// ==============================
				// 1. Xoa bot nuoc cu bang cach ve hinh tron mau den len toa do cu
				BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
				BSP_LCD_FillCircle(old_bubble_x, old_bubble_y, 10);

				// Ve lai truc chu thap o cho bot nuoc cu vua xoa (tranh bi dut net)
				BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
				BSP_LCD_DrawVLine(120, 50, 220);
				BSP_LCD_DrawHLine(20, 160, 200);
				BSP_LCD_DrawCircle(120, 160, 80);
				BSP_LCD_DrawCircle(120, 160, 20);

				// 2. Tinh toan toa do moi (dung gia tri van toc goc nhu luc day, tu tu quay ve tam)
				// Scale gy -> X, gx -> Y. Giam scale de bot nhay
				bubble_xf += (gy / 400.0f);
				bubble_yf += (gx / 400.0f);

				// Decay (Gia lap luc can ve tam khi ngung quay)
				bubble_xf = bubble_xf * 0.95f + 120.0f * 0.05f;
				bubble_yf = bubble_yf * 0.95f + 160.0f * 0.05f;

				// Gioi han khong cho bot nuoc chay ra khoi man hinh
				if (bubble_xf < 15)
					bubble_xf = 15;
				if (bubble_xf > 225)
					bubble_xf = 225;
				if (bubble_yf < 40)
					bubble_yf = 40;
				if (bubble_yf > 280)
					bubble_yf = 280;

				int16_t new_x = (int16_t) bubble_xf;
				int16_t new_y = (int16_t) bubble_yf;

				// 3. Ve bot nuoc o vi tri moi
				BSP_LCD_SetTextColor(LCD_COLOR_RED);
				BSP_LCD_FillCircle(new_x, new_y, 10);

				// 4. Luu lai toa do de xoa o vong lap sau
				old_bubble_x = new_x;
				old_bubble_y = new_y;
			}

			// Log ra UART
			printf("X:%d Y:%d Z:%d\r\n", gx, gy, gz);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE {
	HAL_UART_Transmit(&huart1, (uint8_t*) &ch, 1, HAL_MAX_DELAY);
	return ch;
}
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
