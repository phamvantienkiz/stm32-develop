## 1. The assignment, verbatim

**Yêu cầu**: Thực hành tạo animation bot pixel trên màn hình LCD

## 2. My understanding of it, in my own words

**Diễn giải yêu cầu**:

- Phát triển ứng dụng, trong đó tập trung vào việc tạo một bot pixel trên màn hình LCD, có các hành động lặp đi lặp đi lặp lại.
- Hướng dẫn tạo bot được đặt trong `./pixel_bot_animation_design.md`

## 3. Hardware

- Board: `STM32F429I-DISC1` (Discovery kit with STM32F429ZI MCU)
- MCU: `ARM Cortex-M4: STM32F429ZIT6U`

## 4. Constraints from the assignment

- Ưu tiên sử dụng trực tiếp các API hiển thị bậc cao của BSP (`BSP_LCD_Init`, `BSP_LCD_Clear`, `BSP_LCD_SetTextColor`, `BSP_LCD_DisplayStringAtLine`) thay vì tự viết lại driver thanh ghi LTDC/FMC từ đầu để tránh lỗi định thời phần cứng.
- Tốc độ làm tươi hiển thị LCD (Display Refresh Rate) phải được điều tiết hợp lý (10 Hz - 20 Hz, tương đương delay 50 - 100 ms) bằng Timer hoặc bộ đếm tick, tránh gọi lệnh xóa toàn màn hình (`BSP_LCD_Clear`) liên tục trong `while (1)` gây hiện tượng chớp giật hình ảnh (flickering).
- Không gọi các hàm xử lý chuỗi (`snprintf`) hay hàm vẽ màn hình LCD bên trong trình phục vụ ngắt (ISR/Callback).

## 5. What I already have working

## 6. What I want out of this

- [x] Full design dossier (hướng dẫn tích hợp bộ thư viện BSP LCD vào project CubeMX hiện tại hoặc cách cấu hình project có sẵn BSP) + reference `main.c` hoàn chỉnh kết hợp cả 4 ngoại vi (LCD + Gyro SPI5 + UART + TIM3).

## 7. Open questions I have
