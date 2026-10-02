# Configuration & Implementation — Animated Pixel Bot

| | |
|---|---|
| Task ID | `lcd-pixel-bot` |
| Board | `STM32F429I-DISC1` |
| MCU | `STM32F429ZIT6U` |
| Date | 2026-10-02 |

---

## 0. Open items — confirm before building

| # | Item | Why it matters | How to confirm |
|---|---|---|---|
| 1 | HSE = BYPASS in CubeMX | Discovery lấy clock 8 MHz từ ST-LINK MCO; chọn Crystal sẽ treo cứng trong `Error_Handler()` | `[UM1670 §7.12.1 p.22]` |
| 2 | KHÔNG khởi tạo LCD/FMC/SPI5 trong CubeMX | Chọn "No" khi tạo project để tránh lỗi `Multiple Definition` giữa CubeMX và ST BSP | `[docs/tasks/lcd-init/01-cubemx-and-ide-setup.md]` |
| 3 | Cấu hình Layer 1 sang RGB565 | Mặc định BSP khởi tạo ARGB8888 (32-bit); Pixel Bot cần RGB565 (16-bit) để render 30 FPS mượt mà | `[stm32f429i_discovery_lcd.c L.248]` |
| 4 | Nút nhấn PA0 Active High | Nút B1 nối VDD qua switch và kéo xuống GND bằng trở 220k; cấu hình đúng Falling/Rising edge | `[UM1670 §7.6 p.20; MB1075 sh.6]` |

---

## 1. Problem analysis

**Restated in one sentence:** Xây dựng nhân vật Pixel Bot hoạt hình tương tác trên màn hình LCD 240×320 của STM32F429I-DISC1, sở hữu tập hợp nhiều biểu cảm và hành động phong phú, diễn ra liên tục, ngẫu nhiên, lặp vô tận, kết hợp phản hồi nút nhấn PA0 và điều khiển qua UART.

### Assumptions made

| Assumption | Why | Impact if wrong |
|---|---|---|
| Màn hình đặt hướng Portrait (240×320) | Đúng chuẩn hiển thị mặc định của panel ILI9341 trên kit | Nếu xoay Landscape (320×240) cần hoán đổi tọa độ X/Y |
| Double buffering đặt tại SDRAM `0xD0000000` & `0xD0025800` | 2 Framebuffer RGB565 chiếm 2×150 KB = 300 KB, RAM nội 256 KB không chứa nổi | Bắt buộc phải có SDRAM ngoài do FMC quản lý |
| Tốc độ render 30 FPS (chu kỳ 33 ms) | Đủ mượt cho hoạt hình pixel art, không gây nghẽn băng thông bus FMC/LTDC | Nếu dưới 15 FPS sẽ giật, trên 60 FPS thừa tải CPU |
| Chu kỳ hành động ngẫu nhiên: 2.0 s – 4.0 s | Bot có thời gian thở/chớp mắt tự nhiên giữa các hành động lớn | Tránh hiện tượng đổi chiêu liên tục gây rối mắt |

### Requirements (Inputs, Outputs, Timing, State)

| Signal / Constraint / State | Details |
|---|---|
| **Inputs** | Nút nhấn người dùng B1 tại chân **PA0** (GPIO EXTI0). Cổng giao tiếp nối tiếp **USART1** (PA9/PA10, 115200 bps) nhận lệnh điều khiển. |
| **Outputs** | Màn hình TFT LCD 2.4" QVGA (240×320) qua khối **LTDC** và bộ nhớ ngoài **FMC SDRAM** (IS42S16400J). Đèn LED xanh **PG13** (Heartbeat toggle khi render). Đèn LED đỏ **PG14** (Báo lỗi). Chuỗi log trạng thái qua **USART1**. |
| **Timing constraints** | Render loop: **33 ms** (~30 FPS). Chớp mắt tự nhiên (Blink): ngẫu nhiên mỗi **2.5 s – 5.5 s** (kèm 15% chớp đôi). Chuyển hành động tự do: sau mỗi **2.0 s – 4.0 s** Idle. |
| **State to maintain** | `anim_cur` (con trỏ animation hiện tại), `anim_idx` (frame hiện tại), `t_next_frame`, `t_next_blink`, `t_next_look`, `t_next_action`, `bot_mode` (IDLE, ACTION, OVERLAY). |

