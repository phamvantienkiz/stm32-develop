# Run, Test & Debug Guide — lcd-gyro-bsp

> Hướng dẫn biên dịch, chạy, và kiểm thử dự án trên bo mạch thực tế.

| | |
|---|---|
| Task ID | `lcd-gyro-bsp` |
| Board | `STM32F429I-DISC1` |

---

## 1. Build & Flash (Biên dịch và Nạp)

*Lưu ý: Trước khi Build, đảm bảo bạn đã hoàn thành việc tạo thư mục và Copy các file BSP theo đúng Hướng dẫn phần 5 trong file `lcd-cubemx-config.md`.*

1. Trên STM32CubeIDE, nhấn vào project `lcd-gyro-bsp` trong **Project Explorer**.
2. Nhấn nút **Build (Cái búa)** trên thanh công cụ. Quan sát cửa sổ Console ở dưới:
   - Nếu kết quả là `0 errors, 0 warnings` -> Bạn đã copy file BSP và trỏ đường dẫn Include chuẩn xác.
   - Nếu có lỗi, xem phần Troubleshooting bên dưới.
3. Cắm cáp USB vào cổng **USB ST-LINK** trên bo mạch STM32F429I-DISC1 (cổng Mini-USB sát mép trên).
4. Nhấn nút **Debug (Con bọ xanh)** để nạp firmware vào vi điều khiển.
5. Khi IDE chuyển sang giao diện Debug và dừng ở hàm `main()`, nhấn nút **Resume (F8)** (biểu tượng Play) để chương trình chạy.

---

## 2. Test Scenarios (Kịch bản kiểm thử)

### Test 1: Khởi động và vẽ giao diện tĩnh
- **Mục tiêu**: Xác nhận LCD, chip nhớ SDRAM, và bộ điều khiển LTDC đã được khởi tạo thành công.
- **Thực hiện**: Bấm nút Reset đen trên board.
- **Kỳ vọng**: Màn hình sáng lên ngay lập tức. Nền màu đen, xuất hiện chữ "STM32 GYROSCOPE" màu cam ở giữa trên cùng. Các dòng chữ nền tảng như "AXIS X:", "AXIS Y:", "AXIS Z:", "UPTIME:" hiển thị rõ nét, không bị xước hạt.

### Test 2: Đọc dữ liệu Gyroscope & Cập nhật LCD
- **Mục tiêu**: Xác nhận đường SPI5 và cảm biến L3GD20 phản hồi bình thường, dữ liệu được in ra LCD không bị giật.
- **Thực hiện**: Cầm board mạch xoay lật theo nhiều góc (nghiêng lên xuống, nghiêng trái phải, xoay tròn mặt phẳng).
- **Kỳ vọng**: Các con số nằm cạnh "AXIS X/Y/Z" trên màn hình thay đổi liên tục một cách mượt mà (chớp 20 lần/giây nhưng mắt không thấy giật). Dữ liệu dao động từ vài chục đến hàng ngàn (`dps`).

### Test 3: Bài kiểm tra Padding (Triệt tiêu rác màn hình)
- **Mục tiêu**: Xác minh giải pháp đệm khoảng trắng `%6d` có tác dụng (không bị kẹt chữ số cũ).
- **Thực hiện**: Lắc board mạch thật mạnh để số liệu trục vọt lên 4 chữ số (ví dụ: `2500`), ngay sau đó đặt im board xuống bàn để số liệu rơi về xấp xỉ `0`.
- **Kỳ vọng**: Chữ số "2500" lập tức bị xóa sạch và thay bằng chuỗi có dạng "     0" (có khoảng trống đằng trước). Không bao giờ có hiện tượng số cũ dính lại thành "0500".

### Test 4: Đồng bộ UART & Uptime Timer
- **Mục tiêu**: Đảm bảo ngắt TIM3 và truyền UART1 không bị đụng độ hoặc đình trệ do việc vẽ màn hình.
- **Thực hiện**: 
  - Quan sát dòng "UPTIME:" trên LCD.
  - Mở phần mềm Hercules hoặc PuTTY trên máy tính, kết nối cổng COM tương ứng (Baud rate: 115200).
- **Kỳ vọng**: 
  - UPTIME trên màn hình LCD đếm tăng đều đặn mỗi giây.
  - Cửa sổ Terminal trên PC liên tục nhận được luồng log dữ liệu: `X: ... Y: ... Z: ...`.

---

## 3. Troubleshooting (Khắc phục sự cố)

| Triệu chứng (Symptom) | Nguyên nhân (Cause) | Cách sửa (Fix) |
|---|---|---|
| Lỗi biên dịch: `undefined reference to 'BSP_LCD_Init'` hoặc `fatal error: stm32f429i_discovery.h: No such file` | Trình biên dịch không tìm thấy file `.c` của BSP hoặc đường dẫn Header `.h` bị khai báo thiếu. | Đọc lại Bước 5.4 ở file `lcd-cubemx-config.md`. Chắc chắn bạn đã Add đường dẫn vào **Include paths** của **C/C++ Build -> Settings**. |
| Lỗi biên dịch: `multiple definition of 'HAL_LTDC_MspInit'` | Cả CubeMX và BSP đều sinh ra hàm khởi tạo phần cứng MSP cho cùng 1 ngoại vi. | Bạn đã lỡ tay kích hoạt LTDC hoặc SPI5 hoặc FMC trong CubeMX. Quay lại CubeMX, tắt chúng đi (Disable) và bấm Generate Code. |
| Treo chương trình ở `HardFault_Handler` | Gọi dùng Font chữ (ví dụ `&Font16`) nhưng quên copy file font C vào project. | Đảm bảo bạn đã copy file `font16.c` và thư mục `Utilities/Fonts` nằm đàng hoàng trong project. |
| Nạp code xong, màn hình sáng trắng bóc, không lên chữ nào | Lỗi phần cứng hoặc cấu hình Clock/SDRAM sai. LTDC bị cấp xung nhịp Pixel Clock lỗi. | Lỗi này thường do cấu hình Clock trong CubeMX bị sai nguồn gốc. Kiểm tra lại Bước 2 (RCC) ở hướng dẫn CubeMX: Phải chọn HSE là **BYPASS Clock Source**, sau đó nhập Input 8MHz ở thẻ Clock Configuration. |
| Log UART (PC) in ra liên tục `X:0 Y:0 Z:0` dù lắc board | Code khởi tạo cảm biến L3GD20 thất bại hoặc không ghi được lệnh mở nguồn qua SPI5. | Kiểm tra lại hàm `L3GD20_Init()` xem đã gọi lệnh `GYRO_IO_Write` đúng địa chỉ `0x20` chưa. |
