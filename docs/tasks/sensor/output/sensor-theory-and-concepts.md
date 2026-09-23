# Giải thích Lý thuyết: SPI Gyroscope, Thanh ghi & Chuyển đổi Dữ liệu

Tài liệu này giải đáp các khái niệm liên quan đến giao tiếp SPI, bản đồ thanh ghi của cảm biến L3GD20 (hoặc I3G4250D) và các thắc mắc về độ trôi (Gyro Drift) như yêu cầu.

## 1. Cơ chế SPI Full-Duplex và chân CS (Chip Select)

*   **SPI Full-Duplex:** Là giao tiếp 2 chiều đồng thời. Nghĩa là trong cùng một nhịp xung Clock (SCK), khi Master (STM32) đẩy 1 byte ra chân MOSI (Master Out Slave In) thì nó cũng đồng thời nhận lại 1 byte từ chân MISO (Master In Slave Out). Đó là lý do thư viện ST cung cấp hàm `HAL_SPI_TransmitReceive()`. Ngay cả khi bạn chỉ muốn "Đọc", bạn vẫn phải "Gửi" một byte rác (Dummy byte) để tạo xung Clock kéo dữ liệu về.
*   **Vai trò của CS (Chip Select - `PC1`):** SPI là bus có thể gắn nhiều cảm biến chung SCK, MISO, MOSI. Chân CS dùng để "gọi tên" thiết bị. 
    * Trạng thái nghỉ: CS luôn ở mức Cao (`1`).
    * Khi muốn nói chuyện với Gyro: Kéo CS xuống Thấp (`0`).
    * Sau khi nói chuyện xong: Kéo CS lên lại mức Cao (`1`) để kết thúc giao dịch. Trình tự này bắt buộc phải có bao bọc quanh hàm `HAL_SPI_...`.

## 2. Bản đồ thanh ghi của L3GD20 / I3G4250D

Cảm biến không gửi thẳng góc quay cho bạn, nó chứa dữ liệu trong các ngăn xếp (Thanh ghi).
*   **WHO_AM_I (Địa chỉ `0x0F`):** Đây là căn cước công dân của chip. Đọc thanh ghi này luôn trả về `0xD4` (với L3GD20) hoặc `0xD3` (với I3G4250D). Việc đầu tiên khi khởi động luôn là đọc thử thanh ghi này để kiểm tra xem SPI nối đã đúng chưa, chip có sống không.
*   **CTRL_REG1 (Địa chỉ `0x20`):** Thanh ghi điều khiển nguồn. Mặc định chip ở chế độ ngủ sâu (Power Down). Ta phải ghi giá trị `0x0F` vào đây (Bit PD=1 để thức dậy, và 3 bit Zen, Yen, Xen = 1 để bật đo cả 3 trục).
*   **Các thanh ghi dữ liệu (Từ `0x28` đến `0x2D`):** Dữ liệu 3 trục X, Y, Z mỗi trục chiếm 16-bit (2 byte). Nên có tổng cộng 6 thanh ghi: `OUT_X_L`, `OUT_X_H`, `OUT_Y_L`, `OUT_Y_H`, `OUT_Z_L`, `OUT_Z_H`.

**Cơ chế Bit đọc và Auto-increment (Bit `0xC0`):**
Khi giao tiếp SPI với họ chip này, Byte đầu tiên gửi đi phải chứa Địa chỉ thanh ghi + Bit lệnh:
*   Bit thứ 7 (MSB): Lệnh Đọc = `1`, Lệnh Ghi = `0`. (Tức là cộng thêm `0x80`).
*   Bit thứ 6: Auto-increment (Tự động tăng địa chỉ) = `1` (Tức là cộng thêm `0x40`).
👉 Để đọc 6 byte liên tiếp bắt đầu từ `OUT_X_L` (Địa chỉ `0x28`), Byte gửi đi sẽ là: `0x28 | 0x80 | 0x40 = 0xE8`. Nhờ đó ta chỉ cần gọi lệnh SPI nhận 6 byte 1 lần duy nhất, cực kỳ tối ưu.

## 3. Ghép byte và Ép kiểu (Type Casting) số có dấu

Cảm biến trả về 2 byte riêng lẻ cho mỗi trục. Ví dụ Trục X có phần thấp là `L` và phần cao là `H`.
Để tạo ra con số hoàn chỉnh:
```c
int16_t x_val = (int16_t)( (H << 8) | L );
```
*   `H << 8`: Dịch phần cao sang trái 8 bit (tạo không gian cho phần thấp).
*   `| L`: Ghép phần thấp vào bằng phép toán OR.
*   `(int16_t)`: Cực kỳ quan trọng. Cảm biến lưu góc dưới dạng số bù 2 (Two's Complement). Nếu xoay ngược chiều, nó sẽ trả ra số âm. Ép về kiểu số nguyên có dấu 16-bit giúp C hiểu đây là số âm chứ không phải một số dương khổng lồ. Con số xuất ra mang đơn vị $dps$ (độ trên giây - degrees per second) sau khi nhân với hệ số độ nhạy.

## 4. Giải đáp Open Questions

**Tốc độ SPI5 có ảnh hưởng độ chính xác không?**
Không. Giao tiếp SPI chỉ là cái "ống nước" truyền số Digital. Chỉ cần tốc độ không vượt quá khả năng chịu đựng của mạch điện (Max của L3GD20 là 10 MHz), thì đọc 1 MHz hay 8 MHz dữ liệu đều chính xác 100% như nhau. Dữ liệu lỗi (nhiễu) là do bản thân MEMS Sensor, không phải do tốc độ SPI.

**Giá trị X, Y, Z có bằng 0 tuyệt đối khi nằm im không? (Zero-rate level / Gyro Drift)**
Chắc chắn là **KHÔNG**. Do đặc tính cơ học từ vi cơ điện tử (MEMS) và nhiễu nhiệt, cảm biến khi nằm im trên bàn vẫn trả về một hằng số dao động rác (ví dụ X luôn lệch khoảng +15, Y lệch -8...). Hiện tượng này gọi là Zero-rate Offset (Bias).
*Có cần thuật toán bù trừ không?* Rất cần nếu bạn muốn tính góc xoay tuyệt đối. Trong hệ thống chuyên nghiệp, người ta gọi 1 hàm Calibration lúc khởi động: Đọc 100 mẫu khi mạch đứng yên, tính trung bình cộng, sau đó lấy giá trị đo được trừ đi giá trị trung bình này trước khi dùng. (Trong bài lab này ta sẽ in giá trị thô để bạn tự quan sát hiện tượng nhiễu này).