### Acceptance criteria

- [x] Nhân vật Pixel Bot hiển thị sắc nét với tỷ lệ ô `CELL = 12 px` (lưới 20×26 ô), thân màu tím công nghệ trên nền tối `#0D0B1E`.
- [x] Khi khởi động, bot chạy hoạt cảnh Boot/Wake-up (thức dậy, dụi mắt, vươn vai, vẫy tay chào) 1 lần duy nhất.
- [x] Trong trạng thái bình thường (IDLE), bot thở đều đặn, tự động chớp mắt ngẫu nhiên (Blink 3 frame) và thỉnh thoảng liếc nhìn trái/phải.
- [x] Các hành động đặc biệt (WAVE, JUMP, WALK, DANCE, LAUGH, LOVE, THINKING, SURPRISED, CONFUSED, SLEEP) diễn ra luân phiên, ngẫu nhiên, nối tiếp nhau liên tục mà không làm treo hay giật màn hình.
- [x] Không có hiện tượng rách hình (tearing) hay chớp nháy đen (flickering) nhờ cơ chế Double Buffering đồng bộ VSYNC.
- [x] Nhấn nút PA0 lập tức kích hoạt hành động phản xạ (Jump/Excited) và UART1 in log sự kiện tương ứng.

---

## 2. Peripheral selection

| Requirement | Peripheral | Processing model | Why this, not the alternative |
|---|---|---|---|
| Hiển thị đồ họa 240×320 | LTDC + FMC SDRAM + ILI9341 | DMA / Phần cứng quét trực tiếp từ SDRAM | **Khởi tạo thông qua ST BSP** (`BSP_LCD_Init()`). Tránh tự viết driver thanh ghi FMC/LTDC phức tạp; sau đó cấu hình lại Layer 1 sang RGB565. |
| Timebase & Scheduler | SysTick (`HAL_GetTick()`) | Non-blocking Polling | Đạt độ chính xác 1 ms, không tiêu tốn thêm Timer phần cứng (TIM3 có thể dành cho các tác vụ mở rộng). |
| Nút nhấn tương tác | GPIO PA0 | EXTI0 Interrupt | Bắt sự kiện nhấn phím tức thì với debounce phần mềm `150 ms`, không bỏ sót thao tác. |
| Debug & Command Shell | USART1 (PA9/PA10) | Asynchronous Interrupt / Polling | Kết nối trực tiếp vào mạch nạp ST-LINK/V2-B để giả lập cổng COM ảo (VCP), cho phép gửi phím điều khiển bot từ PC. |
| Bộ sinh số ngẫu nhiên | LFSR (Linear-Feedback Shift Register) / LCG 32-bit | CPU Software Algorithm | Nhẹ, không cần bật thêm ngoại vi RNG, chu kỳ lặp > 4 tỷ mẫu, đủ ngẫu nhiên cho hành vi bot. |

**Escalation justification:**
- Việc dùng bộ đôi Framebuffer trên SDRAM kết hợp nạp địa chỉ lúc VBLANK (`LTDC_SRCR_VBR`) là bắt buộc để triệt tiêu hoàn toàn hiện tượng rách hình (tearing) khi vẽ chuyển động liên tục ở 30 FPS.
- Toàn bộ render được xử lý theo mô hình Pose/Part (ghép ô) thay vì giải nén ảnh GIF/BMP, tiết kiệm 95% Flash và RAM.

---

## 3. Pin map

