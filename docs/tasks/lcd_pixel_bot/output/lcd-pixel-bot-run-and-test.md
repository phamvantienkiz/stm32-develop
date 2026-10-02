# Hướng Dẫn Biên Dịch, Nạp Code và Chạy Test — Animated Pixel Bot

Tài liệu này hướng dẫn chi tiết các bước thực hiện trên **STM32CubeIDE** để biên dịch, nạp và kiểm thử nhân vật Pixel Bot trên bo mạch **STM32F429I-DISC1**.

---

## 1. Biên dịch và Nạp chương trình (Build & Flash)

### 1.1 Biên dịch (Build)
1. Trong cửa sổ STM32CubeIDE, chọn Project `stm32-lcd-pixel-bot`.
2. Nhấn tổ hợp phím **`Ctrl + B`** (hoặc biểu tượng 🔨 **Build** trên thanh công cụ).
3. Quan sát tab **Console** ở góc dưới màn hình. Đảm bảo kết quả trả về:
   ```text
   Build Finished. 0 errors, 0 warnings.
   ```
   *(Nếu xuất hiện lỗi, hãy xem ngay mục 4: Xử lý sự cố thường gặp).*

### 1.2 Nạp và Khởi chạy (Debug & Run)
1. Cắm cáp USB (chuẩn Mini-B) từ máy tính vào cổng **USB ST-LINK** (CN1 ở cạnh trên bo mạch).
2. Nhấn phím **`F11`** (hoặc biểu tượng con bọ 🐛 **Debug**).
3. Nếu có hộp thoại xác nhận cấu hình nạp, giữ nguyên mặc định và nhấn **OK**.
4. Chờ trình nạp hoàn tất ghi Flash. IDE sẽ chuyển sang giao diện Debug và tự động dừng lại ở dòng đầu tiên của hàm `main()`.
5. Nhấn phím **`F8`** (hoặc biểu tượng nút Play ▶️ **Resume**) để MCU bắt đầu chạy mã nguồn.

---

## 2. Công cụ Giám sát & Điều khiển (IDE Views)

### 2.1 Mở Serial Terminal (Nhận Log & Gửi phím điều khiển)
1. Ở nửa dưới màn hình CubeIDE, chuyển sang tab **Console**.
2. Bấm vào mũi tên trỏ xuống 🔽 bên cạnh biểu tượng màn hình máy tính có dấu cộng xanh (**Open Console**).
3. Chọn **3 Command Shell Console**.
4. Cửa sổ hiện ra, cấu hình:
   - **Connection Type:** Chọn `Serial Port`.
   - Bấm nút **New...**:
     + **Name:** Đặt tên `STLINK VCP`.
     + **Serial Port:** Chọn cổng COM của mạch ST-LINK (ví dụ: `COM5`, `COM7`).
     + **Baud Rate:** `115200`.
     + **Data Size:** `8`.
     + **Parity:** `None`.
     + **Stop Bits:** `1`.
   - Bấm **Finish** $\rightarrow$ bấm **OK**.
5. Màn hình Console sẽ bắt đầu nhận các bản tin log `[BOT]` và sẵn sàng nhận phím gõ từ bạn.

### 2.2 Xem biến trực tiếp (Live Expressions)
1. Vào menu **Window $\rightarrow$ Show View $\rightarrow$ Live Expressions**.
2. Bấm vào dấu `+` màu vàng và nhập các biến sau:
   - `anim_cur->name` : Tên animation đang diễn ra (kiểu chuỗi: "IDLE", "JUMP", "DANCE"...).
   - `anim_idx` : Chỉ số frame hiện tại của animation.
   - `flag_btn` : Cờ phát hiện bấm nút PA0.

---

## 3. Kịch bản kiểm thử (Test Scenarios)

### Test 1: Khởi động và Hoạt cảnh BOOT
- **Thao tác:** Bấm nút đen **Reset (B2)** trên kit hoặc nạp lại code.
- **Kỳ vọng quan sát:**
  - Màn hình LCD 2.4" bật sáng, toàn bộ nền chuyển sang màu tím đậm công nghệ (`#0D0B1E`).
  - Bot bắt đầu ở tư thế đang ngủ (mắt nhắm, chữ Zzz bay lên).
  - Sau đó bot hé mắt $\rightarrow$ nhìn sang trái $\rightarrow$ nhìn sang phải $\rightarrow$ vươn vai (thân giãn ra, miệng chữ O) $\rightarrow$ vẫy tay chào với hai má hồng dễ thương.
  - Đèn LED xanh **PG13** bắt đầu nhấp nháy đều đặn theo tần số quét khung hình (30 Hz).
  - Terminal UART in ra dòng chữ chào mừng:
    ```text
    ==============================================
      STM32F429I-DISC1 ANIMATED PIXEL BOT INITIALIZED
    ==============================================
    [BOT] Action: BOOT (Priority 4)
    ```

