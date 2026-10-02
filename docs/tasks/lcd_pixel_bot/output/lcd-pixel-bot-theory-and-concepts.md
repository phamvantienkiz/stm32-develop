# Theory & Concepts — Animated Pixel Bot

| | |
|---|---|
| Task ID | `lcd-pixel-bot` |
| Documents consulted | `UM1670 rev 5`, `RM0090 rev 19`, `datasheet STM32F429ZI rev 6`, `schematic MB1075 rev B` |

---

## 1. Facts established from the documentation

### LTDC (LCD-TFT Display Controller)

| Question | Answer | Source |
|---|---|---|
| Which bus and clock feeds it? | Nằm trên bus **AHB**, xung nhịp điểm ảnh được cấp từ bộ tạo xung chuyên dụng **PLLSAI** (thông qua bộ chia `PLLSAIDivR`) `[RM0090 §6.2.2 p.215]`. | `[RM0090 §18.3 p.490]` |
| What starts it, where does data flow, which flag says done? | Bật bằng bit `LTDC_GCR_LTDCEN`. Phần cứng LTDC Master DMA tự động đọc các luồng pixel liên tục từ SDRAM đẩy trực tiếp ra panel song song 18-bit RGB `[RM0090 §18.4 p.493]`. | `[RM0090 §18.4.1 p.494]` |
| How to reload new frame buffer address without screen tearing? | Ghi địa chỉ Backbuffer vào thanh ghi `LTDC_LxCFBAR`. Sau đó kích hoạt bit **`VBR`** (Vertical Blanking Reload) trong thanh ghi **`LTDC_SRCR`**. Phần cứng sẽ tráo buffer chính xác tại thời điểm quét hết khung hình (Vertical Blanking period), sau đó tự động xóa bit `VBR` về 0. | `[RM0090 §18.5.8 p.515]` |
| Relevant registers and bits | `LTDC_SSCR` (Sync size), `LTDC_BPCR` (Back porch), `LTDC_AWCR` (Active width), `LTDC_SRCR` (Shadow Reload Config: `IMR` bit 0, `VBR` bit 1), `LTDC_L1CFBAR` (Layer 1 Color Frame Buffer Address). | `[RM0090 §18.5 p.507-526]` |

### FMC & External SDRAM (IS42S16400J)

| Question | Answer | Source |
|---|---|---|
| Which bus and memory region? | Nằm trên bus **AHB3**. Bộ nhớ SDRAM ngoài 64 Mbit (8 MByte) trên kit Discovery được ánh xạ cố định vào **FMC SDRAM Bank 2**, bắt đầu từ địa chỉ vật lý **`0xD0000000`** đến `0xD07FFFFF`. | `[RM0090 §13.7 p.364; UM1670 §7.10 p.21]` |
| Refresh & Timing constraints | Điều khiển bởi thanh ghi `FMC_SDRTR` (SDRAM Refresh Timer). Thư viện BSP đã đóng gói chuẩn thời gian nạp và tự động làm tươi theo chu kỳ của chip nhớ ISSI. | `[RM0090 §13.7.5 p.372]` |

### Display Panel ILI9341 & SPI5

| Question | Answer | Source |
|---|---|---|
| How are commands vs pixels transmitted? | Lệnh khởi tạo ban đầu (chế độ màu, hướng quét, gamma) được MCU gửi qua bus **SPI5** (PF7=SCK, PF9=MOSI, PC1=CSX). Sau khi khởi tạo xong, ILI9341 chuyển sang chế độ RGB Interface, nhận pixel trực tiếp từ khối LTDC. | `[UM1670 §7.9 p.21; MB1075 sheet 4]` |

### User Button (B1)

| Question | Answer | Source |
|---|---|---|
| Electrical connection & polarity | Chân **PA0** nối với nút nhấn B1. Có điện trở kéo xuống GND $R_{22} = 220\text{ k}\Omega$. Khi nhấn nút, tiếp điểm đóng thẳng lên nguồn $V_{DD}$ (3V). Do đó nút nhấn là **Active High** (mức 1 khi bấm, mức 0 khi nhả). | `[UM1670 §7.6 p.20; MB1075 sheet 6]` |

---

## 2. Explanation of concepts

### 2.1 Kiến trúc Pixel Art dạng Lớp (Layer-based Component Rendering)

