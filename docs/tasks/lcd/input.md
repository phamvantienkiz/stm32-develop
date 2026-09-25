## 1. The assignment, verbatim

**Yêu cầu**: Thực hành lập trình giao tiếp với màn hình LCD tích hợp trên bo mạch STM32F429I-DISC1, kết hợp đọc dữ liệu cảm biến con quay hồi chuyển (Gyroscope) có sẵn trên bo mạch để hiển thị liên tục các giá trị 3 trục X, Y, Z ra giao diện màn hình TFT LCD (sử dụng tài nguyên BSP có sẵn) và đồng thời xuất log ra UART.

## 2. My understanding of it, in my own words

**Diễn giải yêu cầu**:

- Phát triển một project tích hợp hiển thị đầy đủ trên bo mạch STM32F429I-DISC1, kế thừa các module đã hoàn thiện: **UART (USART1)**, **Hardware Timer (TIM3)**, và **Cảm biến Gyroscope L3GD20 (SPI5)**.
- Sử dụng trực tiếp bộ driver **BSP (Board Support Package)** của ST (`stm32f429i_discovery_lcd` kết hợp driver phần cứng `ili9341` và `stm32f429i_discovery_sdram`) để điều khiển màn hình TFT LCD 2.4 inch (240x320 pixel) qua bộ điều khiển LTDC và SDRAM ngoài (FMC).
- Chi tiết luồng thực thi của chương trình:
  1. **Khởi tạo hệ thống & Phần cứng:**
     - Thiết lập hệ thống xung nhịp, kích hoạt Timer ngắt chu kỳ 1s (TIM3) làm Heartbeat và kiểm soát thời gian.
     - Khởi tạo màn hình LCD qua hàm `BSP_LCD_Init()`, thiết lập Background Layer tại địa chỉ Frame Buffer trên SDRAM, bật hiển thị, xóa màn hình về nền trắng (hoặc đen), cài đặt font chữ (Font16/Font20).
     - Khởi tạo giao tiếp SPI5 và bật cảm biến Gyro L3GD20 qua việc cấu hình thanh ghi `CTRL_REG1` (`0x20`).
  2. **Vòng lặp đọc và cập nhật giao diện (Refresh Loop):**
     - Đọc giá trị góc quay 3 trục ($X, Y, Z$) từ L3GD20 định kỳ (khoảng 50 ms - 100 ms).
     - Định dạng dữ liệu thành các chuỗi ký tự hiển thị (dạng `X: [val] dps`, `Y: [val] dps`, `Z: [val] dps`).
     - Sử dụng API BSP `BSP_LCD_DisplayStringAtLine()` để in đè các dòng chữ lên các tọa độ dòng cố định trên màn hình LCD mà không làm nhấp nháy toàn bộ khung hình.
     - Hiển thị thêm mốc thời gian hoạt động (System Uptime từ Hardware Timer) trên góc màn hình LCD để thể hiện tính năng đo thời gian thực thi.
  3. **Đồng bộ UART:** Vẫn duy trì xuất chuỗi log dữ liệu trục và trạng thái lệnh qua cổng COM ảo (USART1 - ST-LINK VCP) về phần mềm Terminal trên máy tính.

## 3. Hardware

- Board: `STM32F429I-DISC1` (Discovery kit with STM32F429ZI MCU)
- MCU: `ARM Cortex-M4: STM32F429ZIT6U`
- Clock Configuration:
  - Nguồn xung: HSE (Thạch anh ngoài 8 MHz) cấu hình qua PLL để đạt `SYSCLK = 168 MHz` hoặc `180 MHz` (đảm bảo đủ băng thông cấp xung Pixel Clock `PLLSAI` cho LTDC và FMC SDRAM).