### Test 2: Trạng thái IDLE, Thở và Chớp mắt tự nhiên
- **Thao tác:** Giữ nguyên bo mạch, không chạm vào nút bấm trong 10 giây.
- **Kỳ vọng quan sát:**
  - Sau khi kết thúc hoạt cảnh BOOT, bot chuyển sang chế độ **IDLE**.
  - Hai cánh tay bot khẽ nâng lên hạ xuống theo nhịp thở êm ái (chu kỳ 1.8 giây).
  - Sau mỗi khoảng 2.5 đến 5.5 giây ngẫu nhiên, mắt bot tự động chớp 3 frame (khép mắt lại rồi mở ra trong 180 ms) cực kỳ tự nhiên.
  - Thỉnh thoảng bot tự liếc nhìn sang trái rồi sang phải.
  - Không có hiện tượng rách hình (tearing) hay chớp đen màn hình (flickering).

### Test 3: Chu kỳ các hành động ngẫu nhiên liên tiếp (Random Action Loop)
- **Thao tác:** Quan sát bot tự hoạt động liên tục trong 1 - 2 phút.
- **Kỳ vọng quan sát:**
  - Cứ sau khoảng 2.0 đến 4.5 giây ở trạng thái IDLE, bot sẽ tự động chọn ngẫu nhiên một trong 10 hành động đặc biệt để diễn:
    1. **WAVE:** Đứng vẫy tay chào, mắt cười tít, má ửng hồng.
    2. **JUMP:** Thu người dồn lực (squash) $\rightarrow$ bật cao lên trời (stretch) $\rightarrow$ bóng đổ co nhỏ lại $\rightarrow$ đáp đất nảy đàn hồi.
    3. **WALK:** Chân bước xen kẽ, hai tay đung đưa theo nhịp đi bộ.
    4. **DANCE:** Lắc lư người sang hai bên, tay giơ cao, mắt lấp lánh sao vàng.
    5. **LAUGH:** Mắt nhắm tịt `> <`, miệng cười toe toét, bật người nhảy nhót.
    6. **LOVE:** Mắt hình trái tim màu đỏ hồng, hai tay ôm ngực, tim bay lơ lửng.
    7. **THINKING:** Tay chống cằm suy nghĩ, mắt nhìn lên góc trên bên phải, dấu `?` hiện ra.
    8. **SURPRISED:** Mắt trợn tròn xoe, giật nảy mình, dấu `!` màu vàng xuất hiện.
    9. **CONFUSED:** Nghiêng đầu bối rối, tay gãi đầu, dấu `?` nhấp nháy.
    10. **SLEEP:** Người lùn xuống, mắt nhắm nghiền, các chữ Zzz bay lên.
  - Khi diễn xong hành động, bot tự động trở về nhịp thở IDLE quen thuộc, sau đó lại tiếp tục chọn ngẫu nhiên hành động khác.
  - Terminal UART in thông báo mỗi khi đổi chiêu:
    ```text
    [BOT] Action: JUMP (Priority 2)
    [BOT] Action: LOVE (Priority 2)
    [BOT] Action: DANCE (Priority 2)
    ```

### Test 4: Tương tác tức thì bằng nút bấm PA0
- **Thao tác:** Nhấn nút nhấn màu xanh dương **B1 (PA0)** trên bo mạch.
- **Kỳ vọng quan sát:**
  - Bot ngay lập tức phản xạ, ngắt trạng thái hiện tại và bật nhảy ăn mừng (**JUMP**) hoặc nhảy múa (**DANCE**).
  - Terminal UART in ra log:
    ```text
    [BTN] User button pressed! Triggering excited reaction...
    [BOT] Action: JUMP (Priority 2)
    ```