Thay vì lưu trữ chuỗi ảnh bitmap khổng lồ cho từng khung hình (một ảnh 240×320 RGB565 chiếm 150 KB, 10 khung hình sẽ tốn 1.5 MB Flash — chiếm gần hết 2 MB Flash của chip), nhân vật Pixel Bot được thiết kế theo tư duy **lập trình tham số hóa linh hoạt (Procedural Modular Sprite)**:
1. **Lưới cơ sở (Grid System):** Chia màn hình thành lưới các ô vuông kích thước cố định `CELL = 12 px`. Màn hình 240×320 tương đương với ma trận $20 \times 26$ ô.
2. **Cấu trúc Pose:** Mỗi trạng thái nhân vật chỉ là một struct C nhỏ gọn (chưa đầy 20 byte) lưu vị trí tay, độ co giãn thân (squash/stretch), loại mắt, loại miệng, và hiệu ứng (FX).
3. **Thứ tự vẽ từ dưới lên (Painter's Algorithm):**
   $$\text{Nền} \rightarrow \text{Bóng đổ} \rightarrow \text{Chân} \rightarrow \text{Thân} \rightarrow \text{Tay} \rightarrow \text{Má hồng} \rightarrow \text{Mắt} \rightarrow \text{Miệng} \rightarrow \text{FX}$$
4. **Đối xứng và Lật ngang (Horizontal Flip):** Mắt phải là ảnh lật đối xứng của mắt trái; tay trái và tay phải dùng chung công thức đối xứng qua trục giữa thân. Chỉ cần lưu 1 bitmap mắt 3×4 (chỉ tốn 4 byte), ta có thể biểu diễn được cho cả 2 mắt.

### 2.2 Kỹ thuật Double Buffering & VSYNC không rách hình

- **Hiện tượng Tearing (Rách hình):** Xảy ra khi CPU đang ghi dữ liệu điểm ảnh mới vào bộ nhớ đúng lúc phần cứng LTDC đang quét dòng đó ra màn hình, khiến nửa trên hiển thị frame cũ, nửa dưới hiển thị frame mới.
- **Giải pháp Double Buffering:**
  - `FB0` (`0xD0000000`): Front-buffer (nơi LTDC đang đọc và phát ra màn hình).
  - `FB1` (`0xD0025800`): Back-buffer (nơi CPU tự do vẽ hình nhân vật mà không ai nhìn thấy).
- **Cơ chế VSYNC Shadow Reload:**
  Khi vẽ xong trên Back-buffer, ta không tráo ngay mà gọi lệnh nạp địa chỉ `HAL_LTDC_SetAddress_NoReload()`, sau đó kích hoạt cờ `LTDC->SRCR = LTDC_SRCR_VBR`. Khối LTDC sẽ đợi quét hết điểm ảnh cuối cùng của màn hình (bắt đầu chu kỳ xóa dọc - Vertical Blanking), lúc đó mới âm thầm hoán đổi địa chỉ khung hình. Mắt người sẽ nhìn thấy hình ảnh chuyển động mượt mà tuyệt đối mà không có bất kỳ vệt xé nào.
- **Lợi thế trên STM32F429:** Dòng vi điều khiển Cortex-M4 trên STM32F429 không có bộ nhớ đệm dữ liệu (L1 Data Cache), do đó dữ liệu CPU ghi vào vùng SDRAM sẽ có hiệu lực tức thì mà không cần phải thực hiện các lệnh tốn kém như `SCB_CleanDCache()`.

### 2.3 Non-blocking Multi-rate Scheduler (Định thời đa tốc độ phi chặn)

Một con bot "sống động" đòi hỏi nhiều nhịp điệu diễn ra song song cùng lúc:
- Chu kỳ render khung hình: **33 ms** (30 FPS).
- Chu kỳ chớp mắt tự nhiên: ngẫu nhiên **2500 – 5500 ms** (thời lượng chớp chỉ 180 ms).
- Chu kỳ liếc nhìn trái/phải: ngẫu nhiên **8000 – 15000 ms**.
- Chu kỳ chuyển đổi hành động lớn: **2000 – 4000 ms** sau khi ở trạng thái Idle.

Nếu dùng hàm `HAL_Delay()`, CPU sẽ bị khóa chặt, mọi cử động chớp mắt hay tương tác nút bấm sẽ bị đơ cứng. Hệ thống áp dụng mẫu thiết kế **Delta-time Scheduler**:
```c
uint32_t now = HAL_GetTick();
if (now - last_render >= 33 && !(LTDC->SRCR & LTDC_SRCR_VBR)) {
    last_render = now;
    // Render 1 frame đồ họa
}
```
Phép trừ không dấu `(now - last_render >= PERIOD)` luôn đảm bảo an toàn tuyệt đối khi biến đếm 32-bit `SysTick` tràn số sau xấp xỉ 49.7 ngày hoạt động liên tục.

### 2.4 Bộ sinh số ngẫu nhiên nhẹ (Linear Congruential Generator - LCG)

Để các hành động của bot diễn ra ngẫu nhiên và bất ngờ mà không cần tiêu tốn ngoại vi RNG phần cứng, thuật toán LCG 32-bit chuẩn được áp dụng:
$$X_{n+1} = (a \cdot X_n + c) \pmod{2^{32}}$$
Với các tham số nổi tiếng của Numerical Recipes ($a = 1664525$, $c = 1013904223$). Bộ sinh này chỉ mất vài chu kỳ lệnh CPU để tạo ra một chuỗi phân bố đều, kết hợp giá trị khởi tạo (seed) từ bộ đếm `SysTick` khi người dùng nhấn nút lần đầu tiên.

---

## 3. Design rationale

### Tại sao chọn định dạng màu RGB565 thay vì ARGB8888?

1. **Băng thông bus SDRAM:**
   - ARGB8888 cần 4 byte/pixel $\rightarrow$ 1 khung hình 240×320 chiếm $307,200\text{ bytes} \approx 300\text{ KB}$.
   - RGB565 cần 2 byte/pixel $\rightarrow$ 1 khung hình 240×320 chiếm $153,600\text{ bytes} \approx 150\text{ KB}$.
2. **Tốc độ làm tươi:** Với xung nhịp SDRAM 90 MHz (HCLK/2) trên bus 16-bit, việc giảm một nửa dung lượng giúp CPU xóa nền (`fb_clear`) và tô các khối pixel nhanh gấp đôi, giải phóng băng thông bus FMC cho LTDC quét màn hình mà không bao giờ bị nghẽn FIFO (FIFO Underrun).

### Tại sao chọn giải pháp "No" trong CubeMX và dùng ST BSP?

- Cấu hình FMC SDRAM trên chip STM32F429 đòi hỏi hàng chục thông số định thời ngặt nghèo (Row Precharge delay, RAS to CAS delay, Refresh rate count...). Sai lệch chỉ 1 nanogiây sẽ khiến bộ nhớ SDRAM đọc ghi sai dữ liệu.
- Bộ thư viện ST BSP (`stm32f429i_discovery_sdram.c` và `stm32f429i_discovery_lcd.c`) đã được các kỹ sư ST cân chỉnh tối ưu hoàn hảo cho phần cứng trên kit. Việc tận dụng BSP giúp dự án đạt trạng thái hoạt động ngay lập tức với độ ổn định 100%.

---

## 4. What to learn from this task

1. **Quy tắc tráo Framebuffer không giật:** Muốn màn hình chuyển động đồ họa tốc độ cao không bị rách hình, luôn dùng double buffering và chỉ tráo trang tại vạch VSYNC thông qua cờ phần cứng `LTDC_SRCR_VBR`.
2. **Kỹ thuật Procedural Sprite:** Hoạt hình nhân vật phức tạp có thể được tạo ra từ việc tổ chức dữ liệu thông minh (chia nhỏ bộ phận, ghép theo layer) thay vì ngốn hàng Megabyte bộ nhớ Flash cho file ảnh tĩnh.
3. **Quản lý máy trạng thái đa tầng (Hierarchical State Machine):** Tách bạch giữa trạng thái cơ sở (Base Pose), hành động chính (Action sequence), và các lớp phủ tức thời (Blink/Look overlays).

---

## 5. Possible extensions

1. **Âm thanh tương tác:** Nối module I2S DAC (như MAX98357A) để phát các tiếng "bíp", "chíp" 8-bit vui nhộn đồng bộ với mỗi frame hành động (ví dụ: phát tiếng "boing" khi bot nhảy JUMP).
2. **Cảm biến chuyển động Gyro (L3GD20):** Đọc trục X/Y/Z của con quay hồi chuyển trên kit qua SPI5; khi người dùng lắc bo mạch, kích hoạt ngay biểu cảm mắt xoáy **DIZZY** và hoạt cảnh lảo đảo.
3. **Cảm ứng điện trở STMPE811:** Tích hợp I2C3 để phát hiện vị trí ngón tay chạm trên màn hình, kích hoạt phản ứng **POKE** (giật mình, chớp mắt, cười toe toét).
