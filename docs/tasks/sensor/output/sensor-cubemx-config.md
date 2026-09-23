# STM32CubeMX Configuration Checklist (Sensor + UART + Timer)

Bài tập này kế thừa toàn bộ cấu hình UART và TIM3 trước đó, bổ sung thêm giao tiếp SPI5 để điều khiển cảm biến con quay hồi chuyển (Gyroscope) có sẵn trên mạch STM32F429I-DISC1.

## 1. Mở Project & Clock Setup

1. **Mở STM32CubeMX**, tạo project mới cho Board **STM32F429I-DISC1** (hoặc MCU **STM32F429ZITx**).
2. **RCC**: High Speed Clock (HSE) > `Disable` (Sử dụng HSI).
3. **Clock Configuration**: 
   * Đảm bảo nguồn là HSI (16 MHz).
   * APB1 và APB2 đều là 16 MHz.

## 2. GPIO Configuration (CS & User LEDs)

1. **User LEDs**: Chọn chân **PG13** và **PG14** > `GPIO_Output`. Đổi tên thành `LD3_GREEN` và `LD4_RED`.
2. **Gyroscope Chip Select (CS)**: 
   * Tìm chân **PC1**, click chọn `GPIO_Output`.
   * Mở System Core > GPIO > Chọn `PC1`:
     * GPIO output level: `High` *(Cực kỳ quan trọng: Kéo lên mức Cao mặc định để cảm biến không bị truy cập nhầm khi rảnh rỗi)*.
     * User Label: `GYRO_CS`.

## 3. SPI5 Configuration (Giao tiếp Gyroscope)

Cảm biến L3GD20 trên board được đấu nối vật lý vào bộ SPI5.
1. Categories > **Connectivity** > **SPI5**.
2. **Mode**: 
   * Mode: `Full-Duplex Master`
3. Kiểm tra bản đồ chân (Pinout): Đảm bảo các chân **PF7** (SCK), **PF8** (MISO), **PF9** (MOSI) đã sáng màu xanh lá. (Nếu CubeMX gán chân khác, bấm Ctrl+Click kéo thả về đúng bộ 3 chân này).
4. **Parameter Settings**:
   * Data Size: `8 Bits`
   * First Bit: `MSB First`
   * Prescaler (for Baud Rate): `4` hoặc `2` *(APB2 đang là 16 MHz, chia 2 là 8 MHz, chia 4 là 4 MHz. Cảm biến L3GD20 hỗ trợ SPI tối đa 10 MHz nên chia 2 hay 4 đều an toàn)*.
   * Clock Polarity (CPOL): `Low`
   * Clock Phase (CPHA): `1 Edge`
   *(Chuẩn giao tiếp SPI Mode 0 này tương thích 100% với L3GD20).*

## 4. USART1 Configuration (VCP ST-LINK)

1. Categories > **Connectivity** > **USART1**.
2. Mode: `Asynchronous`. (Đảm bảo PA9 và PA10 được gán).
3. **Parameter Settings**: Baud Rate `115200`, 8 Bits, None, 1.
4. **NVIC Settings**: Tích chọn **USART1 global interrupt**.

## 5. TIM3 Configuration (Hardware Timer 1 Hz)

1. Categories > **Timers** > **TIM3**.
2. Clock Source: `Internal Clock`.
3. **Parameter Settings**:
   * Prescaler: `15999`
   * Counter Period: `999`
   * auto-reload preload: `Enable`
4. **NVIC Settings**: Tích chọn **TIM3 global interrupt**.

## 6. NVIC & Priority Settings

Để đảm bảo việc lấy dữ liệu góc và nhận lệnh không đụng chạm nhau:
1. System Core > **NVIC**.
2. Tại cột **Preemption Priority**:
   * `USART1 global interrupt`: Đặt là `1` (Ưu tiên cao nhất).
   * `TIM3 global interrupt`: Đặt là `2` (Ưu tiên thấp hơn UART).

## 7. Project Generation

1. Tab **Project Manager** > Tên Project (VD: `Sensor_Gyro_UART`).
2. Toolchain/IDE: `STM32CubeIDE`.
3. Bấm **GENERATE CODE**.