| Signal | Pin | Mode | Pull | Speed / AF | Active level | Source |
|---|---|---|---|---|---|---|
| `USER_BUTTON` (B1) | **PA0** | `GPIO_EXTI0` | None / Down | — | High = Pressed | `[UM1670 §7.6 p.20; MB1075 sh.6]` |
| `USART1_TX` (VCP) | **PA9** | Alternate Function | Pull-up | Very High / AF7 | High | `[UM1670 §7.3.3 p.17; DS Table 12 p.73]` |
| `USART1_RX` (VCP) | **PA10** | Alternate Function | Pull-up | Very High / AF7 | High | `[UM1670 §7.3.3 p.17; DS Table 12 p.73]` |
| `LED_GREEN` (LD3) | **PG13** | Output Push-Pull | No pull | Low | High = On | `[UM1670 §7.5 p.20; MB1075 sh.6]` |
| `LED_RED` (LD4) | **PG14** | Output Push-Pull | No pull | Low | High = On | `[UM1670 §7.5 p.20; MB1075 sh.6]` |
| `LCD & SDRAM Bus` | Multi (Ports B, C, D, E, F, G) | Alternate Function | — | — | — | Do thư viện ST BSP tự khởi tạo trong code |

### Pin availability & Conflicts
- Chân `PA13` (SWDIO), `PA14` (SWCLK), `PB3` (SWO) giữ nguyên cho mạch nạp ST-LINK.
- Các chân thuộc bus dữ liệu SDRAM (FMC: `FMC_D0..D15`, `FMC_A0..A11`, `FMC_BA0..1`, `SDCLK`, `SDNWE`...) và bus LTDC (`LCD_R2..R7`, `LCD_G2..G7`, `LCD_B2..B7`, `CLK`, `HSYNC`, `VSYNC`, `DE`) tuyệt đối **KHÔNG** gán vào bất kỳ GPIO tự do nào trong CubeMX.

---

## 4. Clock and timing calculations

### System clock (180 MHz Max Performance)

```
HSE (8 MHz Bypass từ ST-LINK MCO) 
  → PLLM = /8  → 1 MHz VCO In
  → PLLN = ×360 → 360 MHz VCO Out
  → PLLP = /2  → SYSCLK = 180 MHz
  → AHB  = /1  → HCLK   = 180 MHz
  → APB1 = /4  → PCLK1  = 45 MHz (Timer clk = 90 MHz)
  → APB2 = /2  → PCLK2  = 90 MHz (Timer clk = 180 MHz)
```

### Display & Animation Timing

| Đại lượng | Công thức / Tính toán | Giá trị thực tế |
|---|---|---|
| **Kích thước 1 Framebuffer RGB565** | $240 \times 320 \times 2\text{ bytes}$ | $153,600\text{ bytes} = \text{0x25800}$ |
| **Địa chỉ FB0 (Front)** | Bộ nhớ ngoài SDRAM Bank 2 | `0xD0000000` |
| **Địa chỉ FB1 (Back)** | $\text{0xD0000000} + \text{0x25800}$ | `0xD0025800` |
| **Thời gian quét 1 Frame (30 FPS)** | $T = 1000\text{ ms} / 30$ | $\approx 33\text{ ms}$ |
| **USART1 Baud Divisor** | $90,000,000 / (16 \times 115200)$ | $48.8281 \rightarrow \text{Baud } 115200\text{ bps}$ |

---

## 5. STM32CubeMX Configuration (Click-by-Click Checklist)

Thực hiện chính xác theo thứ tự sau trong STM32CubeMX:

