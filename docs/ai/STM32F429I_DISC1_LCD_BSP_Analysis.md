# Phân tích và Hướng dẫn lập trình LCD trên STM32F429I-DISC1 sử dụng BSP

## 1. Tổng quan
Trên kit phát triển **STM32F429I-DISC1**, việc điều khiển màn hình LCD (sử dụng chip điều khiển ILI9341 và giao diện LTDC của vi điều khiển) nếu lập trình từ đầu (bare-metal hoặc chỉ dùng HAL cơ bản) sẽ rất phức tạp vì cần cấu hình chi tiết timer, tín hiệu đồng bộ (HSYNC, VSYNC), bộ nhớ đệm SDRAM ngoài, và SPI để khởi tạo chip ILI9341.

Giải pháp tối ưu và nhanh chóng nhất là sử dụng bộ thư viện **Board Support Package (BSP)** do ST cung cấp cho chính board này. Thư viện này bọc lại toàn bộ các thiết lập phức tạp ở tầng dưới và cung cấp các hàm API thân thiện cho người dùng ở tầng ứng dụng (UI text, vẽ hình cơ bản).

## 2. Cấu trúc và thư viện cần thiết
Từ folder `BSP` hiện có trong source:
- **Thư mục `Src` và `Inc`**: Chứa các file code mẫu như `lcd.c`, `main.c`, `lcd_log_conf.h`, `stlogo.h` minh hoạ cách sử dụng màn hình.
- Để sử dụng được thư viện LCD trong một Project STM32CubeIDE thực tế, bạn cần copy các file BSP driver cốt lõi từ gói Firmware STM32CubeF4 (thường nằm ở `Drivers/BSP/STM32F429I-Discovery/` và `Drivers/BSP/Components/ili9341/`) bao gồm:
  - `stm32f429i_discovery_lcd.c` / `.h`: Chứa các hàm API chính như `BSP_LCD_Init()`, `BSP_LCD_DisplayStringAt()`.
  - `stm32f429i_discovery_sdram.c` / `.h`: Quản lý bộ nhớ SDRAM ngoài để làm Frame Buffer (bộ đệm khung hình) cho LCD.
  - `ili9341.c` / `.h`: Driver giao tiếp SPI để cấu hình chip điều khiển màn hình.

## 3. Các bước khởi tạo và sử dụng LCD
Dựa trên phân tích mã nguồn `main.c` và `lcd.c` của thư mục BSP, quy trình chuẩn để sử dụng màn hình như sau:

### Bước 1: Khởi tạo hệ thống
- Khởi tạo HAL (`HAL_Init()`)
- Cấu hình System Clock (Thường là 180MHz cho STM32F429).
- Khởi tạo SDRAM (Thường được bọc sẵn trong hàm Init của LCD, nhưng đôi khi cần cấp phát Frame Buffer tại địa chỉ `0xD0000000` - Base address của SDRAM ngoài trên FMC Bank 2).

### Bước 2: Khởi tạo LCD và Layer
LCD trên STM32F429 hỗ trợ tối đa 2 layer (Layer 1 - Background và Layer 2 - Foreground) thông qua bộ điều khiển LTDC.

```c
/* Khởi tạo màn hình LCD (Bao gồm LTDC, SPI khởi tạo chip ILI9341, SDRAM) */
BSP_LCD_Init();

/* Khởi tạo Layer 1 và trỏ Frame Buffer tới bộ nhớ SDRAM ngoài */
// LCD_FRAME_BUFFER thường được define là 0xD0000000
BSP_LCD_LayerDefaultInit(1, 0xD0000000);

/* Chọn Layer 1 làm layer hiện tại để thao tác vẽ */
BSP_LCD_SelectLayer(1);
```

### Bước 3: Cấu hình hiển thị và vẽ lên màn hình
Sử dụng các hàm có sẵn trong BSP để vẽ:

```c
/* Xoá toàn bộ màn hình với màu trắng */
BSP_LCD_Clear(LCD_COLOR_WHITE);

/* Cài đặt màu chữ và màu nền cho chữ */
BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
BSP_LCD_SetBackColor(LCD_COLOR_WHITE);

/* Cài đặt Font chữ */
BSP_LCD_SetFont(&Font16); // Font12, Font16, Font20, Font24...

/* Hiển thị một chuỗi ký tự */
// Chế độ: LEFT_MODE, CENTER_MODE, RIGHT_MODE
BSP_LCD_DisplayStringAt(0, 10, (uint8_t*)"Hello STM32", CENTER_MODE);

/* Các hàm vẽ hình học cơ bản */
BSP_LCD_DrawRect(x, y, width, height);
BSP_LCD_FillRect(x, y, width, height);
BSP_LCD_DrawCircle(x, y, radius);
```

### Bước 4: Hiển thị hình ảnh (Bitmap)
Bạn có thể render ảnh mảng bit (thường được convert thành dạng mảng C trong file header như `stlogo.h`) bằng hàm:
```c
BSP_LCD_DrawBitmap(x, y, (uint8_t *)stlogo);
```

## 4. Troubleshooting & Lưu ý (Bẫy thường gặp)
- **Vấn đề Clock (SystemClock_Config):** Màn hình LTDC đòi hỏi cấu hình xung nhịp (Clock Tree) rất chính xác cho PLLSAI để cấp xung pixel clock cho màn hình. Nếu cấu hình sai, màn hình sẽ bị nhiễu hạt (Salt noise) hoặc không lên hình. STM32CubeMX có thể giúp tự động tính toán.
- **Dung lượng Frame Buffer:** Màn hình phân giải QVGA (240x320) ở chế độ 16-bit màu (RGB565) tiêu tốn khoảng `240 * 320 * 2 = 153.6 KB` RAM cho mỗi layer. Do đó, phải luôn đặt `LCD_FRAME_BUFFER` ở `0xD0000000` (SDRAM ngoài 8MB) chứ không để trong SRAM nội bộ của MCU vì dễ gây tràn bộ nhớ.
- **LTDC vs SPI:** Chip ILI9341 được MCU giao tiếp cấu hình (gửi các lệnh setup) qua giao thức **SPI** (hoặc I2C tuỳ version mạch). Sau khi cấu hình xong, dữ liệu hình ảnh được bắn liên tục từ SDRAM ra màn hình qua bus song song **LTDC RGB**. Cần chắc chắn các chân (Pins) của cả 2 ngoại vi này được cấu hình đúng.

## 5. Hướng phát triển tiếp theo
1. Thực hành việc tạo project CubeIDE, add các file source thư viện `Drivers/BSP` vào.
2. Hiển thị thử các dòng chữ đơn giản, thay đổi màu sắc và toạ độ.
3. Khi đã quen với BSP, có thể tiến tới việc tích hợp hệ thống GUI chuyên nghiệp của ST là **TouchGFX** (hoặc **LVGL**) để tạo các UI hiện đại (có nút bấm, hiệu ứng, thanh cuộn, ...) thay vì chỉ dùng các hàm vẽ text thô sơ.
