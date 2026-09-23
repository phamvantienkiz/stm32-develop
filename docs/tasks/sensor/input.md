## 1. The assignment, verbatim

**Yêu cầu**: Thực hành Sensor, tạo dự án giao tiếp với cảm biến con quay hồi chuyển có sẵn trên bo mạch để lấy thông tin 3 trục cảm biến, kết hợp truyền nhận qua UART và kiểm soát định thời bằng Timer

## 2. My understanding of it, in my own words

**Diễn giải yêu cầu**:

- Phát triển một project mới kế thừa hoàn toàn mã nguồn bài UART đang chạy tốt (main.c cũ) và phần cấu hình **Hardware Timer (TIM3)** chạy ngắt chu kỳ 1s (Update Interrupt/Period Elapsed Callback).
- Tích hợp thêm module giao tiếp với **cảm biến con quay hồi chuyển (MEMS Gyroscope L3GD20/I3G4250D)** có sẵn trên bo mạch thông qua ngoại vi **SPI5 (Master Full-Duplex)** và chân điều khiển Chip Select (`PC1`).
- Quy trình hoạt động của cảm biến:
  1. Đọc kiểm tra ID nhận dạng từ thanh ghi `WHO_AM_I` (`0x0F`). Nếu trả về đúng `0xD4` (hoặc `0xD3`) mới kích hoạt thanh ghi điều khiển `CTRL_REG1` (`0x20`) để bật nguồn cảm biến và 3 trục đo.
  2. Đọc 6 byte dữ liệu thô liên tiếp bắt đầu từ thanh ghi `OUT_X_L` (`0x28`) sử dụng cờ đọc nhiều byte tự tăng địa chỉ (`0xC0`), ghép thành 3 giá trị góc quay có dấu 16-bit ($X, Y, Z$).
- Mở rộng tập lệnh UART giao tiếp với phần mềm Hercules trên máy tính (cổng COM3, 115200 bps):
  1. **Kế thừa các lệnh cũ:** Nhận chuỗi qua ngắt UART từng byte; `"LED_GREEN"` bật LD3 5s; `"LED_RED"` bật LD4 5s; hoàn thành gửi phản hồi `"DONE\r\n"`.
  2. **Kế thừa các lệnh Timer:** Lệnh `"GET_TIME"` trả về thời gian uptime hệ thống; lệnh `"CHECK_HW"` trả về giá trị các thanh ghi của bộ đếm phần cứng `TIM3` (`CNT`, `PSC`, `ARR`).
  3. **Thêm lệnh đọc cảm biến (`"GET_GYRO"`):** Đọc tức thời dữ liệu 3 trục và gửi phản hồi dạng: `Gyro -> X: [giá trị] | Y: [giá trị] | Z: [giá trị] dps\r\n`.
  4. **Chế độ phát trực tiếp (`"STREAM_ON"` / `"STREAM_OFF"`):** Cho phép bật/tắt chế độ tự động gửi dữ liệu góc quay $X, Y, Z$ định kỳ (ví dụ mỗi 200 ms hoặc đồng bộ theo nhịp ngắt Timer) lên màn hình Hercules.

## 3. Hardware