- Peripheral Connections & Resources:
  - **TFT LCD & SDRAM (Quản lý qua BSP):**
    - Màn hình: LCD TFT 2.4" QVGA (240x320), chip điều khiển RGB `ILI9341`.
    - Giao tiếp: Quét dữ liệu điểm ảnh qua bus song song **LTDC** (RGB565), kết hợp bus SPI để gửi tập lệnh khởi tạo ban đầu cho ILI9341.
    - Bộ nhớ Frame Buffer: Chip ngoài SDRAM 64 Mbit (8 MB) **ISSI IS42S16400J** kết nối qua bộ điều khiển **FMC** (Địa chỉ cơ sở: `0xD0000000`).
  - **MEMS Gyroscope (L3GD20 / I3G4250D):**
    - Ngoại vi: **SPI5** Full-Duplex Master
    - Chân tín hiệu: `PF7` (SCK), `PF8` (MISO), `PF9` (MOSI)
    - Chân điều khiển: `PC1` (CS - Chip Select, Active LOW)
  - **USART1 (VCP nối máy tính qua ST-LINK USB):**
    - Pin `PA9` (TX), `PA10` (RX)
    - Cấu hình: `115200 bps`, `8N1`
  - **TIM3 (Hardware Timebase & Heartbeat):**
    - Cấu hình tạo ngắt tràn chu kỳ 1s (Update Interrupt)
  - **User LEDs & Buttons:**
    - `LD3` Green (`PG13`), `LD4` Red (`PG14`)
    - `B1` USER Button: Pin `PA0` (xử lý đổi màu nền hoặc tạm dừng đọc nếu cần)

## 4. Constraints from the assignment

- Ưu tiên sử dụng trực tiếp các API hiển thị bậc cao của BSP (`BSP_LCD_Init`, `BSP_LCD_Clear`, `BSP_LCD_SetTextColor`, `BSP_LCD_DisplayStringAtLine`) thay vì tự viết lại driver thanh ghi LTDC/FMC từ đầu để tránh lỗi định thời phần cứng.
- Tốc độ làm tươi hiển thị LCD (Display Refresh Rate) phải được điều tiết hợp lý (10 Hz - 20 Hz, tương đương delay 50 - 100 ms) bằng Timer hoặc bộ đếm tick, tránh gọi lệnh xóa toàn màn hình (`BSP_LCD_Clear`) liên tục trong `while (1)` gây hiện tượng chớp giật hình ảnh (flickering).
- Không gọi các hàm xử lý chuỗi (`snprintf`) hay hàm vẽ màn hình LCD bên trong trình phục vụ ngắt (ISR/Callback).

## 5. What I already have working

- Giao tiếp UART (USART1) qua cổng Virtual COM Port kết nối phần mềm Hercules: truyền nhận 2 chiều ổn định, echo chuỗi và nhận diện lệnh điều khiển LED.
- Hardware Timer (TIM3) ngắt chu kỳ 1s hoạt động chính xác, tạo nhịp Heartbeat kiểm tra xung nhịp và đếm thời gian thực thi (uptime).
- Module giao tiếp SPI5 đọc thanh ghi cảm biến con quay hồi chuyển L3GD20: đọc đúng mã định danh `WHO_AM_I = 0xD4`, tính toán trích xuất thành công 3 trục góc quay $X, Y, Z$ xuất ra máy tính.

## 6. What I want out of this

- [x] Full design dossier (hướng dẫn tích hợp bộ thư viện BSP LCD vào project CubeMX hiện tại hoặc cách cấu hình project có sẵn BSP) + reference `main.c` hoàn chỉnh kết hợp cả 4 ngoại vi (LCD + Gyro SPI5 + UART + TIM3).
- [x] Extra explanation of <peripheral/concept> — I have not used it before
  - Cụ thể:
    - Quy trình khởi tạo tầng hiển thị của BSP: Vai trò của `BSP_LCD_Init()`, cơ chế cấp phát vùng nhớ Frame Buffer trên SDRAM (`LCD_FRAME_BUFFER = 0xD0000000`), và cách thức bộ điều khiển LTDC tự động quét vùng nhớ này ra màn hình.
    - Kỹ thuật in đè text (in-place text update): Cách thiết lập màu chữ (`TextColor`), màu nền ký tự (`BackColor`) và dùng khoảng trắng đệm chuỗi để cập nhật số liệu cảm biến liên tục mà không cần xóa lại nền màn hình.
    - Cách tổ chức bố cục giao diện (UI layout) cơ bản: Vẽ tiêu đề, khung viền, nhãn hiển thị và giá trị động trên màn hình LCD độ phân giải 240x320.

## 7. Open questions I have

- Khi cập nhật liên tục các con số thay đổi nhanh từ cảm biến lên LCD, làm thế nào để xử lý triệt để hiện tượng số bị nhấp nháy hoặc các ký tự cũ không bị đè hết khi độ dài chuỗi ký tự thay đổi (ví dụ: số từ 4 chữ số nhảy về 2 chữ số)?
- Trong trường hợp xung đột bus nội bộ khi bộ điều khiển DMA2D/LTDC liên tục đọc SDRAM trong khi CPU cũng truy xuất bus để đọc ghi dữ liệu, cần lưu ý điều chỉnh mức ưu tiên băng thông bộ nhớ như thế nào?
