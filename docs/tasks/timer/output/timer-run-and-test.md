# Hướng dẫn Biên dịch, Nạp Code và Chạy Test (Timer + UART)

Tài liệu này hướng dẫn chi tiết các bước nghiệm thu chức năng phối hợp giữa Hardware Timer (TIM3) và UART.

---

## 1. Biên dịch và Nạp chương trình (Build & Debug)

1. **Biên dịch (Build):**
   * Nhấn biểu tượng cây búa (🔨 **Build**) trên thanh công cụ. Đảm bảo Console báo `0 errors`.
2. **Nạp và Gỡ lỗi (Debug):**
   * Đảm bảo cáp USB đã cắm vào mạch.
   * Nhấn biểu tượng con bọ (🐛 **Debug**). Chờ quá trình nạp xong.
3. **Chạy chương trình (Resume):**
   * Nhấn biểu tượng nút Play màu xanh lá (▶️ **Resume**) hoặc phím `F8` để mạch bắt đầu chạy thực thi mã.

---

## 2. Công cụ Giám sát (IDE Views)

### Mở Serial Terminal
1. Chọn tab **Console** ở nửa dưới màn hình.
2. Bấm vào mũi tên 🔽 cạnh biểu tượng Open Console, chọn **3 Command Shell Console**.
3. Connection Type: `Serial Port` > Tạo kết nối **New...**.
4. Chọn đúng cổng COM của ST-LINK, nhập Baud Rate là `115200`. Ấn OK.
*(Lưu ý: Mở Terminal xong, hãy bấm nút Reset màu đen trên bo mạch để thấy lời chào khởi động).*

### Mở Live Expressions (Tùy chọn)
Để giám sát biến thời gian đếm dưới nền mà không cần in ra terminal:
1. Vào menu **Window > Show View > Live Expressions**.
2. Thêm biến `uptime_seconds`. Bạn sẽ thấy biến này tăng đều đặn 1 đơn vị mỗi giây một cách chính xác.

---

## 3. Các bước Test chức năng (Test Scenarios)

### Test 1: Khởi động và System Heartbeat
* **Hành động:** Nhấn nút Reset màu đen trên bo mạch.
* **Kỳ vọng:** 
  1. Lập tức in ra dòng chữ `--- HW Timer & UART Ready ---`.
  2. Sau đó, **cứ đúng 1 giây**, màn hình sẽ tự động nảy ra một dòng chữ: `[HW_TIMER] Heartbeat... Uptime: X s`. Số X tăng dần đều.
*(Nếu bạn lấy đồng hồ bấm giờ ra đo, cứ mỗi 60 giây trôi qua ngoài đời thực thì biến Uptime cũng sẽ điểm đúng 60, sai số cực kỳ nhỏ. Điều này chứng minh cấu hình HSI và Clock Tree đã chính xác).*

### Test 2: Kế thừa lệnh cũ (Non-blocking check)
* **Hành động:** Trong lúc terminal đang in Heartbeat ầm ầm, bạn gõ `LED_GREEN` (nhớ tích CR LF trên Hercules hoặc ấn Enter nếu dùng IDE) rồi gửi đi.
* **Kỳ vọng:** 
  * Đèn xanh trên mạch sáng ngay lập tức.
  * Màn hình vẫn tiếp tục in ra các dòng Heartbeat 1 giây/lần xen kẽ bình thường. Chờ đủ 5 giây, đèn tắt và in ra chữ `DONE`.
  *(Điều này chứng tỏ UART và ngắt Timer hoạt động hòa bình, không chặn họng nhau).*

### Test 3: Lấy thời gian bằng lệnh `GET_TIME`
* **Hành động:** Gõ lệnh `GET_TIME` rồi ấn Gửi.
* **Kỳ vọng:** MCU lập tức trả về `--> Current Uptime: X seconds` bằng đúng số giây hiện tại.

### Test 4: Truy xuất chẩn đoán phần cứng bằng lệnh `CHECK_HW`
* **Hành động:** Gõ lệnh `CHECK_HW` rồi ấn Gửi.
* **Kỳ vọng:** Màn hình sẽ in ra thông số đọc *trực tiếp từ tim đen* của thanh ghi phần cứng TIM3:
  `--> HW REG: CNT=xxx, PSC=15999, ARR=999`
  *(Chỉ số CNT sẽ là một số ngẫu nhiên từ 0 đến 999 tùy thuộc vào khoảnh khắc bạn ấn nút gửi).*

---

## 4. Xử lý sự cố thường gặp (Troubleshooting)

1. **Heartbeat in ra quá nhanh (chạy lướt vèo vèo) hoặc quá chậm:**
   * **Nguyên nhân:** Cấu hình Clock Tree sai, nhịp đập thực tế không phải là 16 MHz.
   * **Khắc phục:** Mở lại file `.ioc` > Clock Configuration > Đảm bảo HSI (16 MHz) được chọn và bộ chia APB1 là `/1`.
2. **Gõ chữ `LED_GREEN` nhưng bị mất ký tự (ví dụ gõ LED_GREEN nhưng MCU nhận thành ED_GEN):**
   * **Nguyên nhân:** Xung đột mức độ ưu tiên ngắt (Preemption Priority). Ngắt Timer đang chặn ngắt UART.
   * **Khắc phục:** Vào NVIC Settings, phải set Priority của `USART1` là số 1, và của `TIM3` là số 2. Nhỏ hơn ưu tiên cao hơn.
3. **Mọi thứ đơ cứng, không in ra lời chào khởi động:**
   * Hãy chắc chắn bạn đã gọi `HAL_TIM_Base_Start_IT(&htim3);` và `HAL_UART_Receive_IT(...)` trong khối `USER CODE BEGIN 2`. Nếu thiếu, MCU không bao giờ kích hoạt ngắt.
