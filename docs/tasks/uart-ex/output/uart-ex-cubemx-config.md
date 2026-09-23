# Configuration & Implementation — UART Echo & Command Parsing

> Required structure for the primary configuration deliverable.

| | |
|---|---|
| Task ID | `uart-ex` |
| Board | STMicroelectronics STM32F429I-DISC1 (MB1075) |
| MCU | STM32F429ZIT6 (Arm® Cortex®-M4F, LQFP144) |
| Date | 2026-09-21 |

---

## 0. Open items — confirm before building

| # | Item | Why it matters | How to confirm | Status |
|---|---|---|---|---|
| 1 | USART1 VCP connection | Cần xác nhận PA9/PA10 có được nối với ST-LINK VCP không | MB1075 Sheet 2, SB11 & SB15 | **CONFIRMED**: Có (mặc định SB11, SB15 ON) |
| 2 | LED_RED (LD4) pin | Cần gán đúng chân để nháy đèn đỏ | UM1670 §7.5 p.20 | **CONFIRMED**: PG14, Active High |

## 1. Problem analysis

**Restated in one sentence:** Ứng dụng nhận từng ký tự từ máy tính qua UART (ngắt), khi gặp `\r` hoặc `\n` thì echo lại toàn bộ chuỗi; đồng thời phân tích lệnh `"LED_GREEN"` / `"LED_RED"` để bật đèn tương ứng trong 5 giây (non-blocking) rồi báo `"DONE\r\n"`.

### Assumptions made

| Assumption | Why | Impact if wrong |
|---|---|---|
| Sử dụng HSI 16 MHz làm nguồn Clock | Tránh lỗi cấu hình PLL phức tạp hoặc cấu hình sai HSE Bypass | Tốc độ CPU chậm hơn (16 MHz thay vì 180 MHz), nhưng với 115200 baud UART thì sai số baud rate vẫn nằm trong chuẩn an toàn (< 1%) |
| Buffer nhận (Line Buffer) là 128 bytes | Các lệnh điều khiển ("LED_GREEN") rất ngắn | Chuỗi quá 128 ký tự sẽ bị ngắt bỏ phần đuôi |
| Bỏ qua các ký tự rác nếu Main chưa xử lý xong chuỗi cũ | Tránh ghi đè buffer khi lệnh trước đang được xử lý | Mất dữ liệu nếu gõ quá nhanh liên tục nhiều dòng lệnh (cần Ring Buffer để giải quyết triệt để, nhưng Line Buffer + flag là đủ và dễ hiểu cho bài cơ bản) |

### Requirements (Inputs, Outputs, Timing, State)

| Signal / Constraint / State | Details |
|---|---|
| **Inputs** | UART Rx (PA10) nhận dữ liệu 115200 8N1 |
| **Outputs** | UART Tx (PA9) gửi phản hồi; LED Green (PG13), LED Red (PG14) |
| **Timing constraints** | LED sáng 5s (5000ms) chính xác, CPU không được block |
| **State to maintain** | Cờ hiệu `line_ready`, mảng `rx_buffer`, thời gian `led_green_start`, `led_red_start`, cờ `led_green_active`, `led_red_active` |

### Acceptance criteria

- [ ] Khi gõ chuỗi bất kỳ và Enter, terminal hiện dòng echo chuỗi đó.
- [ ] Gõ `"LED_GREEN"`, đèn LD3 sáng 5s, sau 5s terminal hiện `"DONE\r\n"`.
- [ ] Gõ `"LED_RED"`, đèn LD4 sáng 5s, sau 5s terminal hiện `"DONE\r\n"`.
- [ ] Trong lúc đèn sáng 5s, vẫn có thể gõ chuỗi khác và nhận echo bình thường (minh chứng non-blocking).

## 2. Peripheral selection

