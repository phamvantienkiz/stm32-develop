# Theory & Concepts — UART Echo & Command Parsing

> Required structure for the secondary deliverable. This file isolates theoretical explanations, documentation facts, register details, and specific conceptual questions raised by the user.

| | |
|---|---|
| Task ID | `uart-ex` |
| Documents consulted | `UM1670 Rev 6, RM0090 Rev 19` |

---

## 1. Facts established from the documentation

### `USART1`

| Question | Answer | Source |
|---|---|---|
| Which bus and clock feeds it? | Nằm trên bus **APB2**. Clock được bật qua bit `USART1EN` trong thanh ghi `RCC_APB2ENR`. Tốc độ clock tối đa của APB2 là 90 MHz. Trong project này chạy mặc định HSI nên PCLK2 = 16 MHz. | `[RM0090 §2.3 p.65; §6.3.14 p.246]` |
| What starts it, where does data flow, which flag says done? | Ghi dữ liệu vào thanh ghi `USART_DR` sẽ tự đẩy ra TX. Dữ liệu nhận từ RX cũng tự chui vào `USART_DR`. Cờ `TXE` (Transmit data register empty) báo hiệu có thể ghi byte tiếp theo. Cờ `RXNE` (Read data register not empty) báo hiệu có byte mới vừa nhận xong. | `[RM0090 §30.6.1 p.1007]` |
| Which events raise an interrupt / DMA request? | USART có thể sinh ngắt khi `RXNE` = 1 (nhận được 1 byte), `TXE` = 1 (sẵn sàng gửi), `TC` = 1 (đã gửi xong), v.v. Bật ngắt RXNE thông qua bit `RXNEIE` trong `USART_CR1`. | `[RM0090 §30.6.4 p.1011]` |
| Relevant registers and bits | - `USART_SR` (Status Register): Chứa cờ `RXNE`, `TXE`.<br>- `USART_DR` (Data Register): Thanh ghi chứa dữ liệu 8-bit TX/RX.<br>- `USART_BRR`: Thanh ghi thiết lập tốc độ Baud.<br>- `USART_CR1`: Control Register 1, chứa bit `UE` (UART Enable), `TE`, `RE`, `RXNEIE`. | `[RM0090 §30.6 p.1007]` |

## 2. Explanation of concepts

Dưới đây là giải đáp chi tiết cho các câu hỏi và thắc mắc bạn nêu trong `input.md`:

### Cơ chế ngắt nhận từng byte của UART trong HAL: `HAL_UART_Receive_IT` và `HAL_UART_RxCpltCallback`
- **Hoạt động:** Không giống như `HAL_UART_Receive` sẽ khóa (block) CPU chờ đến khi nhận đủ byte, `HAL_UART_Receive_IT` chỉ báo cho phần cứng biết "Hãy gọi ngắt khi nào nhận đủ số lượng byte này". Sau đó nó trả CPU về vòng lặp chính ngay lập tức.
- **Sự kiện ngắt:** Khi chân RX nhận được 1 byte (cờ `RXNE` bật), vi điều khiển nhảy vào hàm phục vụ ngắt chung `USART1_IRQHandler`, hàm này gọi vào HAL. HAL sẽ lấy byte từ `USART_DR` cất vào biến bạn đã chỉ định, rồi gọi hàm `HAL_UART_RxCpltCallback`.
- **Lưu ý tử huyệt:** Khi nhảy vào `HAL_UART_RxCpltCallback`, ngắt nhận đã bị tắt (disable) đối với luồng đó. Nếu bạn muốn hứng byte tiếp theo, bạn **phải gọi lại lệnh `HAL_UART_Receive_IT`** ở cuối hàm Callback. Nếu quên, MCU sẽ bị điếc sau byte đầu tiên.

### Cách tổ chức Line Buffer và đồng bộ bằng cờ `volatile`
- **Khái niệm Line Buffer:** Là một mảng ký tự lưu dần các byte nhận được (`rx_buffer[rx_index++]`). Nó được gọi là Line (Dòng) vì ta quy định khi gặp ký tự `\r` hoặc `\n` thì đóng chuỗi lại bằng ký tự Null `\0`.
- **Tại sao cần `volatile`:** Hàm ngắt (Interrupt) có thể xảy ra bất cứ lúc nào, "đâm ngang" vào vòng lặp `while(1)`. Trình biên dịch C rất thông minh, nó có thể tối ưu hóa và cho rằng biến `line_ready` trong vòng `while` không bao giờ đổi giá trị (vì nó không thấy đoạn code nào trong `while` sửa biến đó). Hậu quả là vòng lặp sẽ bị kẹt. Từ khóa `volatile` cấm trình biên dịch tối ưu hóa biến này, ép nó phải đọc lại RAM mỗi lần kiểm tra.
- **Vấn đề chống tràn bộ đệm:** Nếu người dùng gõ chuỗi 150 ký tự trong khi buffer chỉ có 128, thì `rx_index` sẽ chạy lố mảng, ghi đè vào biến khác gây crash (Lỗi tràn bộ đệm - Buffer Overflow). Cách giải quyết: Trong callback, luôn phải kiểm tra `if (rx_index < 127)` thì mới cho phép ghi vào mảng.