```text
□ 1. New Project & Setup
     - File -> New -> STM32 Project -> Chọn tab "Board Selector".
     - Tìm kiếm và chọn: STM32F429I-DISC1 -> Bấm Next.
     - Project Name: stm32-lcd-pixel-bot.
     - ⚠️ BẢNG THÔNG BÁO QUAN TRỌNG: "Initialize all peripherals with their default Mode?"
       -> BẮT BUỘC CHỌN "NO". 
       (Lý do: Tránh để CubeMX sinh code đụng độ MspInit với ST BSP).

□ 2. System Core -> RCC
     - High Speed Clock (HSE): Chọn "BYPASS Clock Source".

□ 3. System Core -> SYS
     - Debug: Chọn "Serial Wire" (giữ chân PA13/PA14).
     - Timebase Source: Chọn "SysTick".

□ 4. Clock Configuration tab
     - Input frequency: Nhập 8 (MHz).
     - PLL Source Mux: Chọn HSE.
     - Tại ô HCLK (MHz): Nhập 180 và nhấn phím Enter.
     - CubeMX tự động tính: PLLM=8, PLLN=360, PLLP=2, APB1 Prescaler=/4, APB2 Prescaler=/2.

□ 5. Connectivity -> USART1
     - Mode: Chọn "Asynchronous".
     - Parameter Settings: Baud Rate = 115200, Word Length = 8 Bits, Parity = None, Stop Bits = 1.
     - NVIC Settings: Tích chọn "USART1 global interrupt".

□ 6. GPIO Configuration (Pinout View)
     - Click chân PA0: Chọn GPIO_EXTI0.
     - Click chân PG13: Chọn GPIO_Output (Label: LED_GREEN).
     - Click chân PG14: Chọn GPIO_Output (Label: LED_RED).
     - Trong System Core -> GPIO:
       + PA0: GPIO Mode = "External Interrupt Mode with Rising edge trigger detection", Pull = "No pull-up and no pull-down".
       + PG13 & PG14: GPIO Mode = "Output Push Pull", Pull = "No pull", Speed = "Low".

□ 7. System Core -> NVIC
     - Priority Group: Chọn "4 bits for pre-emption priority, 0 bits for subpriority".
     - EXTI line0 interrupt: Preemption Priority = 3, tích chọn Enable.
     - USART1 global interrupt: Preemption Priority = 2, tích chọn Enable.

□ 8. Project Manager
     - Tab Project: Toolchain / IDE chọn "STM32CubeIDE".
     - Tab Code Generator: Chọn "Copy only the necessary library files".
       Tích chọn "Generate peripheral initialization as a pair of '.c/.h' files per peripheral".

□ 9. Generate Code
     - Nhấn phím Alt + K hoặc menu Project -> Generate Code.
```

---

## 5b. Hướng Dẫn Tích Hợp Thư Viện ST BSP Vào STM32CubeIDE (SOP)

Sau khi CubeMX sinh code xong, mở Project trong **STM32CubeIDE** và thực hiện quy chuẩn sau (dựa trên `docs/tasks/lcd-init/01-cubemx-and-ide-setup.md`):

### 1. Tạo cấu trúc thư mục trong Project Explorer
Chuột phải vào Project -> **New -> Folder**:
```text
stm32-lcd-pixel-bot/
├── Drivers/
│   ├── BSP/
│   │   ├── STM32F429I-Discovery/
│   │   ├── Components/
│   │   │   ├── ili9341/
│   │   │   ├── Common/
├── Utilities/
│   ├── Fonts/
```

