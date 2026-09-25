# Configuration & Implementation — lcd-gyro-bsp

| | |
|---|---|
| Task ID | `lcd-gyro-bsp` |
| Board | `STM32F429I-DISC1` |
| MCU | `STM32F429ZIT6U` |
| Date | 2026-09-24 |

---

## 1. Problem analysis

**Restated in one sentence:** Cập nhật liên tục 3 trục Gyroscope lên màn hình TFT LCD (thông qua thư viện BSP có sẵn) với giao diện mượt mà, kết hợp ngắt TIM3 làm heartbeat và UART1 xuất log.

### Requirements (Inputs, Outputs, Timing, State)

| Signal / Constraint / State | Details |
|---|---|
| **Inputs** | Dữ liệu góc 3 trục từ cảm biến L3GD20 qua SPI5 (PF7, PF8, PF9). |
| **Outputs** | Màn hình LCD 2.4" qua LTDC & FMC SDRAM. Chuỗi UART qua USART1 (PA9/PA10). |
| **Timing constraints** | TIM3 ngắt 1s (Hardware timebase). Vòng lặp chính cập nhật LCD mỗi 50ms (20Hz). |

### Acceptance criteria

- [x] LCD hiển thị giá trị Gyro liên tục, không bị giật/nhấp nháy (flickering).
- [x] Xử lý rác màn hình khi số liệu ngắn đi.
- [x] UART log và ngắt TIM3 hoạt động độc lập.

## 2. Peripheral selection

| Requirement | Peripheral | Processing model |
|---|---|---|
| TFT LCD 2.4" & SDRAM | LTDC + FMC | **Cấu hình tự động bởi BSP**. (Dùng DMA2D quét hình ảnh). |
| Gyroscope (L3GD20) | SPI5 | **Cấu hình tự động bởi BSP**. (Đọc/ghi qua `GYRO_IO_Write`). |
| Timebase | TIM3 | Interrupt (Ngắt chu kỳ 1s). |
| PC Log | USART1 | Polling hoặc Interrupt. |

## 3. Pin map (Tham khảo)

*Lưu ý: Các chân SPI5, LTDC, FMC sẽ được thư viện BSP tự động cấu hình bằng code, ta **không** thiết lập trong CubeMX.*

| Signal | Pin | Mode |
|---|---|---|
| SPI5 (SCK/MISO/MOSI) | PF7 / PF8 / PF9 | Alternate Function 5 (Do BSP config) |
| USART1 (TX/RX) | PA9 / PA10 | Alternate Function 7 (Do CubeMX config) |

## 4. Cấu hình STM32CubeMX (Step-by-step)

> ⚠️ **CẢNH BÁO QUAN TRỌNG:** Ở bước này, ta **KHÔNG** kích hoạt SPI5, LTDC, hay FMC. Việc kích hoạt chúng trong CubeMX sẽ sinh ra code đụng độ (Multiple Definition) với code của BSP. (Xem giải thích ở file `lcd-theory-and-concepts.md`).

Thực hiện lần lượt các bước sau trong STM32CubeMX:

