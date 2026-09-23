# Hướng dẫn Biên dịch, Nạp Code và Chạy Test (UART)

Tài liệu này hướng dẫn chi tiết các bước thực hiện trên STM32CubeIDE sau khi bạn đã cấu hình xong phần cứng và cập nhật xong file `main.c`.

---

## 1. Biên dịch và Nạp chương trình (Build & Debug)

1. **Biên dịch (Build):**
   * Nhấn biểu tượng cây búa (🔨 **Build**) trên thanh công cụ, hoặc ấn phím tắt `Ctrl + B`.
   * Chờ ở góc dưới cùng (Console) hiện dòng chữ `Build Finished. 0 errors, 0 warnings`.
2. **Nạp và Gỡ lỗi (Debug):**
   * Đảm bảo cáp USB đã cắm từ máy tính vào cổng **CN1 (ST-LINK)** trên bo mạch STM32F429I-DISC1.
   * Nhấn biểu tượng con bọ (🐛 **Debug**) trên thanh công cụ, hoặc phím `F11`.
   * Hộp thoại "Edit Configuration" (nếu có) hiện ra, cứ để mặc định và nhấn **OK**.
   * Trình biên dịch sẽ nạp code vào chip. Khi nạp xong, IDE sẽ tự động chuyển sang giao diện Debug (Perspective) và dừng lại ở dòng lệnh đầu tiên của hàm `main()`.
3. **Chạy chương trình (Resume):**
   * Nhấn biểu tượng nút Play màu xanh lá/vàng (▶️ **Resume**), hoặc phím `F8` để MCU chạy tự do. Lúc này mã nguồn của bạn đã chính thức được thực thi.

---

## 2. Mở Serial Terminal trực tiếp trong STM32CubeIDE

Để giao tiếp UART với bo mạch, bạn không cần dùng phần mềm bên ngoài (như PuTTY hay Hercules). STM32CubeIDE có sẵn tính năng này:

1. Ở nửa dưới màn hình (cùng chỗ với tab Problems, Debug...), chọn tab **Console**.
2. Phía bên phải của tab Console có một biểu tượng **Màn hình máy tính có dấu cộng màu xanh (Open Console)**. Bấm vào mũi tên trỏ xuống 🔽 ngay cạnh biểu tượng đó.
3. Chọn **3 Command Shell Console**.
4. Cửa sổ thiết lập hiện ra:
   * **Connection Type:** Chọn `Serial Port`.
   * Nhấn nút **New...** để tạo kết nối mới:
     - **Name:** Gõ tên bất kỳ (Ví dụ: `STLINK VCP`).
     - **Serial Port:** Chọn cổng COM của ST-LINK (ví dụ `COM3`, `COM4`...). Nếu không biết, hãy mở Device Manager của Windows > mục *Ports (COM & LPT)* để xem cổng nào là "STMicroelectronics STLink Virtual COM Port".
     - **Baud Rate:** Chọn `115200`.
     - Data Size: `8` | Parity: `None` | Stop Bits: `1`
     - Nhấn **Finish**.
5. Nhấn **OK**. Lúc này cửa sổ Console đã biến thành một Serial Terminal kết nối thẳng tới mạch STM32.

*(Mẹo: Bạn có thể nhấn nút chổi quét (Clear Console) hoặc Pin Console (ghim cửa sổ) để thao tác dễ hơn).*

---

## 3. Các bước Test chức năng (Test Scenarios)

### Test 1: Khởi động và Echo cơ bản
* Nhấn nút Reset màu đen trên bo mạch (hoặc tắt đi bật lại kết nối Console).
* **Kỳ vọng:** Bạn sẽ thấy dòng chữ `--- UART Command Ready ---` in ra.
* Gõ một chuỗi bất kỳ, ví dụ `Hello STM32` rồi ấn `Enter`.
* **Kỳ vọng:** Terminal sẽ lập tức trả về `Echo: Hello STM32`.

### Test 2: Bật đèn Xanh (LED_GREEN)
* Nhập chính xác chuỗi `LED_GREEN` (in hoa) và ấn `Enter`.
* **Kỳ vọng 1:** Terminal trả về dòng `Echo: LED_GREEN`.
* **Kỳ vọng 2:** Đèn xanh (LD3) trên bo mạch lập tức sáng lên.
* **Kỳ vọng 3:** Chờ đúng 5 giây, đèn xanh tự tắt và Terminal hiện thêm dòng chữ `DONE`.

### Test 3: Chứng minh Non-blocking (Không khóa CPU)
* Nhập chuỗi `LED_RED` và ấn `Enter`. Đèn đỏ (LD4) sẽ sáng.
* **Ngay lập tức** (trong khi đèn đỏ vẫn đang sáng), hãy gõ một lệnh khác, ví dụ `Test echo` rồi ấn `Enter`.
* **Kỳ vọng:** Terminal vẫn trả về `Echo: Test echo` ngay lập tức mà không phải đợi đèn đỏ tắt xong. Đèn đỏ sau đó vẫn tự tắt và báo `DONE` đúng chu kỳ 5 giây. 
*(Điều này chứng minh vòng lặp `while(1)` không bị khóa bởi các lệnh delay, hệ thống xử lý song song rất mượt mà).*

---

## 4. Xử lý sự cố thường gặp (Troubleshooting)

1. **Không thấy cổng COM trong phần New Connection:**
   * Do bạn chưa cài đặt Driver ST-LINK cho Windows. Hãy tải "STSW-LINK009" từ trang chủ ST và cài đặt.
2. **Gõ phím ấn Enter nhưng không thấy hiện chữ dội lại:**
   * Đảm bảo bạn đang gõ *bên trong* tab Console Terminal.
   * Ấn vào khung Terminal, nếu gõ mà terminal báo chữ màu xám không rõ ràng, có thể kết nối cổng COM bị chiếm. Nhấn nút Disconnect (hình phích cắm màu đỏ) rồi Connect lại (phích cắm xanh).
   * Đảm bảo bạn đã ấn phím **Enter** (tương đương ký tự `\r` hoặc `\n`), vì logic trong code cần ấn Enter để chốt dòng `line_ready = true`.
3. **Mạch đơ, không báo dòng System Ready:**
   * Hãy chắc chắn bạn đã nhấn phím Resume (F8) lúc Debug.
   * Nếu dùng chức năng HSE (thạch anh ngoại) thay vì HSI (clock nội), hãy kiểm tra lại cấu hình xem có nhầm Bypass thành Crystal không (lỗi này làm treo MCU).