- Board: `STM32F429I-DISC1` (Discovery kit with STM32F429ZI MCU)
- MCU: `ARM Cortex-M4: STM32F429ZIT6U`[
- Clock Configuration:
  - Nguồn xung: HSI (Internal High Speed oscillator) = `16 MHz`
  - Bus Clocks: SYSCLK = 16 MHz, HCLK = 16 MHz, PCLK1 = 16 MHz (APB1 Timer Clock = 16 MHz), PCLK2 = 16 MHz (APB2 Peripheral Clock = 16 MHz)
- Peripheral Connections:
  - **USART1 (VCP nối máy tính qua cổng ST-LINK USB):**
    - Pin `PA9`: USART1_TX (cầu hàn SB11)
    - Pin `PA10`: USART1_RX (cầu hàn SB15)
    - Cấu hình: `115200 bps`, `8N1`, NVIC USART1 global interrupt Enable
  - **TIM3 (General Purpose Hardware Timebase):**
    - Chế độ: Internal Clock
    - Prescaler (PSC): `15999` (chia clock 16 MHz về 1 kHz -> 1 ms/tick)
    - Counter Period (ARR): `999` (đếm 1000 tick -> ngắt tràn đúng 1.000 s / 1 Hz)
    - NVIC TIM3 global interrupt: `Enable`
  - **SPI5 (Giao tiếp Gyroscope L3GD20 tích hợp):**
    - Pin `PF7`: SPI5_SCK
    - Pin `PF8`: SPI5_MISO
    - Pin `PF9`: SPI5_MOSI
    - Pin `PC1`: GPIO_Output (CS - Chip Select, kéo HIGH mặc định)
    - Cấu hình SPI: Mode Full-Duplex Master, Data Size 8-bit, FirstBit MSB First, CPOL Low, CPHA 1 Edge (Mode 0) hoặc CPOL High/CPHA 2 Edge (Mode 3), Prescaler điều chỉnh clock $\le 10\text{ MHz}$.
  - **User LEDs:**
    - `LD3` (Green LED): Pin `PG13` (Active HIGH)
    - `LD4` (Red LED): Pin `PG14` (Active HIGH)

## 4. Constraints from the assignment

- Giữ nguyên cấu trúc non-blocking của project UART/Timer trước đó; tuyệt đối không thực hiện đọc dữ liệu SPI hoặc in chuỗi UART dài bên trong hàm phục vụ ngắt Timer (`HAL_TIM_PeriodElapsedCallback`).
- Sử dụng chuẩn thư viện STM32 HAL Driver do STM32CubeMX / STM32CubeIDE sinh ra.
- Quá trình giao tiếp SPI5 và xử lý cờ đọc dữ liệu cảm biến được thực hiện tập trung tại vòng lặp `while (1)`.
- Các biến chia sẻ giữa Interrupt Context và Main Thread phải được khai báo với từ khóa `volatile`.

## 5. What I already have working

- Giao tiếp UART qua cổng COM3 với phần mềm Hercules hoạt động mượt mà 2 chiều: Echo chuỗi biến thiên, nhận diện lệnh `LED_GREEN`, `LED_RED` bật LED 5s non-blocking và gửi trả `DONE`.
- Ngoại vi Timer `TIM3` ngắt chu kỳ 1s chuẩn xác, phục vụ tính năng Heartbeat và kiểm soát thời gian phần cứng độc lập.

## 6. What I want out of this

- [x] Full design dossier (CubeMX configuration cho SPI5, USART1, TIM3, GPIO) + reference `main.c` hoàn chỉnh kết hợp cả 3 ngoại vi.
- [x] Extra explanation of <peripheral/concept> — I have not used it before
  - Cụ thể:
    - Cơ chế truyền nhận SPI Full-Duplex dùng hàm `HAL_SPI_TransmitReceive()` và vai trò điều khiển chân Chip Select (CS).
    - Bản đồ thanh ghi của chip L3GD20: ý nghĩa thanh ghi `WHO_AM_I` (`0x0F`), thanh ghi cấu hình `CTRL_REG1` (`0x20`), và cơ chế bit tự tăng địa chỉ (`0xC0` / Auto-increment) khi đọc luồng 6 byte dữ liệu trục.
    - Cách chuyển đổi (type casting / bit shift) từ 2 byte nhị phân riêng biệt (`OUT_X_L` và `OUT_X_H`) sang số nguyên có dấu 16-bit (`int16_t`) biểu diễn vận tốc góc $dps$.

## 7. Open questions I have

- Tốc độ truyền của đường SPI5 có ảnh hưởng gì tới độ chính xác khi đọc dữ liệu từ cảm biến L3GD20 hay không (chuẩn Baudrate Prescaler an toàn nhất là bao nhiêu)?
- Khi bo mạch nằm yên trên bàn, giá trị $X, Y, Z$ trả về có bằng 0 tuyệt đối hay không (hiện tượng Zero-rate level / Gyro Drift) và có cần thuật toán bù trừ trừ bias đơn giản hay không?