### Ký tự xuống dòng của các phần mềm Terminal
- Windows (PuTTY, TeraTerm) thường gửi `\r` (Carriage Return - CR, ASCII 13).
- Linux / Mac thường gửi `\n` (Line Feed - LF, ASCII 10).
- Vài phần mềm lại gửi cả cặp `\r\n`.
- **Cách xử lý triệt để:** Bắt cả `\r` và `\n` coi như là tín hiệu chốt chuỗi. Khi gặp một trong hai, ta chốt chuỗi (`line_ready = true`) và thay nó bằng `\0`. Nếu byte tiếp theo lại là `\n` mà `rx_index` đang bằng 0 (vì main đã reset) thì bỏ qua nó để tránh báo chuỗi rỗng.

### Cách mở Serial Terminal tích hợp trong STM32CubeIDE
Để không cần dùng PuTTY hay app thứ ba, bạn có thể dùng Console nội bộ của IDE:
1. Mở STM32CubeIDE, nhìn xuống bảng Console (cùng chỗ với thẻ Build/Problems).
2. Tìm icon hình **màn hình máy tính có dấu cộng màu xanh** (Open Console) -> Bấm vào mũi tên trỏ xuống bên cạnh.
3. Chọn **Command Shell Console**.
4. Chọn **Connection Type: Serial Port**.
5. Nhấn **New...**, gõ tên kết nối (vd: STLINK VCP). Chọn cổng COM tương ứng (COM3, COM4...), chỉnh Baud rate là **115200**, Data size 8, Parity None, Stop 1.
6. Nhấn Finish rồi OK. Console sẽ biến thành terminal, bạn có thể gõ chữ trực tiếp vào đó. Khi code gọi `HAL_UART_Transmit`, chữ sẽ hiện lên đó.

## 3. Design rationale

### Tại sao lại dùng Polling (blocking) cho phần Truyền (Tx) mà không dùng Ngắt (Tx_IT)?
- **Lý do:** Gửi một chuỗi `"DONE\r\n"` ở tốc độ 115200 baud tốn khoảng chưa tới 1 mili-giây. Trong ứng dụng nháy đèn LED cơ bản này, trễ 1 ms không ảnh hưởng đến mắt người nhìn. Dùng `HAL_UART_Transmit` (chờ gửi xong mới đi tiếp) làm code cực kì đơn giản, dễ đọc.
- **So sánh:** Nếu dùng ngắt truyền `HAL_UART_Transmit_IT`, bạn sẽ phải quản lý thêm mảng buffer truyền (phải đảm bảo mảng không bị giải phóng khi đang truyền ngầm). Điều đó làm bài toán phức tạp không cần thiết.

## 4. What to learn from this task

Kỹ năng quan trọng nhất là **Event-driven programming (Lập trình hướng sự kiện)** với cờ (flag). Thay vì bắt CPU phải đứng chờ ở lệnh nhận UART, ta thả cho CPU chạy rảnh rỗi trong `while(1)`. Khi phần cứng (Interrupt) báo có dữ liệu bằng cách dựng cờ `line_ready = true`, CPU mới chuyển hướng sang xử lý. Đây là tiền đề bắt buộc trước khi học lên RTOS (Hệ điều hành thời gian thực).

## 5. Possible extensions

1. **Non-blocking Transmit:** Thử đổi `HAL_UART_Transmit` thành `HAL_UART_Transmit_IT` để xem sự khác biệt. (Gợi ý: cần dùng mảng toàn cục cho buffer truyền).
2. **Ring Buffer:** Line buffer hiện tại có nhược điểm: Nếu main loop xử lý lệnh "LED_GREEN" mất nhiều thời gian, mà ngắt vẫn tiếp tục nhận chữ gõ vào, các chữ đó sẽ bị đè hoặc bị rớt. Hãy nâng cấp lên một cấu trúc dữ liệu Ring Buffer (Circular Buffer) để hứng luồng ký tự mà không bao giờ sợ mất.