```text
► 1. New project
     - File -> New -> STM32 Project -> "Board Selector" tab -> Chọn STM32F429I-DISC1 -> Next.
     - Project name: lcd-gyro-bsp
     - Trả lời "Initialize all peripherals with their default Mode?" -> CHỌN NO (Cực kỳ quan trọng).

► 2. System Core -> RCC
     - High Speed Clock (HSE): Chọn "BYPASS Clock Source" (mạch Discovery lấy xung từ ST-LINK).

► 3. System Core -> SYS
     - Debug: Serial Wire
     - Timebase Source: SysTick

► 4. Clock Configuration tab
     - Input frequency: Nhập 8 (MHz).
     - Đường dẫn HSE: Chọn HSE -> PLLCLK.
     - HCLK (MHz): Nhập 180 và nhấn Enter. CubeMX sẽ tự động giải quyết các hệ số M, N, P, Q.
     - (Lúc này APB1 Timer clock = 90 MHz, APB2 = 90 MHz).

► 5. Connectivity -> USART1
     - Mode: Asynchronous. 
     - Kiểm tra Pinout View: Lúc này PA9 và PA10 sẽ được tự động gán nhãn là **STLINK_RX** và **STLINK_TX** (hoặc VCP_TX/RX). Đừng lo lắng, đây chính là 2 chân của USART1 được nối trực tiếp vào mạch nạp ST-LINK trên board để giả lập cổng COM ảo. Bạn cứ giữ nguyên, không cần đổi tên.
     - Parameter Settings: Baud Rate = 115200, Word Length = 8, Parity = None, Stop Bits = 1.
     - NVIC Settings: Tích chọn "USART1 global interrupt" (nếu bạn dùng ngắt nhận).

► 6. Timers -> TIM3 (Tạo ngắt 1s)
     - Clock Source: Internal Clock
     - Parameter Settings:
          + Prescaler: 8999 (90MHz / 9000 = 10 kHz)
          + Counter Period: 9999 (10kHz / 10000 = 1s)
          + auto-reload preload: Enable
     - NVIC Settings: Tích chọn "TIM3 global interrupt".

► 6b. GPIO -> PA0 (Nút nhấn USER)
     - Trên Pinout View, click chuột trái vào chân PA0, chọn **GPIO_EXTI0**.
     - Trong thẻ GPIO bên trái: Bấm vào PA0, mục *GPIO mode* chọn **External Interrupt Mode with Rising edge trigger detection** (Hoặc Falling edge tùy ý).

► 7. System Core -> NVIC
     - Quan trọng: Ở dòng trên cùng **Priority Group**, hãy chọn **4 bits for pre-emption priority, 0 bits for subpriority**. (Nếu để mặc định 0 bits, bạn sẽ không thể chỉnh số ở cột Preemption).
     - Đặt Preemption Priority cho "EXTI line0 interrupt" = 3.
     - Đặt Preemption Priority cho "USART1 global interrupt" = 1.
     - Đặt Preemption Priority cho "TIM3 global interrupt" = 2.

► 8. Project Manager
     - Tab Project: Chọn Toolchain/IDE là "STM32CubeIDE".
     - Tab Code Generator: Tích chọn "Generate peripheral initialization as a pair of .c/.h files per peripheral".

► 9. Bấm GENERATE CODE (Alt+K)
```

## 5. Hướng dẫn Copy và Tích hợp thư viện BSP & HAL vào CubeIDE

Vì chúng ta "nhường" quyền khởi tạo phần cứng cho BSP, CubeMX sẽ mặc định bỏ qua không copy các file thư viện HAL tương ứng. Do đó, bạn cần làm thủ công bước này để trình biên dịch không báo lỗi thiếu file hoặc thiếu cấu trúc (như `SPI_HandleTypeDef`, `lcd.h`).

### 5.1. Tìm vị trí thư mục STM32Cube Repository trên máy bạn
Mặc định trên Windows, nó nằm ở: 
`C:\Users\<Tên_User>\STM32Cube\Repository\STM32Cube_FW_F4_V1.28.x` (phiên bản có thể khác tùy máy).

### 5.2. Tạo cấu trúc thư mục trong Project CubeIDE
Trong **Project Explorer**, click chuột phải vào Project của bạn (`lcd-gyro-bsp`), chọn **New -> Folder** để tạo cây thư mục như sau:
```text
lcd-gyro-bsp/
├── Drivers/
│   ├── BSP/
│   │   ├── STM32F429I-Discovery/    <-- (Tạo folder này)
│   │   ├── Components/
│   │   │   ├── ili9341/             <-- (Tạo folder này)
│   │   │   ├── Common/              <-- (Tạo folder này)
├── Utilities/
│   ├── Fonts/                       <-- (Tạo folder này)
```

