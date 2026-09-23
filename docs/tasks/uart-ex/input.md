## 1. The assignment, verbatim

**Yêu cầu**: Thực hành truyền nhận dữ liệu với UART

## 2. My understanding of it, in my own words

**Diễn giải yêu cầu**:

- Triển khai module **UART/USART** sử dụng STM32 HAL Library kết hợp cơ chế ngắt nhận (Interrupt-driven Rx) để giao tiếp 2 chiều với máy tính thông qua cổng ST-Link Virtual COM Port (VCP).
- Không cần ứng dụng bên thứ 3 phức tạp, có thể đọc và tương tác trực tiếp qua Serial Terminal tích hợp của STM32CubeIDE.
- Chi tiết hành vi chương trình cần thực hiện:
  1. **Echo chuỗi cố định & biến thiên:** Nhận luồng ký tự qua ngắt UART từng byte, tích lũy đến khi gặp ký tự kết thúc dòng (`\r` hoặc `\n`), sau đó gửi ngược lại chuỗi đã nhận (Echo) lên Terminal máy tính.
  2. **Điều khiển ngoại vi (Command Parsing):**
     - Khi nhận được chuỗi `"LED_GREEN"`: Bật sáng đèn LED xanh (LD3) trong 5 giây, sau đó tắt và truyền phản hồi chuỗi `"DONE\r\n"` về máy tính.
     - Khi nhận được chuỗi `"LED_RED"`: Bật sáng đèn LED đỏ (LD4) trong 5 giây, sau đó tắt và truyền phản hồi chuỗi `"DONE\r\n"` về máy tính.
  3. Cấu trúc chương trình phải non-blocking ở tầng nhận ngắt (chỉ lưu byte và set flag trong ngắt, đưa việc delay/xử lý nặng ra vòng lặp `while (1)`).

## 3. Hardware

- Board: `STM32F429I-DISC1` (Discovery kit with STM32F429ZI MCU)
- MCU: `ARM Cortex-M4: STM32F429ZIT6U`
- Peripheral Connections:
  - **USART1 (VCP):**
    - Pin `PA9`: USART1_TX (kết nối ST-LINK V2-B RX qua cầu hàn SB11)
    - Pin `PA10`: USART1_RX (kết nối ST-LINK V2-B TX qua cầu hàn SB15)
    - Baud rate: `115200 bps`, `8N1` (8 data bits, no parity, 1 stop bit) - Default
  - **User LEDs:**
    - `LD3` (Green LED): Pin `PG13` (Active HIGH)
    - `LD4` (Red LED): Pin `PG14` (Active HIGH)

## 4. Constraints from the assignment

- Sử dụng STM32 HAL Driver chuẩn do STM32CubeMX / STM32CubeIDE sinh ra.
- Không để hàm xử lý nặng/delay (như `HAL_Delay(5000)`) bên trong hàm phục vụ ngắt (ISR/Callback).

## 5. What I already have working

- Đã cấu hình và chạy thành công bài toán cơ bản: Blink LED LD3 (Green - PG13) nhấp nháy chu kỳ 1s (1s ON, 1s OFF) trên bo mạch thật bằng STM32CubeIDE.
- Nắm được cách nạp code và debug qua giao tiếp ST-Link on-board.

## 6. What I want out of this

- [x] Full design dossier (CubeMX configuration) + reference `main.c` (default)
- [x] Extra explanation of <peripheral/concept> — I have not used it before
  - Cụ thể:
    - Cơ chế ngắt nhận từng byte của UART trong HAL: `HAL_UART_Receive_IT` và callback `HAL_UART_RxCpltCallback`.
    - Cách tổ chức bộ đệm (Ring Buffer / Line Buffer) và kỹ thuật đồng bộ giữa Interrupt Context và Main Thread (sử dụng cờ `volatile`).
    - Cách mở Serial Terminal tích hợp ngay bên trong STM32CubeIDE để gửi/nhận dữ liệu trực tiếp mà không cần cài app rời.

## 7. Open questions I have

- Cách xử lý chống tràn bộ đệm khi Terminal người dùng gửi chuỗi dài hơn kích thước buffer định sẵn.
- Ký tự xuống dòng của các phần mềm Terminal khác nhau (CR `\r`, LF `\n`, hoặc CRLF `\r\n`) ảnh hưởng như thế nào đến việc nhận diện kết thúc lệnh và cách xử lý triệt để.