### 2. Copy file từ STM32Cube Repository
Vào thư mục: `C:\Users\<Tên_User>\STM32Cube\Repository\STM32Cube_FW_F4_V1.28.x\` (hoặc copy trực tiếp từ `workspace_0.0.1/lcd-gyro-bsp`):
1. Thả vào `Drivers/BSP/STM32F429I-Discovery/`:
   - `stm32f429i_discovery.c`, `stm32f429i_discovery.h`
   - `stm32f429i_discovery_lcd.c`, `stm32f429i_discovery_lcd.h`
   - `stm32f429i_discovery_sdram.c`, `stm32f429i_discovery_sdram.h`
2. Thả vào `Drivers/BSP/Components/ili9341/`:
   - `ili9341.c`, `ili9341.h`
3. Thả vào `Drivers/BSP/Components/Common/`:
   - `lcd.h` *(Bắt buộc, không có sẽ báo fatal error)*
4. Thả vào `Utilities/Fonts/`:
   - `font8.c`, `font12.c`, `font16.c`, `font20.c`, `font24.c`, `fonts.h`

### 3. Copy các file HAL Driver bị thiếu
1. Thả vào `Drivers/STM32F4xx_HAL_Driver/Src/`:
   - `stm32f4xx_hal_spi.c`
   - `stm32f4xx_hal_i2c.c`, `stm32f4xx_hal_i2c_ex.c`
   - `stm32f4xx_hal_ltdc.c`, `stm32f4xx_hal_ltdc_ex.c`
   - `stm32f4xx_hal_sdram.c`
   - `stm32f4xx_ll_fmc.c`
   - `stm32f4xx_hal_dma2d.c`
2. Thả vào `Drivers/STM32F4xx_HAL_Driver/Inc/`:
   - Các file `.h` tương ứng với các file `.c` trên.

### 4. Kích hoạt Macro trong `Core/Inc/stm32f4xx_hal_conf.h`
Mở file `stm32f4xx_hal_conf.h`, bỏ comment 5 dòng sau:
```c
#define HAL_DMA2D_MODULE_ENABLED
#define HAL_SDRAM_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_LTDC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
```

### 5. Cài đặt Include Paths trong CubeIDE
1. Chuột phải vào Project -> **Properties** -> **C/C++ Build** -> **Settings**.
2. Dưới mục **MCU GCC Compiler** -> chọn **Include paths**.
3. Bấm nút **Add... (Màu xanh)** -> **Workspace...** và thêm 3 đường dẫn:
   - `/${ProjName}/Drivers/BSP/STM32F429I-Discovery`
   - `/${ProjName}/Drivers/BSP/Components/ili9341`
   - `/${ProjName}/Utilities/Fonts`
4. Bấm **Apply and Close**.

---

## 6. Implementation plan

### Program structure

```
+--------------------------------------------------------------------------+
|                               main()                                     |
|  1. HAL_Init() & SystemClock_Config()                                    |
|  2. MX_GPIO_Init(), MX_USART1_UART_Init()                                |
|  3. LCD_Init_RGB565(): Gọi BSP_LCD_Init(), config Layer sang RGB565      |
|  4. anim_play(&ANIM_BOOT): Phát hoạt cảnh khởi động                      |
+--------------------------------------------------------------------------+
                                    |
                                    v
