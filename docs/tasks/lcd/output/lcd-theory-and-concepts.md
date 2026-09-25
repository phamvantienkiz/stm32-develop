# Theory & Concepts — lcd-gyro-bsp

> Lý thuyết và giải thích chi tiết các quyết định kiến trúc, đặc biệt là lý do đằng sau các cài đặt trong CubeMX và cách xử lý giao diện LCD tĩnh khi kết hợp với dữ liệu cảm biến.

| | |
|---|---|
| Task ID | `lcd-gyro-bsp` |
| Documents consulted | `UM1670` (Board User Manual), `RM0090` (Reference Manual) |

---

## 1. Facts established from the documentation

### `TFT LCD via LTDC & ILI9341`

| Question | Answer | Source |
|---|---|---|
| Màn hình được điều khiển như thế nào? | Vi điều khiển dùng ngoại vi **LTDC (LCD-TFT Display Controller)** để liên tục lấy dữ liệu điểm ảnh (pixel) từ bộ nhớ SDRAM và đẩy ra màn hình thông qua bus song song. Quá trình này dùng bộ điều khiển DMA2D phần cứng độc lập. | `[RM0090 §18]` |
| Tại sao lại có giao tiếp SPI5 ở đây? | Màn hình có một chip điều khiển tích hợp là ILI9341. Trước khi LTDC có thể bơm dữ liệu hình ảnh, MCU phải gửi các lệnh thiết lập (độ phân giải, chiều, định dạng màu) cho ILI9341 qua bus SPI5. Sau khi khởi tạo xong, SPI5 không còn dùng để xuất hình nữa. | `[UM1670 §6.10]` |
| Frame Buffer được lưu ở đâu? | Do một khung ảnh kích thước 240x320 với màu 16-bit tốn khoảng 153.6 KB (quá lớn so với RAM nội bộ của chip, đặc biệt khi dùng 2 layer), ST đã trang bị sẵn một chip SDRAM 8MB bên ngoài. Khung ảnh (Frame Buffer) được cấp phát tại địa chỉ `0xD0000000` (Bank 2 của bộ điều khiển bộ nhớ FMC). | `[UM1670 §6.10]` |

## 2. Explanation of concepts

### Tại sao BSP lại tiện lợi nhưng cũng "nguy hiểm" khi kết hợp với CubeMX?
Bộ thư viện BSP (Board Support Package) được ST viết sẵn để test mạch Discovery. Nó "bọc" lại toàn bộ quy trình cấu hình phức tạp của màn hình, SDRAM, và cảm biến. 
Trong file `stm32f429i_discovery.c`, ST định nghĩa sẵn các hàm khởi tạo phần cứng ở cấp thấp (MSP - MCU Support Package) như `HAL_SPI_MspInit`, `HAL_LTDC_MspInit`, `HAL_FMC_MspInit`. Tại đây, họ đã viết code cấp xung clock (bao gồm cả PLLSAI cho LCD) và gán chức năng chân GPIO (Alternate Function).

**Vấn đề xung đột:**
Nếu bạn dùng CubeMX cấu hình (bật) SPI5, LTDC hay FMC, CubeMX cũng sẽ tự động sinh ra các hàm `HAL_SPI_MspInit`, `HAL_LTDC_MspInit` trong file `stm32f4xx_hal_msp.c`. Ngôn ngữ lập trình C không cho phép 2 hàm cùng tên tồn tại. Khi build project, trình biên dịch sẽ báo lỗi `Multiple Definition` (định nghĩa nhiều lần).
**Giải pháp:** Đó là lý do trong bản hướng dẫn thiết lập CubeMX, ta chủ động **"nhường" (không kích hoạt SPI5, LTDC, FMC)** để file BSP toàn quyền quản lý chúng.

### Kỹ thuật in đè text (In-place text update) để chống nhấp nháy
Khi cần cập nhật giá trị 3 trục Gyro lên màn hình liên tục, vấn đề phổ biến là hiện tượng màn hình chớp nháy (flickering).
1. **Nguyên nhân gây nhấp nháy:** Việc gọi lệnh xóa toàn màn hình `BSP_LCD_Clear()` liên tục trong vòng lặp `while(1)` sẽ làm GPU phải tô màu đen (hoặc trắng) lên toàn bộ 153.6KB dữ liệu trước khi vẽ chữ lên. Mắt người sẽ nhìn thấy màn hình lúc có chữ lúc không, gây cảm giác nhấp nháy.
2. **Kỹ thuật in đè:** Khi gọi hàm vẽ chuỗi `BSP_LCD_DisplayStringAt()`, ta cài đặt thêm màu nền bằng `BSP_LCD_SetBackColor(LCD_COLOR_BLACK)`. Lúc này, hàm vẽ chữ sẽ tô nét chữ (màu xanh) và đồng thời **tô luôn phần nền (màu đen) đè lên vùng ký tự cũ**, mà không cần gọi hàm Clear.
3. **Xử lý số bị ngắn đi (Rác ký tự):** Nếu lần 1 bạn in "1234", lần 2 số giảm còn "99", nếu in đè bình thường, phần cuối của chữ số cũ ("34") vẫn còn sót lại, làm kết quả bị đọc nhầm thành "9934". Cách giải quyết triệt để nhất là **đệm khoảng trắng (Padding)**.
Dùng hàm `sprintf(buf, "%6d dps", val);`. `%6d` buộc chuỗi luôn chiếm đúng 6 chỗ trống (những vị trí thừa bên trái sẽ bị điền bằng dấu cách). Dấu cách này khi được in ra sẽ đè màu đen (BackColor) lên màn hình, "tẩy" đi số thừa cũ một cách hoàn hảo.

## 3. Design rationale

### Tại sao dùng `GYRO_IO_Write` thay vì `HAL_SPI_Transmit` như bài tập trước?
Trên bo mạch STM32F429I-DISC1, cảm biến Gyro L3GD20 và màn hình LCD ILI9341 **dùng chung một đường dây bus SPI5** (PF7, PF8, PF9).
Vì ta đã giao quyền cho BSP khởi tạo phần cứng, BSP đã có sẵn một handle nội bộ cho SPI5. 
ST đã thiết kế sẵn các hàm `GYRO_IO_Write()` và `GYRO_IO_Read()` trong file `stm32f429i_discovery.c`. Các hàm này sẽ dùng biến SPI nội bộ đó và tự động quản lý việc kéo chân Chip Select của Gyro (`PC1`) lên xuống đúng chuẩn.
Nếu ta tự khởi tạo thêm một biến `hspi5` riêng, ta có thể vô tình ghi đè cấu hình SPI5 mà BSP đã căn chỉnh cẩn thận, dẫn đến xung đột bus giữa LCD và Cảm biến. Sử dụng các hàm API có sẵn là an toàn và dễ bảo trì nhất.

## 4. What to learn from this task
Bài toán này dạy bạn nguyên lý quản lý **Shared Bus (Bus dùng chung)** và sự phân chia trách nhiệm giữa HAL (Hardware Abstraction Layer) và BSP (Board Support Package). Khi làm việc với các hệ thống nhúng lớn có sử dụng GUI (màn hình màu) hay RTOS, việc hiểu rõ module nào khởi tạo phần cứng nào là bắt buộc để không "dẫm chân" lên Code Generator của STM32CubeMX.
Việc giải quyết lỗi hiển thị nhấp nháy bằng thuật toán "khoảng trắng đệm" (padding) là một bài học kinh điển về lập trình giao diện mà bạn sẽ áp dụng lại cho tất cả các thư viện vẽ UI sau này (như TouchGFX hay LVGL).
