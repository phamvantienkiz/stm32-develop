## 1. The assignment, verbatim

**Yêu cầu**: Thực hành Timer (Timebase & Interrupt) kết hợp UART để kiểm tra thời gian thực thi và cơ chế check hardware.

## 2. My understanding of it, in my own words

**Diễn giải yêu cầu**:

- Phát triển một project mới kế thừa hoàn toàn mã nguồn bài UART đang chạy tốt (main.c cũ), bổ sung thêm ngoại vi **Hardware Timer (TIM3)** chạy ở chế độ ngắt chu kỳ (Period Elapsed Callback / Update Interrupt).
- Tách biệt logic đếm thời gian khỏi `HAL_GetTick()` / SysTick sang bộ đếm phần cứng chuyên dụng `TIM3` (sử dụng các thanh ghi `PSC` và `ARR`).
- Triển khai đầy đủ 2 mức độ của yêu cầu "check hardware":
  1. **Mức độ 1 (System Heartbeat & Non-blocking Life-sign):** Timer tạo nhịp ngắt đều đặn mỗi 1 giây (1 Hz) để tăng biến đếm `uptime_seconds`, định kỳ gửi thông điệp heartbeat qua UART (`[HW_TIMER] Uptime: X s\r\n`) để chứng minh CPU và bộ ngắt NVIC hoạt động bình thường, không bị treo.
  2. **Mức độ 2 (Clock Tree Sanity Check & Timing Validation):** Cấu hình Timer dựa trên tần số dao động nội HSI 16 MHz để tạo ngắt chuẩn 1.000 s. Số tick tăng lên được đối chiếu trực tiếp với đồng hồ thực tế để xác nhận cấu hình Clock Tree / Prescaler của phần cứng là chính xác.
- Giữ nguyên và mở rộng tập lệnh UART:
  1. **Kế thừa các lệnh cũ:** Nhận lệnh qua ngắt UART từng byte. Lệnh `"LED_GREEN"` bật LD3 trong 5s; `"LED_RED"` bật LD4 trong 5s; sau khi tắt phản hồi chuỗi `"DONE\r\n"`.
  2. **Thêm lệnh mới (`"GET_TIME"`):** Trả về thời gian hệ thống đã chạy liên tục tính bằng giây được đo bởi chính Hardware Timer (`uptime_seconds`).
  3. **Thêm lệnh kiểm tra phần cứng (`"CHECK_HW"`):** Đọc giá trị thanh ghi đếm `CNT`, `PSC`, `ARR` của `TIM3` để in ra terminal chứng minh Timer phần cứng đang chạy thực tế.

## 3. Hardware

- Board: `STM32F429I-DISC1` (Discovery kit with STM32F429ZI MCU)[cite: 3]
- MCU: `ARM Cortex-M4: STM32F429ZIT6U`
- Clock Configuration:
  - Nguồn xung: HSI (Internal High Speed oscillator) = `16 MHz`
  - Bus Clocks: SYSCLK = 16 MHz, HCLK = 16 MHz, PCLK1 = 16 MHz (APB1 Timer Clock = 16 MHz)
- Peripheral Connections:
  - **USART1 (VCP qua ST-LINK):**
    - Pin `PA9`: USART1_TX (cầu hàn SB11)
    - Pin `PA10`: USART1_RX (cầu hàn SB15)
    - Cấu hình: `115200 bps`, `8N1`, NVIC USART1 Interrupt Enable
  - **TIM3 (General Purpose Timer):**
    - Chế độ: Internal Clock
    - Prescaler (PSC): `15999` (chia clock 16 MHz về 1 kHz -> 1 ms/tick)
    - Counter Period (ARR): `999` (đếm 1000 tick -> ngắt đúng 1.000 s / 1 Hz)
    - NVIC TIM3 global interrupt: `Enable`
  - **User LEDs:**
    - `LD3` (Green LED): Pin `PG13` (Active HIGH)
    - `LD4` (Red LED): Pin `PG14` (Active HIGH)

## 4. Constraints from the assignment

- Giữ nguyên cấu trúc code non-blocking của project UART trước đó; không gọi `HAL_Delay()` hay các hàm xử lý chuỗi nặng bên trong hàm phục vụ ngắt Timer (`HAL_TIM_PeriodElapsedCallback`).
- Sử dụng chuẩn thư viện STM32 HAL Driver do STM32CubeMX / STM32CubeIDE sinh ra.
- Các biến chia sẻ giữa Interrupt Context (ngắt Timer, ngắt UART) và vòng lặp `while (1)` phải được khai báo với từ khóa `volatile`.

## 5. What I already have working

- Project UART hoàn chỉnh độc lập: Kết nối qua cổng COM ảo của ST-Link, truyền nhận mượt mà với phần mềm Hercules[cite: 2].
- Đã xử lý thành công việc echo chuỗi dữ liệu biến thiên kết thúc bằng CR/LF (`\r` hoặc `\n`).
- Đã thực hiện nhận diện lệnh `LED_GREEN` và `LED_RED`, bật LED 5s theo phương thức non-blocking và phản hồi `DONE`.

## 6. What I want out of this

- [x] Full design dossier (CubeMX configuration) + reference `main.c` (kế thừa toàn bộ code UART cũ + tích hợp TIM3)
- [x] Extra explanation of <peripheral/concept> — I have not used it before
  - Cụ thể:
    - Cơ chế hoạt động của bộ đếm Hardware Timer trong STM32: mối quan hệ giữa Clock nội, thanh ghi chia tần `PSC`, thanh ghi chu kỳ `ARR` và cờ ngắt tràn `UIF`.
    - Cách thức khởi tạo ngắt Timer bằng hàm `HAL_TIM_Base_Start_IT()` và cơ chế ghi đè hàm yếu (weak function) `HAL_TIM_PeriodElapsedCallback()`.
    - Giải thích về mặt kỹ thuật khái niệm "Check Hardware bằng Timer" (kiểm tra xung nhịp và nhịp sinh tồn heartbeat) để đưa vào phần báo cáo cho thầy.

## 7. Open questions I have

- Sự ưu tiên ngắt (Interrupt Preemption Priority) giữa ngắt nhận UART (`USART1`) và ngắt định thời (`TIM3`) nên được phân bổ như thế nào trong NVIC để hệ thống không bao giờ bị mất ký tự khi nhận dữ liệu tốc độ cao?
- Khi giá trị thanh ghi biến đếm giây (`uptime_seconds`) tăng lên liên tục trong thời gian dài, cách xử lý vấn đề tràn số (integer overflow) như thế nào?
