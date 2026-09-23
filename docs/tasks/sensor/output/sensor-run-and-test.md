# Hướng dẫn Biên dịch, Nạp Code và Chạy Test (Sensor + UART + Timer)

Tài liệu này hướng dẫn chi tiết các bước nghiệm thu chức năng giao tiếp liên hoàn giữa SPI (cảm biến Gyroscope), Hardware Timer và UART.

---

## 1. Biên dịch và Nạp chương trình (Build & Debug)

1. **Biên dịch (Build):** Nhấn biểu tượng cây búa (🔨). Đảm bảo Console báo `0 errors`.
2. **Nạp và Gỡ lỗi (Debug):** Nhấn biểu tượng con bọ (🐛).
3. **Chạy chương trình (Resume):** Nhấn biểu tượng nút Play màu xanh lá (▶️) hoặc phím `F8` để chạy.

---

## 2. Công cụ Giám sát

1. Chọn tab **Console** > **3 Command Shell Console**.
2. Connection Type: `Serial Port` > Tạo kết nối **New...**.
3. Cổng COM: `ST-LINK VCP`, Baud Rate: `115200`. Nhấn OK.
4. Đảm bảo bạn đã TÍCH CHỌN 2 ô `CR` và `LF` trên phần mềm Hercules hoặc bật gửi `\r\n` nếu dùng IDE Terminal.

---

## 3. Các bước Test chức năng (Test Scenarios)

### Test 1: Khởi động và Xác nhận SPI kết nối
* **Hành động:** Nhấn nút Reset màu đen trên bo mạch.
* **Kỳ vọng:** Màn hình in ra dòng chữ `--- HW Timer, UART & Gyroscope Ready ---`. Nếu in ra dòng `Gyro Init FAILED!`, có nghĩa là giao tiếp SPI đã thất bại (sai chân, cấu hình sai Mode SPI hoặc mạch đứt). 

### Test 2: Đọc dữ liệu Gyroscope 1 lần (Single Polling)
* **Hành động:** Gõ lệnh `GET_GYRO` và ấn Gửi (Nhớ có CR LF).
* **Kỳ vọng:** Trả về `Gyro -> X: [số] | Y: [số] | Z: [số] dps`. Thử xoay nhẹ bo mạch trên mặt bàn và gửi lệnh lại nhiều lần để thấy sự thay đổi. Khi để yên, các số không bằng 0 mà dao động (Gyro Drift).

### Test 3: Kích hoạt chế độ phát liên tục (Stream)
* **Hành động:** Gõ lệnh `STREAM_ON` và ấn Gửi.
* **Kỳ vọng:** 
  1. Trả về `Gyro Stream STARTED`.
  2. Cứ mỗi 200 mili-giây, màn hình sẽ "phun" ra 1 dòng: `Stream -> X: ... | Y: ... | Z: ...`.
  3. Cầm bo mạch xoay mạnh các hướng trong không gian, bạn sẽ thấy X, Y, Z biến thiên mạnh lên đến hàng ngàn.

### Test 4: Chứng minh khả năng Multi-tasking siêu đỉnh (Non-blocking)
* **Hành động:** Trong lúc dòng chữ Stream đang phụt ra liên tục (5 dòng/giây), xen kẽ với dòng Heartbeat Uptime (1 dòng/giây), bạn hãy gõ lệnh `LED_GREEN` rồi ấn Gửi.
* **Kỳ vọng:** 
  * Đèn xanh trên mạch lập tức sáng lên.
  * Việc Stream dữ liệu cảm biến và Heartbeat Timer VẪN TIẾP TỤC CHẠY đều đặn như vắt chanh, không hề bị khựng lại hay đơ.
  * Sau đúng 5 giây, đèn tắt, chữ `DONE` hiện ra chen giữa dòng thác dữ liệu.

### Test 5: Tắt Stream
* **Hành động:** Gõ lệnh `STREAM_OFF` và ấn Gửi.
* **Kỳ vọng:** Màn hình trả về `Gyro Stream STOPPED` và ngừng phun dữ liệu Gyro (chỉ còn lại Heartbeat 1s/lần).

---

## 4. Xử lý sự cố thường gặp (Troubleshooting)

1. **Báo lỗi `Gyro Init FAILED!` lúc khởi động:**
   * **Nguyên nhân 1:** Chưa cấu hình chân `PC1` là `GPIO_Output` với mức `High`. Cảm biến bị khóa (ngủ).
   * **Nguyên nhân 2:** Gán nhầm chân SPI. Chân đúng phải là PF7, PF8, PF9. Không phải là PA5, PA6, PA7...
2. **Giá trị X, Y, Z toàn ra 0 hoặc 255 không đổi dù lắc mạch rất mạnh:**
   * **Nguyên nhân:** Đang cấu hình sai SPI Mode. Hãy vào CubeMX kiểm tra lại SPI5 phải là: `CPOL = Low`, `CPHA = 1 Edge` (hoặc CPOL High, CPHA 2 Edge). Nếu để CPOL Low, CPHA 2 Edge dữ liệu sẽ bị lệch bit.
3. **Vừa bật `STREAM_ON`, chương trình treo luôn hoặc mất chữ:**
   * **Nguyên nhân:** Ưu tiên ngắt của UART không đủ cao hoặc tràn buffer UART. Hãy chắc chắn NVIC của UART đang ở mức 1. Cổng COM3 có thể xử lý tốt 115200bps, nhưng nếu gọi hàm in chuỗi liên tục quá nhanh (ví dụ giảm delay stream xuống còn 5ms) sẽ làm hàm Transmit bị chặn (block). Mức 200ms là cực kỳ an toàn.