+--------------------------------------------------------------------------+
|                           while (1) Loop                                 |
|  [Đoạn 1: Scheduler 33ms (~30 FPS)]                                      |
|    - Đợi LTDC hoàn tất reload VSYNC (LTDC->SRCR & VBR == 0)              |
|    - Cập nhật frame hiện tại: Pose p = *anim_tick(now)                   |
|    - Chồng lớp tự nhiên: overlay_blink(&p, now), overlay_look(&p, now)   |
|    - Render ra Backbuffer: render(back, &p)                              |
|    - Tráo trang (Present): HAL_LTDC_SetAddress_NoReload + LTDC_SRCR_VBR  |
|    - Toggle PG13 (Heartbeat)                                             |
|                                                                          |
|  [Đoạn 2: Random Action Selector]                                        |
|    - Khi đang ở IDLE và đã hết chu kỳ nghỉ ngẫu nhiên (2-4s):            |
|      -> Sinh số ngẫu nhiên chọn 1 trong 10 actions                       |
|      -> Gọi anim_play(action_random)                                     |
|                                                                          |
|  [Đoạn 3: Xử lý sự kiện tức thời]                                        |
|    - Nếu cờ nút nhấn PA0 (flag_btn) bật -> kích hoạt hành động JUMP      |
|    - Nếu nhận ký tự UART -> chuyển animation tương ứng                   |
+--------------------------------------------------------------------------+
```

### Where each fragment goes in `main.c`

| CubeMX marker | What you add |
|---|---|
| `/* USER CODE BEGIN Includes */` | `#include "stm32f429i_discovery_lcd.h"`, `#include <stdio.h>`, `#include <stdbool.h>` |
| `/* USER CODE BEGIN PTD */` | Các kiểu cấu trúc: `Bmp`, `Pose`, `Anim`, `EyeId`, `MouthId`, `FxId` |
| `/* USER CODE BEGIN PD */` | Tọa độ màn hình, Palette RGB565, Địa chỉ Framebuffer `FB0_ADDR` và `FB1_ADDR` |
| `/* USER CODE BEGIN PV */` | Con trỏ `front`, `back`, bảng Pose các Animation (BOOT, IDLE, WAVE, JUMP...), biến cờ ngắt `flag_btn` |
| `/* USER CODE BEGIN 0 */` | Bộ hàm đồ họa pixel (`cell_fill`, `render`, `eye_draw`), engine animation (`anim_play`, `anim_tick`), bộ sinh số ngẫu nhiên |
| `/* USER CODE BEGIN 2 */` | Gọi `LCD_Init_RGB565(FB0_ADDR)`, khởi động UART RX interrupt, kích hoạt `ANIM_BOOT` |
| `/* USER CODE BEGIN WHILE */` | Vòng lặp 30 FPS, Present VSYNC, máy trạng thái chuyển hành động ngẫu nhiên |
| `/* USER CODE BEGIN 4 */` | Callbacks: `HAL_GPIO_EXTI_Callback()` cho PA0, `HAL_UART_RxCpltCallback()` cho USART1 |

---

## 7. Bring-up and verification

### Incremental stages

| Stage | What to build | What you should observe | If it fails, suspect |
|---|---|---|---|
| 1 | Dự án rỗng sau khi copy BSP | Biên dịch 0 Error, 0 Warning | Thiếu include path hoặc chưa bật Macro trong `stm32f4xx_hal_conf.h` |
| 2 | Khởi tạo LCD cơ bản | Màn hình sáng đèn nền, xóa màn hình toàn màu đen | Lỗi cáp màn hình, chưa cấu hình HSE BYPASS |
| 3 | Khởi tạo Layer RGB565 | Nền chuyển sang tím đậm (`0x0843`), hiển thị nhân vật tĩnh (NEUTRAL) | Sai thông số cấu hình Layer hoặc nhầm địa chỉ SDRAM |
| 4 | Kích hoạt Animation Loop | Nhân vật chớp mắt, thở phập phồng, cử động mượt mà | Sai logic `anim_tick()` hoặc vòng lặp bị `HAL_Delay` chặn |
| 5 | Hoạt cảnh ngẫu nhiên & Nút nhấn | Sau mỗi vài giây bot đổi chiêu (Nhảy, Vẫy tay, Đi bộ...). Nhấn PA0 bot nhảy | Chưa bật ngắt EXTI0 hoặc thiếu seed ngẫu nhiên |

### Failure decision tree for this task

```
Màn hình LCD trắng xóa hoặc đen ngòm?
 ├── Có dừng trong Error_Handler() khi chạy debug không?
 │    ├── CÓ -> Kiểm tra RCC HSE: Đã chọn "BYPASS Clock Source" chưa? (Bắt buộc).
 │    └── KHÔNG -> Kiểm tra hàm BSP_LCD_Init() đã được gọi chưa.
 └── Màu sắc bị loang lỗ, sai lệch hàng pixel?
      └── Kiểm tra Layer PixelFormat: Đã đặt LTDC_PIXEL_FORMAT_RGB565 chưa?
          Nếu vẫn để ARGB8888 mặc định của BSP, việc ghi 16-bit sẽ làm vỡ ảnh.
```
