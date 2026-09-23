# STM32CubeMX Configuration Checklist (UART & Timer)

Bài tập này kế thừa bài tập UART trước đó, bổ sung thêm việc cấu hình Hardware Timer (TIM3) để tạo ngắt chu kỳ 1 giây (1 Hz).

## 1. Mở Project & Clock Setup

1. **Mở STM32CubeMX**, chọn File > New Project.
2. Chọn Board: **STM32F429I-DISC1** hoặc chọn MCU: **STM32F429ZITx**. Nhấn Start Project.
3. Trong tab **Pinout & Configuration** > Categories > **System Core** > **RCC**:
   * High Speed Clock (HSE): `Disable` (Chúng ta sẽ dùng dao động nội HSI).
4. Chuyển sang tab **Clock Configuration**:
   * Kiểm tra bộ dao động HSI (16 MHz) đang được chọn.
   * Tất cả các bộ chia HCLK, APB1, APB2 đều là `/1`.
   * **Kết quả:** Tần số `APB1 Timer clocks` cấp cho TIM3 sẽ là **16 MHz**.

## 2. GPIO Configuration (User LEDs)

1. Trở lại tab **Pinout & Configuration**.
2. Tìm chân **PG13** trên sơ đồ chip, click chuột trái, chọn `GPIO_Output`.
3. Tìm chân **PG14** trên sơ đồ chip, click chuột trái, chọn `GPIO_Output`.
4. Mở Categories > **System Core** > **GPIO**:
   * Chọn `PG13`, đổi User Label thành `LD3_GREEN`.
   * Chọn `PG14`, đổi User Label thành `LD4_RED`.

## 3. USART1 Configuration (VCP ST-LINK)

*(Phần này y hệt bài tập UART cũ)*
1. Categories > **Connectivity** > **USART1**.
2. Đổi Mode thành `Asynchronous`.
3. Nhìn lên hình vi điều khiển, đảm bảo chân **PA9** (USART1_TX) và **PA10** (USART1_RX) đang có màu xanh lá cây.
4. **Parameter Settings**:
   * Baud Rate: `115200` Bits/s
   * Word Length: `8 Bits` (kể cả Parity)
   * Parity: `None`
   * Stop Bits: `1`
5. **NVIC Settings**:
   * Tích chọn **USART1 global interrupt**.

## 4. TIM3 Configuration (Hardware Timer 1 Hz)

Đây là điểm cốt lõi của bài tập mới. Chúng ta sử dụng TIM3 (Timer 16-bit) để đếm và sinh ngắt mỗi 1 giây.

1. Categories > **Timers** > **TIM3**.
2. Đổi **Clock Source** thành `Internal Clock`.
3. **Parameter Settings**:
   * **Prescaler (PSC - 16 bits value)**: `15999`
   * **Counter Mode**: `Up`
   * **Counter Period (AutoReload Register - ARR - 16 bits value)**: `999`
   * **auto-reload preload**: `Enable`
4. **NVIC Settings**:
   * Tích chọn **TIM3 global interrupt**.

> **Tại sao lại có công thức PSC = 15999 và ARR = 999?**
> Tần số cấp cho Timer (F_timer) là 16 MHz = 16,000,000 Hz.
> - Prescaler (PSC) có tác dụng chia nhỏ xung nhịp. Vi điều khiển đếm từ 0, nên hệ số chia thực tế là PSC + 1.
> - Chọn PSC = 15999, hệ số chia là 16000.
> - Tần số sau chia: F_tick = 16,000,000 / 16,000 = **1,000 Hz (Mỗi tick mất đúng 1 ms)**.
> - AutoReload Register (ARR) là số nhịp Timer đếm trước khi tràn và sinh ngắt.
> - Chọn ARR = 999 (đếm từ 0 đến 999 là 1000 nhịp).
> - Tổng thời gian sinh ngắt: 1000 nhịp × 1 ms = **1000 ms = 1 giây**.

## 5. NVIC (Ngắt) & Priority Settings

Trong bài tập này, chúng ta có 2 ngắt tranh chấp nhau (UART nhận ký tự và Timer đếm giây). Nếu CPU đang bận xử lý ngắt Timer mà có ký tự bay vào UART, việc xử lý trễ có thể làm mất ký tự (Overrun Error). Vì vậy, UART phải được ưu tiên cao hơn Timer.

1. Categories > **System Core** > **NVIC**.
2. Trong bảng ngắt, tìm 2 dòng `USART1 global interrupt` và `TIM3 global interrupt`.
3. Tại cột **Preemption Priority**:
   * Sửa của `USART1` thành `1`.
   * Sửa của `TIM3` thành `2`.
   *(Trong ARM Cortex-M, số càng nhỏ ưu tiên càng cao. Mặc định tất cả đều là 0. Chỉnh như trên giúp UART luôn được ưu tiên chèn ngang).*

## 6. Project Generation

1. Chuyển sang thẻ **Project Manager**.
2. Đặt tên project (ví dụ: `UART_TIM3_CheckHW`).
3. Toolchain / IDE: Chọn **STM32CubeIDE**.
4. Bấm **GENERATE CODE** ở góc phải trên cùng.