### 5.3. Copy các file BSP từ Repository vào Project
Vào đường dẫn Repository ở bước 5.1, **Copy** các file sau thả vào các thư mục tương ứng trong CubeIDE:
1. Vô `Drivers\BSP\STM32F429I-Discovery\` copy vào `STM32F429I-Discovery/`:
   - `stm32f429i_discovery.c` và `.h`
   - `stm32f429i_discovery_lcd.c` và `.h`
   - `stm32f429i_discovery_sdram.c` và `.h`
2. Vô `Drivers\BSP\Components\ili9341\` copy vào `ili9341/`:
   - `ili9341.c` và `.h`
3. Vô `Drivers\BSP\Components\Common\` copy vào `Common/`:
   - `lcd.h` *(Cực kỳ quan trọng, tránh lỗi fatal error lcd.h)*
4. Vô `Utilities\Fonts\` copy vào `Fonts/`:
   - `font8.c`, `font12.c`, `font16.c`, `font20.c`, `font24.c` và `fonts.h`

### 5.4. Bổ sung các file HAL Driver bị thiếu
Vì CubeMX không tự sinh ra, bạn phải tự copy chúng từ Repository vào Project:
1. **Mở thư mục `Drivers\STM32F4xx_HAL_Driver\Src\` (trên máy)**, copy các file `.c` sau thả vào thư mục `Drivers/STM32F4xx_HAL_Driver/Src/` của Project:
   - `stm32f4xx_hal_spi.c`
   - `stm32f4xx_hal_i2c.c` và `stm32f4xx_hal_i2c_ex.c`
   - `stm32f4xx_hal_ltdc.c` và `stm32f4xx_hal_ltdc_ex.c`
   - `stm32f4xx_hal_sdram.c`
   - `stm32f4xx_ll_fmc.c`
   - `stm32f4xx_hal_dma2d.c`
2. **Mở thư mục `Drivers\STM32F4xx_HAL_Driver\Inc\` (trên máy)**, copy các file `.h` tương ứng thả vào thư mục `Drivers/STM32F4xx_HAL_Driver/Inc/` của Project:
   - `stm32f4xx_hal_spi.h`
   - `stm32f4xx_hal_i2c.h` và `stm32f4xx_hal_i2c_ex.h`
   - `stm32f4xx_hal_ltdc.h` và `stm32f4xx_hal_ltdc_ex.h`
   - `stm32f4xx_hal_sdram.h`
   - `stm32f4xx_ll_fmc.h`
   - `stm32f4xx_hal_dma2d.h`

### 5.5. Bật (Enable) các Module trong file cấu hình
Mở file `Core/Inc/stm32f4xx_hal_conf.h` trong Project, tìm và **bỏ dấu comment** (xóa `/*` và `*/`) cho 5 dòng sau để kích hoạt các thư viện vừa copy:
```c
#define HAL_DMA2D_MODULE_ENABLED
#define HAL_SDRAM_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_LTDC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
```

### 5.6. Trỏ đường dẫn Include trong CubeIDE (Rất quan trọng)
1. Chuột phải vào Project -> **Properties**.
2. Chọn **C/C++ Build** -> **Settings**.
3. Tab **Tool Settings**, dưới mục **MCU GCC Compiler**, chọn **Include paths**.
4. Bấm nút **Add...** -> **Workspace...** và thêm 3 đường dẫn sau:
   - `lcd-gyro-bsp/Drivers/BSP/STM32F429I-Discovery`
   - `lcd-gyro-bsp/Drivers/BSP/Components/ili9341`
   - `lcd-gyro-bsp/Utilities/Fonts`
5. Nhấn **Apply and Close**.

*(Đến đây Project của bạn đã sẵn sàng để gọi các hàm BSP trong `main.c`)*

## 6. Implementation plan

### Where each fragment goes (trong `main.c`)

| CubeMX marker | What you add |
|---|---|
| `/* USER CODE BEGIN Includes */` | Các file header `#include "stm32f429i_discovery_lcd.h"`, `stdio.h` |
| `/* USER CODE BEGIN 2 */` | Gọi `BSP_LCD_Init()`, `BSP_LCD_LayerDefaultInit()`, `HAL_TIM_Base_Start_IT()`. Gọi BSP GYRO Init. Vẽ Layout tĩnh. |
| `/* USER CODE BEGIN WHILE */` | Vòng lặp định kỳ: Dùng hàm `GYRO_IO_Read()` đọc góc, dùng `BSP_LCD_DisplayStringAt()` đè lên LCD. |
| `/* USER CODE BEGIN 4 */` | Override hàm `__io_putchar()` để dùng được lệnh `printf()` qua UART. |