| Requirement | Peripheral | Processing model | Why this, not the alternative |
|---|---|---|---|
| Giao tiếp máy tính | **USART1** | Interrupt Rx, Polling Tx | Dùng ngắt Rx để không bỏ sót byte nào khi người dùng gõ phím. Dùng Polling Tx (`HAL_UART_Transmit`) vì gửi echo là tác vụ đồng bộ, chuỗi ngắn, không ảnh hưởng lớn đến timing. |
| Bật đèn LED | **GPIO Output** (Port G) | Push-pull, No pull | Tương thích phần cứng có sẵn (Active High). |
| Định thời 5s | **SysTick (`HAL_GetTick()`)** | Polling non-blocking | Đưa logic đếm thời gian ra vòng `while(1)`, không dùng Timer ngoại vi để tránh phức tạp. |

**Escalation justification:** Bắt buộc dùng Interrupt cho UART Receive vì polling UART trong vòng lặp `while(1)` chứa nhiều tác vụ khác sẽ rất dễ bị tràn thanh ghi dịch (Overrun error) và rớt byte.

## 3. Pin map

| Signal | Pin | Mode | Pull | Speed / AF | Active level | Source |
|---|---|---|---|---|---|---|
| `LD3_GREEN` | **PG13** | `Output push-pull` | `No pull` | `Low` | `High` | `[UM1670 p.20]` |
| `LD4_RED` | **PG14** | `Output push-pull` | `No pull` | `Low` | `High` | `[UM1670 p.20]` |
| `USART1_TX` | **PA9** | `Alternate Function` | `No pull` | `Very High` | N/A | `[UM1670 p.17]` |
| `USART1_RX` | **PA10**| `Alternate Function` | `No pull` | `Very High` | N/A | `[UM1670 p.17]` |

### Pin availability & Conflicts
Các chân PG13, PG14, PA9, PA10 hoàn toàn khả dụng trên STM32F429I-DISC1 cho mục đích cơ bản, không xung đột với LCD hay SDRAM. PA9/PA10 được hàn sẵn nối với mạch ST-LINK trên board để làm Virtual COM Port.

## 4. Clock and timing calculations

### System clock

Sử dụng HSI 16 MHz mặc định:
```
HSI (16 MHz) → SYSCLK = 16 MHz
AHB /1 → HCLK = 16 MHz
APB1 /1 → PCLK1 = 16 MHz
APB2 /1 → PCLK2 = 16 MHz (Cấp clock cho USART1)
```

Baud rate 115200 bps tại 16 MHz:
`USARTDIV = 16,000,000 / (16 * 115200) = 8.6805`. Hệ thống chia fractional tự động đáp ứng sai số rất thấp.

### Derived values

| Value | Derivation | Result |
|---|---|---|
| `LED Delay` | 5 seconds = 5000 ms ticks | `5000` |

## 5. STM32CubeMX configuration

Thực hiện từng bước theo đúng thứ tự:

```
□ 1. New project
     File → New → STM32 Project → "Board Selector" tab → STM32F429I-DISC1 → Next
     "Initialize all peripherals with their default Mode?" → No
     Project name: uart-ex

□ 2. System Core → RCC
     High Speed Clock (HSE): Disable (Dùng nội HSI)

□ 3. System Core → SYS
     Debug: Serial Wire
     Timebase Source: SysTick

□ 4. Clock Configuration tab
     Để nguyên hệ thống chọn HSI (HCLK = 16 MHz).

□ 5. GPIO pins
     Pinout view → Click PG13 → GPIO_Output. Label: LD3_GREEN
     Pinout view → Click PG14 → GPIO_Output. Label: LD4_RED
     Vào System Core → GPIO, chọn cả PG13 và PG14, xác nhận Output level: Low, Push Pull, No pull.

□ 6. Communication peripherals → USART1
     Mode: Asynchronous
     Configuration → Parameter Settings:
       Baud Rate: 115200
       Word Length: 8 Bits
       Parity: None
       Stop Bits: 1
     Configuration → NVIC Settings:
       ☑ USART1 global interrupt: Tích vào ô "Enabled"

□ 7. Project Manager → Code Generator
     ☑ Generate peripheral initialization as a pair of .c/.h files per peripheral

□ 8. GENERATE CODE (Alt+K)
```

### Configuration traps relevant to this task