### Test 5: Điều khiển trực tiếp từ bàn phím máy tính qua UART
- **Thao tác:** Click chuột vào cửa sổ Command Shell Console trong CubeIDE và gõ các phím ký tự sau:
  - Phím **`w`**: Bot vẫy tay (**WAVE**).
  - Phím **`j`**: Bot bật nhảy cao (**JUMP**).
  - Phím **`k`**: Bot đi bộ (**WALK**).
  - Phím **`d`**: Bot nhảy múa (**DANCE**).
  - Phím **`l`**: Bot cười ré lên (**LAUGH**).
  - Phím **`h`**: Bot thả tim (**LOVE**).
  - Phím **`t`**: Bot chống cằm suy nghĩ (**THINKING**).
  - Phím **`!`**: Bot giật mình tròn mắt (**SURPRISED**).
  - Phím **`?`**: Bot bối rối (**CONFUSED**).
  - Phím **`s`**: Bot ngủ ngáy Zzz (**SLEEP**).
  - Phím **`r`**: Kích hoạt một hành động bất kỳ ngẫu nhiên (**RANDOM**).
- **Kỳ vọng quan sát:** Bot lập tức đổi ngay sang hành động tương ứng với phím gõ.

---

## 4. Xử lý sự cố thường gặp (Troubleshooting)

### 1. Màn hình tối đen, đèn LED đỏ PG14 sáng đứng
- **Nguyên nhân:** Chương trình bị rơi vào hàm `Error_Handler()` ngay khi khởi động.
- **Khắc phục:** 
  1. Kiểm tra lại cấu hình xung nhịp RCC trong file `.ioc`: `High Speed Clock (HSE)` **phải chọn là BYPASS Clock Source**, không được chọn Crystal. Bo mạch STM32F429I-DISC1 lấy xung clock 8 MHz từ vi điều khiển nạp ST-LINK, nếu chọn Crystal chip sẽ không lock được PLL và chết ngay tại `SystemClock_Config()`.
  2. Bấm phím **F8 (Resume)** để chắc chắn MCU không đang bị debugger tạm dừng.

### 2. Màn hình sáng đèn nền nhưng chỉ toàn màu trắng
- **Nguyên nhân:** Khối LCD chưa nhận được lệnh khởi tạo qua SPI5 hoặc cáp màn hình lỏng.
- **Khắc phục:**
  1. Đảm bảo hàm `BSP_LCD_Init()` đã được gọi bên trong `LCD_Init_RGB565()`.
  2. Kiểm tra lại chân cáp bẹ nối màn hình LCD với bo mạch xem có bị xộc xệch hay không.

### 3. Hình ảnh nhân vật bị vỡ sọc, sai màu, lệch hàng ngang
- **Nguyên nhân:** Khối LTDC vẫn đang cấu hình ở chế độ `ARGB8888` (32-bit/pixel) mặc định của ST BSP trong khi code đang ghi dữ liệu màu `RGB565` (16-bit/pixel).
- **Khắc phục:**
  Kiểm tra hàm `LCD_Init_RGB565()` trong `main.c`, đảm bảo đã có đoạn cấu hình lại định dạng màu:
  ```c
  Layercfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  HAL_LTDC_ConfigLayer(&LtdcHandler, &Layercfg, LCD_BACKGROUND_LAYER);
  ```

### 4. Lỗi biên dịch `undefined reference to BSP_LCD_...` hoặc `fatal error: lcd.h: No such file`
- **Nguyên nhân:** Chưa cấu hình đường dẫn Include Paths hoặc chưa copy đủ file BSP/HAL.
- **Khắc phục:**
  1. Xem lại mục **5b** trong tài liệu `lcd-pixel-bot-cubemx-config.md`.
  2. Kiểm tra Project Properties $\rightarrow$ **C/C++ Build** $\rightarrow$ **Settings** $\rightarrow$ **MCU GCC Compiler** $\rightarrow$ **Include paths**, đảm bảo đã add đủ 3 đường dẫn:
     - `.../Drivers/BSP/STM32F429I-Discovery`
     - `.../Drivers/BSP/Components/ili9341`
     - `.../Utilities/Fonts`
  3. Kiểm tra file `Core/Inc/stm32f4xx_hal_conf.h`, đảm bảo đã xóa dấu comment ở 5 macro:
     `HAL_DMA2D_MODULE_ENABLED`, `HAL_SDRAM_MODULE_ENABLED`, `HAL_I2C_MODULE_ENABLED`, `HAL_LTDC_MODULE_ENABLED`, `HAL_SPI_MODULE_ENABLED`.

### 5. Terminal Console không in ra chữ khi bot hoạt động
- **Nguyên nhân:** Chưa kết nối đúng cổng COM hoặc sai Baudrate.
- **Khắc phục:**
  1. Kiểm tra Device Manager trên Windows xem cổng **STMicroelectronics STLink Virtual COM Port** đang là cổng COM mấy.
  2. Đảm bảo cấu hình cổng Terminal đúng tốc độ **`115200 bps`**.