| Trap | Symptom if you hit it | Correct setting |
|---|---|---|
| Quên bật ngắt NVIC cho USART1 | Hàm `HAL_UART_Receive_IT` chạy không báo lỗi, nhưng Callback không bao giờ được gọi | Phải tick chọn `USART1 global interrupt` trong tab NVIC của USART1 |

## 6. Implementation plan

### Program structure

```
[Khởi động]
    HAL_Init(), SystemClock_Config(), MX_GPIO_Init(), MX_USART1_UART_Init()
    Gọi HAL_UART_Receive_IT() kích hoạt ngắt nhận byte đầu tiên
           │
           ▼
[Vòng lặp while(1)]
    1. Kiểm tra cờ line_ready:
       - ĐÚNG:
         + Gửi echo chuỗi.
         + So sánh lệnh: nếu là "LED_GREEN", set cờ led_green_active = true, lưu tick hiện tại, bật PG13.
         + Tương tự cho "LED_RED".
         + Reset buffer, xóa cờ line_ready.
    2. Quản lý đèn LED_GREEN:
       - Nếu led_green_active == true VÀ (now - green_start >= 5000):
         + Tắt PG13, led_green_active = false.
         + In "DONE\r\n".
    3. Quản lý đèn LED_RED tương tự.

[Ngắt USART1 Rx (Callback)]
    Lưu byte nhận được vào rx_buffer.
    Nếu là '\r' hoặc '\n': đánh dấu line_ready = true, chèn ký tự kết thúc chuỗi '\0'.
    Gọi lại HAL_UART_Receive_IT() để hứng byte tiếp theo.
```

### Where each fragment goes

| CubeMX marker | What you add |
|---|---|
| `/* USER CODE BEGIN Includes */` | `#include <string.h>`, `#include <stdbool.h>`, `#include <stdio.h>` |
| `/* USER CODE BEGIN PV */` | Khai báo buffer, index, cờ ngắt `volatile`, biến quản lý thời gian LED |
| `/* USER CODE BEGIN 2 */` | Gọi mồi `HAL_UART_Receive_IT(&huart1, &rx_byte, 1);` |
| `/* USER CODE BEGIN WHILE */` | Vòng lặp chính xử lý chuỗi và quản lý đếm giờ |
| `/* USER CODE BEGIN 4 */` | Viết hàm `HAL_UART_RxCpltCallback` phục vụ ngắt nhận |

Key API calls: `HAL_UART_Receive_IT()` (kích hoạt ngắt nhận 1 byte) và `HAL_UART_Transmit()` (truyền chuỗi đồng bộ).

## 7. Bring-up and verification

### Incremental stages

| Stage | What to build | What you should observe | If it fails, suspect |
|---|---|---|---|
| 1 | Cấu hình xong, mồi ngắt nhận và in ra một chuỗi khởi động. | Mở Terminal (Command Palette IDE), thấy dòng chữ báo "System Ready". | Lỗi chọn sai baud rate hoặc chưa cắm ST-LINK VCP. |
| 2 | Code echo cơ bản | Gõ phím, ấn Enter, thấy dòng chữ dội về. | Quên gọi lại `Receive_IT` trong callback. |
| 3 | Tích hợp điều khiển LED | Gõ "LED_GREEN", đèn sáng 5s. Trong 5s này gõ phím khác, hệ thống vẫn echo bình thường (chứng tỏ non-blocking). | Lỗi dùng hàm so sánh chuỗi `strcmp` không dọn dẹp `\r\n` ẩn bên trong buffer. |

### Failure decision tree for this task

- Không thấy cổng COM trên máy tính: Bạn chưa cài driver ST-LINK cho Windows.
- Gõ vào Terminal không thấy gì (kể cả echo): Bạn chưa mở Terminal (Xem bài 3 phần Theory) hoặc ngắt chưa chạy do quên bật NVIC.
- So sánh chuỗi không bao giờ khớp lệnh: Ký tự gửi từ terminal đi kèm `\r\n` nhưng trong code lại chỉ so sánh `"LED_GREEN"`. Đảm bảo trong hàm ngắt đã chặn lọc `\r\n` không cho vào `rx_buffer` (chỉ đưa vào '\0').
