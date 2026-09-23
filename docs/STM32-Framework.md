# Framework cá nhân: Từ đề bài → chạy được trên STM32

> Phiên bản dành cho board **STM32F429I-DISC1** (MCU `STM32F429ZIT6`), toolchain **STM32CubeIDE + CubeMX + HAL**.
> Mục tiêu: không bao giờ còn cảm giác "đọc xong đề rồi... không biết bắt đầu từ đâu".

---

## 0. Triết lý của framework

Người mới thường nghĩ quy trình là: _đề bài → mở CubeMX → code_. Đó là lý do bị kẹt.

Quy trình thật sự là:

```
ĐỀ BÀI (ngôn ngữ đời thường)
   ↓  Pha 1 — Phân tích
ĐẶC TẢ CHỨC NĂNG (input / output / timing / giao tiếp)
   ↓  Pha 2 — Chọn ngoại vi
DANH SÁCH NGOẠI VI TRỪU TƯỢNG (GPIO, TIM, UART, ADC...)
   ↓  Pha 3 — Ánh xạ lên board thật
DANH SÁCH CHÂN PIN CỤ THỂ (PG13, PA0, PF7...)
   ↓  Pha 4 — Tra Datasheet + Reference Manual
BẢNG CẤU HÌNH (mode, AF, clock, NVIC, DMA)
   ↓  Pha 5 — Tính toán clock/timing trên giấy
CON SỐ CỤ THỂ (PSC, ARR, baudrate, prescaler)
   ↓  Pha 6 — CubeMX
CODE KHUNG TỰ SINH
   ↓  Pha 7 — Viết logic HAL
FIRMWARE
   ↓  Pha 8 — Build / Flash / Debug
CHẠY ĐƯỢC ✅
```

**Nguyên tắc vàng:** 80% thời gian ở Pha 1–5 (trên giấy), 20% ở Pha 6–8 (trên máy). Nếu bạn đang loay hoay ở CubeMX thì nghĩa là Pha 1–5 chưa xong.

---

## 1. Bộ tài liệu — tải 1 lần, dùng cả khoá

Tạo thư mục `~/STM32_Docs/` và tải sẵn 6 file sau từ [st.com](https://www.st.com):

| #   | Tài liệu                         | Mã                            | Dày       | Trả lời câu hỏi gì                                                                    |
| --- | -------------------------------- | ----------------------------- | --------- | ------------------------------------------------------------------------------------- |
| 1   | **Discovery kit user manual**    | `UM1670`                      | ~32 tr    | _Board này có sẵn cái gì? LED nào nối chân nào? Nút nhấn ở đâu? Jumper nào phải cắm?_ |
| 2   | **Board schematic**              | `MB1075` (đính kèm UM1670)    | vài trang | _Chính xác mạch điện: điện trở kéo, active-high hay active-low, chân nào đã bị chiếm_ |
| 3   | **MCU Datasheet**                | `STM32F427xx/429xx datasheet` | ~200 tr   | _Chân PA9 làm được những chức năng gì? AF mấy? Điện áp, dòng, tốc độ tối đa_          |
| 4   | **Reference Manual**             | `RM0090`                      | ~1750 tr  | _Ngoại vi hoạt động thế nào? Thanh ghi nào, bit nào? Sơ đồ clock tree_                |
| 5   | **HAL driver description**       | `UM1725`                      | ~1300 tr  | _Hàm HAL nào tồn tại? Tham số gì? Trạng thái/callback ra sao_                         |
| 6   | **Cortex-M4 programming manual** | `PM0214`                      | ~250 tr   | _NVIC, SysTick, độ ưu tiên ngắt, lệnh assembly, FPU_                                  |

Thêm (tuỳ chọn, rất đáng giá):

- **STM32CubeF4 package** (`STM32Cube_FW_F4_Vxx`) — chứa **hàng trăm ví dụ chạy sẵn** trong `Projects/STM32F429I-Discovery/Examples/`. Đây là nguồn tham khảo tốt nhất, hơn mọi tutorial trên mạng.

### Bảng tra nhanh "câu hỏi → tài liệu"

| Câu hỏi trong đầu bạn                   | Mở tài liệu                             | Tìm mục                                   |
| --------------------------------------- | --------------------------------------- | ----------------------------------------- |
| "LED trên board nối chân nào?"          | UM1670                                  | _LEDs_ / _Hardware layout_                |
| "Nhấn nút thì mức logic là 0 hay 1?"    | MB1075 schematic                        | Vùng `B1 USER`                            |
| "Chân PF7 có dùng cho SPI5 được không?" | Datasheet                               | Bảng _Alternate function mapping_         |
| "SPI5 nằm ở bus APB nào?"               | Datasheet hoặc RM0090                   | _Block diagram_ / _Bus matrix_            |
| "Timer đếm như thế nào? công thức?"     | RM0090                                  | Chương _General-purpose timers_           |
| "Bit EN của ADC nằm ở thanh ghi nào?"   | RM0090                                  | Chương _ADC → ADC registers_              |
| "Hàm HAL để đọc ADC tên gì?"            | UM1725                                  | Chương _HAL ADC Generic Driver_           |
| "Clock 180 MHz lấy từ đâu?"             | RM0090                                  | Chương _RCC → Clock tree_                 |
| "Ngắt EXTI0 tên vector là gì?"          | `startup_stm32f429zitx.s` trong project | Danh sách vector                          |
| "Có ví dụ mẫu nào không?"               | STM32Cube_FW_F4                         | `Projects/STM32F429I-Discovery/Examples/` |

---

## 2. PHA 1 — Phân tích đề bài

Không bao giờ mở CubeMX trước khi điền xong **Phiếu phân tích đề** này.

### 2.1 Template phiếu phân tích

```markdown
## PHIẾU PHÂN TÍCH ĐỀ — Bài số \_\_\_

### A. Mô tả lại đề bằng 1 câu của riêng tôi

...

### B. INPUT (hệ thống nhận gì từ thế giới bên ngoài?)

| Nguồn         | Loại tín hiệu      | Tần suất       | Ghi chú        |
| ------------- | ------------------ | -------------- | -------------- |
| Nút nhấn USER | Digital, mức logic | Bất kỳ lúc nào | Cần chống dội? |
| Biến trở      | Analog 0–3.3V      | 10 Hz          | Cần ADC        |

### C. OUTPUT (hệ thống tác động ra ngoài thế nào?)

| Đích     | Loại tín hiệu  | Tần suất | Ghi chú     |
| -------- | -------------- | -------- | ----------- |
| LED LD3  | Digital ON/OFF | 1 Hz     |             |
| Máy tính | Chuỗi ký tự    | 1 lần/s  | UART 115200 |

### D. RÀNG BUỘC THỜI GIAN

- [ ] Có yêu cầu định thời chính xác? (VD: "đúng 500 ms") → cần TIMER, không dùng HAL_Delay
- [ ] Có yêu cầu phản hồi tức thì? → cần NGẮT, không dùng polling
- [ ] Có xử lý dữ liệu liên tục/tốc độ cao? → cần DMA

### E. TRẠNG THÁI (state) hệ thống cần nhớ

- VD: chế độ hiện tại (nhanh/chậm), số lần nhấn, giá trị đo gần nhất

### F. TIÊU CHÍ "XONG BÀI"

- Quan sát được gì thì coi là đúng? (LED chớp đúng nhịp, terminal in đúng số...)
```

### 2.2 Từ điển dịch: động từ trong đề → ngoại vi

Đây là bảng quan trọng nhất khi mới bắt đầu. Đọc đề, gạch chân động từ, tra bảng.

| Trong đề xuất hiện                                                          | Ngoại vi cần                   | Ghi chú                       |
| --------------------------------------------------------------------------- | ------------------------------ | ----------------------------- |
| bật/tắt LED, điều khiển relay                                               | **GPIO Output**                | Push-pull                     |
| đọc nút nhấn, công tắc, cảm biến on/off                                     | **GPIO Input**                 | + Pull-up/down; cần chống dội |
| "khi nhấn nút thì ngay lập tức..."                                          | **GPIO + EXTI** (ngắt ngoài)   | Không dùng vòng lặp quét      |
| "sau X giây", "mỗi X ms", "định kỳ"                                         | **TIM** (timer) + ngắt update  | Chính xác hơn `HAL_Delay`     |
| "đo thời gian giữa 2 sự kiện"                                               | **TIM Input Capture**          |                               |
| "điều chỉnh độ sáng", "điều khiển tốc độ động cơ", "phát âm thanh đơn giản" | **TIM PWM Output**             |                               |
| "đếm xung", "encoder"                                                       | **TIM Counter / Encoder mode** |                               |
| đọc điện áp, nhiệt độ (cảm biến analog), biến trở, LDR                      | **ADC**                        | 12-bit, tham chiếu 3.3 V      |
| xuất điện áp analog, phát sóng sin                                          | **DAC**                        | F429 có 2 kênh                |
| giao tiếp với máy tính, module GPS/Bluetooth HC-05, `printf`                | **UART/USART**                 |                               |
| cảm biến I2C (MPU6050, BME280, OLED SSD1306), EEPROM                        | **I2C**                        |                               |
| cảm biến/màn hình SPI (ILI9341, L3GD20, thẻ nhớ)                            | **SPI**                        | Nhanh hơn I2C                 |
| thẻ nhớ SD tốc độ cao                                                       | **SDIO**                       |                               |
| "truyền dữ liệu lớn mà không tốn CPU"                                       | **DMA**                        | Ghép với UART/ADC/SPI         |
| hiển thị lên màn hình LCD trên board                                        | **LTDC + SPI5 + FMC(SDRAM)**   | Phức tạp — dùng BSP có sẵn    |
| đồng hồ thời gian thực, ngày/giờ                                            | **RTC**                        | Cần LSE/LSI                   |
| "hệ thống treo thì tự khởi động lại"                                        | **IWDG/WWDG**                  |                               |

### 2.3 Quyết định kiến trúc xử lý (rất quan trọng)

Với mỗi cặp input/output, chọn **một** trong ba mô hình:

| Mô hình       | Khi nào dùng                                              | Đặc điểm                                        |
| ------------- | --------------------------------------------------------- | ----------------------------------------------- |
| **Polling**   | Bài tập đơn giản, không ràng buộc thời gian               | Dễ nhất, CPU bận rộn, dễ bỏ sót sự kiện         |
| **Interrupt** | Sự kiện bất ngờ, cần phản hồi nhanh (nút nhấn, UART nhận) | ISR phải ngắn; dùng `volatile` cho biến chia sẻ |
| **DMA**       | Dữ liệu lớn/liên tục (ADC liên tục, UART dài, ảnh)        | CPU rảnh hoàn toàn; cấu hình phức tạp hơn       |

> Quy tắc thực dụng cho người mới: **bài 1–5 dùng polling, từ bài 6 trở đi bắt đầu chuyển sang interrupt**. Đừng nhảy thẳng vào DMA.

---

## 3. PHA 2 — Từ chức năng sang danh sách ngoại vi

Kết quả của pha này là một bảng như sau (chưa có tên chân cụ thể):

| Chức năng trong đề  | Ngoại vi           | Mô hình xử lý      | Tham số cần tính             |
| ------------------- | ------------------ | ------------------ | ---------------------------- |
| Nhấn nút đổi chế độ | GPIO Input + EXTI  | Interrupt          | Pull-up/down, sườn lên/xuống |
| LED nháy 500 ms     | GPIO Output + TIM3 | Interrupt (update) | PSC, ARR                     |
| In log ra PC        | USART1             | Polling            | Baudrate 115200              |

Tại đây bạn **chưa cần biết chân nào**. Đừng vội.

---

## 4. PHA 3 — Ánh xạ lên board thật (UM1670 + Schematic)

Câu hỏi chủ đạo: **"Board đã có sẵn gì, và cái gì đã bị chiếm?"**

### 4.1 Tài nguyên có sẵn trên STM32F429I-DISC1

> ⚠️ Luôn **tự kiểm chứng lại bảng này trong UM1670 và schematic MB1075** của đúng revision board bạn cầm trên tay. Đây chính là kỹ năng cần rèn — đừng tin bảng chép sẵn (kể cả bảng này).

| Thành phần        | Kết nối (cần verify)                                                                      | Ghi chú                                                                          |
| ----------------- | ----------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| MCU               | STM32F429ZIT6, LQFP144                                                                    | 2 MB Flash, 256+4 KB SRAM, tối đa 180 MHz, có FPU                                |
| LED LD3 (xanh lá) | PG13                                                                                      | User LED                                                                         |
| LED LD4 (đỏ)      | PG14                                                                                      | User LED                                                                         |
| Nút USER (B1)     | PA0                                                                                       | ⚠️ Trên board này thường là **active HIGH** — nhấn ra mức 1. Phải xem schematic! |
| Nút RESET (B2)    | NRST                                                                                      | Không lập trình được                                                             |
| Debugger          | ST-LINK/V2-B onboard, SWD                                                                 | PA13 SWDIO, PA14 SWCLK — **không đụng vào**                                      |
| Clock ngoài       | 8 MHz từ MCO của ST-LINK                                                                  | ⚠️ Phải chọn **HSE = BYPASS Clock Source**, không phải Crystal                   |
| LCD 2.4" QVGA     | ILI9341 — điều khiển qua **SPI5** (PF7/PF8/PF9, CS PC2, WRX PD13) + hiển thị qua **LTDC** | Rất tốn chân                                                                     |
| SDRAM 64 Mbit     | IS42S16400J qua **FMC**                                                                   | Chiếm nhiều chân PD/PE/PF/PG                                                     |
| Gyroscope         | L3GD20 (3 trục) qua **SPI5**                                                              | Dùng chung SPI5 với LCD, khác chân CS                                            |
| USB               | OTG HS chạy ở chế độ FS: PB12/PB14/PB15                                                   | Micro-AB                                                                         |
| Header mở rộng    | Đưa ra toàn bộ I/O của LQFP144                                                            | Nơi cắm cảm biến ngoài                                                           |

### 4.2 Hai cái bẫy lớn nhất của board này

1. **HSE là clock bypass 8 MHz**, không phải thạch anh. Trong CubeMX Clock Configuration phải chọn `HSE → BYPASS Clock Source`. Chọn sai → gọi `HAL_RCC_OscConfig` trả về lỗi → chương trình kẹt trong `Error_Handler()` ngay từ đầu, LED không nháy, và bạn sẽ ngồi debug code GPIO vô ích.
2. **ST-LINK trên board này không có Virtual COM Port** (khác với dòng Nucleo). Muốn `printf` ra máy tính, bạn có 3 lựa chọn:
   - Gắn module **USB–TTL** ngoài vào USART1 (PA9 = TX, PA10 = RX) + chung GND.
   - Dùng **SWO/ITM trace** qua chân PB3 (ST-LINK hỗ trợ, xem trong CubeIDE: _SWV ITM Data Console_).
   - Xem biến trực tiếp bằng **Live Expressions** trong debugger (đơn giản nhất, khuyên dùng lúc đầu).

### 4.3 Quy tắc chọn chân cho ngoại vi mới

Khi đề bài yêu cầu thêm cảm biến ngoài:

1. Mở **Datasheet → bảng _Alternate function mapping_** (bảng lớn liệt kê từng chân PA0…PG15 và 16 chức năng AF0–AF15).
2. Tìm dòng của ngoại vi cần (VD `I2C1_SCL`) → xem có những chân nào.
3. Đối chiếu với **UM1670/schematic**: chân đó đã bị LCD/SDRAM/gyro chiếm chưa?
4. Đối chiếu xem chân đó có ra **header mở rộng** không (nếu cần cắm dây).
5. Chốt chân → ghi vào bảng.

> Mẹo: CubeMX làm bước 1–2 giúp bạn. Chọn ngoại vi trong CubeMX, các chân khả dụng sẽ tự sáng lên; click chuột phải vào một chân để xem danh sách chức năng. Nhưng **CubeMX không biết board có gì** — bước 3 vẫn phải do bạn đọc UM1670.

---

## 5. PHA 4 — Đọc Datasheet và Reference Manual đúng cách

Tuyệt đối **không đọc từ đầu đến cuối**. Cả hai đều là từ điển tra cứu.

### 5.1 Datasheet — dùng để trả lời "chân nào / giới hạn điện bao nhiêu"

| Mục trong Datasheet               | Bạn lấy được gì                                                        |
| --------------------------------- | ---------------------------------------------------------------------- |
| _Block diagram_                   | Ngoại vi nào nằm trên bus nào (AHB1/APB1/APB2) → ảnh hưởng clock       |
| _Pinouts and pin description_     | Sơ đồ chân LQFP144, tên chân                                           |
| **Alternate function mapping** ⭐ | **Bảng quan trọng nhất**: chân X + AFn = chức năng gì                  |
| _Memory mapping_                  | Địa chỉ cơ sở của từng ngoại vi (VD GPIOA = `0x40020000`)              |
| _Electrical characteristics_      | Dòng tối đa mỗi chân (~20–25 mA), mức logic VIH/VIL, tốc độ chuyển mức |

### 5.2 Reference Manual (RM0090) — dùng để trả lời "ngoại vi hoạt động thế nào"

Cấu trúc mỗi chương ngoại vi trong RM luôn giống nhau — hãy đọc theo đúng thứ tự này:

1. **Introduction / Main features** — ngoại vi làm được gì (đọc 2 phút).
2. **Functional description** — sơ đồ khối + cách hoạt động (đọc kỹ, đây là phần "hiểu").
3. **Interrupts / DMA requests** — có những cờ ngắt nào (nếu bài dùng interrupt).
4. **Registers** — chỉ tra khi cần, hoặc khi debug muốn xem giá trị thanh ghi.

Các chương bạn sẽ dùng nhiều nhất cho bài cơ bản:

| Cần gì                               | Chương RM0090                                             |
| ------------------------------------ | --------------------------------------------------------- |
| Bật clock cho ngoại vi, cấu hình PLL | **RCC** (Reset and clock control)                         |
| Cấu hình chân                        | **GPIO**                                                  |
| Ngắt ngoài từ chân                   | **EXTI** (Interrupts and events)                          |
| Định thời, PWM, capture              | **TIM** (General-purpose timers, chương riêng cho TIM2–5) |
| Đọc analog                           | **ADC**                                                   |
| Giao tiếp máy tính                   | **USART**                                                 |
| Cảm biến I2C/SPI                     | **I2C** / **SPI**                                         |
| Truyền nền                           | **DMA**                                                   |

### 5.3 Kỹ thuật "3 câu hỏi" khi đọc RM cho một ngoại vi mới

Với bất kỳ ngoại vi nào, chỉ cần trả lời 3 câu là đủ để cấu hình:

1. **Clock**: ngoại vi này ăn clock từ bus nào, tần số bao nhiêu?
2. **Chu trình hoạt động**: cần bật bit nào để chạy, dữ liệu vào/ra qua thanh ghi nào, cờ nào báo "xong"?
3. **Sự kiện**: nó sinh ra ngắt/DMA request trong tình huống nào?

---

## 6. PHA 5 — Tính toán trên giấy trước khi mở CubeMX

### 6.1 Clock tree (bắt buộc hiểu)

```
HSE 8 MHz (bypass từ ST-LINK)
   → /M  → PLL VCO input (khuyến nghị 1–2 MHz)
   → ×N  → VCO output (100–432 MHz)
   → /P  → SYSCLK (tối đa 180 MHz)
        → AHB  prescaler → HCLK  (tối đa 180 MHz)  → CPU, SysTick, DMA, GPIO
        → APB1 prescaler → PCLK1 (tối đa 45 MHz)   → TIM2-7,12-14, USART2/3, I2C, SPI2/3
        → APB2 prescaler → PCLK2 (tối đa 90 MHz)   → TIM1,8-11, USART1/6, SPI1/4/5/6, ADC
```

Cấu hình kinh điển cho 180 MHz: `M=8, N=360, P=2, Q=7`, AHB `/1`, APB1 `/4`, APB2 `/2`.

> 🔑 **Luật timer phải nhớ**: nếu APBx prescaler khác 1, thì **clock vào timer = PCLKx × 2**.
> Với cấu hình trên: PCLK1 = 45 MHz → **TIM2–7 nhận 90 MHz**; PCLK2 = 90 MHz → **TIM1/8–11 nhận 180 MHz**.
> Sai chỗ này là nguyên nhân số 1 khiến LED nháy sai nhịp.

### 6.2 Công thức Timer

```
f_tick  = f_timer_clock / (PSC + 1)
T_period = (ARR + 1) / f_tick
```

**Ví dụ:** muốn TIM3 ngắt mỗi **500 ms**, biết TIM3 nhận 90 MHz.

- Chọn PSC sao cho tick dễ tính: `PSC = 8999` → f_tick = 90 000 000 / 9000 = **10 000 Hz** (1 tick = 0.1 ms).
- Cần 500 ms = 5000 tick → `ARR = 4999`.
- ✅ `PSC = 8999`, `ARR = 4999`. (ARR max của TIM3 là 65535 → hợp lệ)

**Ví dụ PWM:** muốn tần số 1 kHz, duty 25%, timer 90 MHz.

- `PSC = 89` → f_tick = 1 MHz. `ARR = 999` → 1 kHz. `CCR = 250` → 25%.

### 6.3 Công thức khác hay dùng

| Mục                   | Công thức                                                               |
| --------------------- | ----------------------------------------------------------------------- |
| ADC → điện áp         | `V = ADC_value × 3.3 / 4095` (12-bit)                                   |
| Thời gian lấy mẫu ADC | `T = (sampling_time + 12) chu kỳ ADC_CLK`                               |
| UART baudrate         | CubeMX tự tính từ PCLK; chỉ cần kiểm tra ô _Baud Rate_ không báo lỗi đỏ |
| SysTick               | Mặc định HAL đặt 1 ms → `HAL_Delay(ms)`, `HAL_GetTick()`                |

---

## 7. PHA 6 — Cấu hình CubeMX (checklist cố định 12 bước)

Luôn làm **đúng thứ tự này**, mỗi lần đều như nhau. Thứ tự quan trọng vì bước sau phụ thuộc bước trước.

```
[ ]  1. File → New → STM32 Project → tab "Board Selector" → gõ STM32F429I-DISC1 → chọn
       → hộp thoại "Initialize peripherals with default Mode?" →
         • Chọn NO nếu muốn tự cấu hình từ đầu (KHUYẾN NGHỊ khi học)
         • Chọn YES nếu muốn dùng luôn LCD/SDRAM có sẵn (sẽ sinh rất nhiều code)

[ ]  2. Đặt tên project, chọn Targeted Language = C, Toolchain = STM32CubeIDE

[ ]  3. Tab "Pinout & Configuration" → System Core → RCC:
       → High Speed Clock (HSE) = **BYPASS Clock Source**   ← BẪY SỐ 1
       → (Low Speed Clock LSE = Disable, trừ khi dùng RTC)

[ ]  4. System Core → SYS:
       → Debug = **Serial Wire**                            ← BẪY SỐ 2
         (không chọn → sau lần nạp đầu tiên sẽ không kết nối lại được)
       → Timebase Source = SysTick (giữ mặc định)

[ ]  5. Tab "Clock Configuration":
       → Nhập 180 vào ô HCLK rồi Enter → CubeMX tự giải PLL
       → Kiểm tra APB1 ≤ 45 MHz, APB2 ≤ 90 MHz, không có ô nào đỏ
       → GHI LẠI: APB1 Timer clocks = ___ MHz, APB2 Timer clocks = ___ MHz

[ ]  6. Cấu hình GPIO (theo bảng chân đã chốt ở Pha 3):
       → Click trực tiếp lên chân trong sơ đồ → chọn GPIO_Output / GPIO_Input
       → Vào System Core → GPIO để đặt: Output level, Push-Pull/Open-Drain,
         Pull-up/Pull-down, Maximum output speed, **User Label** (VD: LED_GREEN)
       → User Label rất đáng giá: code sẽ dùng LED_GREEN_GPIO_Port / LED_GREEN_Pin

[ ]  7. Cấu hình các ngoại vi giao tiếp (UART / I2C / SPI / ADC):
       → Chọn Mode trước (VD USART1 → Asynchronous)
       → Sang tab Parameter Settings đặt thông số (baudrate, word length, ...)
       → Kiểm tra chân được gán có đúng chân đã chốt không; nếu không, click chân
         và đổi thủ công

[ ]  8. Cấu hình Timer (nếu có):
       → Chọn Clock Source = Internal Clock  ← nếu quên, timer không chạy
       → Nhập Prescaler (PSC) và Counter Period (ARR) đã tính ở Pha 5
       → Nếu PWM: chọn Channel → PWM Generation CHx, đặt Pulse (CCR)

[ ]  9. Cấu hình ngắt (nếu có) — tab NVIC Settings:
       → Tick "enabled" cho: TIMx global interrupt / EXTI line / USARTx global interrupt
       → Đặt Preemption Priority (số nhỏ = ưu tiên cao). Giữ ≥ 5 để không đè SysTick.

[ ] 10. Cấu hình DMA (nếu có) — tab DMA Settings → Add → chọn request, Mode
       (Normal/Circular), Data Width
       ⚠️ Khi bật DMA thì phải bật luôn ngắt của ngoại vi tương ứng ở NVIC

[ ] 11. Tab "Project Manager" → Code Generator:
       → ✅ Generate peripheral initialization as a pair of .c/.h files per peripheral
       → ✅ Keep User Code when re-generating
       (giúp code gọn và không mất code của bạn khi sinh lại)

[ ] 12. Bấm "GENERATE CODE" (Alt+K) → Open Project
```

### Sau khi sinh code, mở `main.c` và đọc theo thứ tự:

```
main()
 ├── HAL_Init();                 // reset ngoại vi, cấu hình SysTick, NVIC group
 ├── SystemClock_Config();       // ← xác lập 180 MHz; lỗi ở đây → Error_Handler()
 ├── MX_GPIO_Init();             // luôn được gọi ĐẦU TIÊN trong các MX_*
 ├── MX_USART1_UART_Init();      // các ngoại vi khác
 ├── MX_TIM3_Init();
 └── while (1) { ... }           // vòng lặp vô tận của bạn
```

---

## 8. PHA 7 — Viết code HAL

### 8.1 Bản đồ file

| File                            | Vai trò                                     | Bạn sửa ở đâu                                  |
| ------------------------------- | ------------------------------------------- | ---------------------------------------------- |
| `Core/Src/main.c`               | Khởi tạo + vòng lặp chính                   | Trong các khối `/* USER CODE BEGIN ... END */` |
| `Core/Inc/main.h`               | Define chân (từ User Label)                 | `USER CODE`                                    |
| `Core/Src/stm32f4xx_it.c`       | **Các hàm ISR**                             | Thân các `*_IRQHandler`                        |
| `Core/Src/stm32f4xx_hal_msp.c`  | Khởi tạo mức thấp (clock ngoại vi, chân AF) | Thường không sửa                               |
| `Drivers/STM32F4xx_HAL_Driver/` | Thư viện HAL                                | **Không bao giờ sửa**                          |

> ⚠️ **Mọi code bạn viết trong `main.c` phải nằm giữa `USER CODE BEGIN` và `USER CODE END`.** Code nằm ngoài sẽ bị xoá khi bạn sinh lại từ CubeMX.

### 8.2 Các hàm HAL cần thuộc lòng (bài cơ bản)

```c
/* ---------- GPIO ---------- */
HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);    // bật
HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
GPIO_PinState s = HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);

/* ---------- Delay / tick ---------- */
HAL_Delay(500);              // blocking, dựa trên SysTick
uint32_t t = HAL_GetTick();  // ms kể từ lúc khởi động — dùng cho non-blocking delay

/* ---------- UART ---------- */
HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
HAL_UART_Receive_IT(&huart1, &rxByte, 1);          // nhận 1 byte bằng ngắt

/* ---------- Timer ---------- */
HAL_TIM_Base_Start_IT(&htim3);                     // chạy timer + bật ngắt update
HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);          // chạy PWM
__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 250); // đổi duty lúc chạy

/* ---------- ADC ---------- */
HAL_ADC_Start(&hadc1);
HAL_ADC_PollForConversion(&hadc1, 10);
uint32_t v = HAL_ADC_GetValue(&hadc1);
HAL_ADC_Stop(&hadc1);

/* ---------- I2C / SPI ---------- */
HAL_I2C_Mem_Read(&hi2c1, DEV_ADDR<<1, REG, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
HAL_SPI_TransmitReceive(&hspi5, txbuf, rxbuf, len, 100);
```

### 8.3 Ba callback quan trọng (viết trong `main.c`, vùng USER CODE 4)

HAL gọi các hàm này thay bạn — chỉ cần định nghĩa lại (chúng là `__weak`):

```c
/* Timer tràn (update event) */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3) {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }
}

/* Ngắt ngoài từ chân GPIO */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_Pin) {
        flag_button = 1;          // chỉ set cờ, xử lý ở while(1)
    }
}

/* UART nhận xong */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        rx_ready = 1;
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);   // luôn re-arm
    }
}
```

**Quy tắc ISR:** ngắn, không `HAL_Delay`, không `printf`, không vòng lặp chờ. Chỉ **set cờ** rồi để `while(1)` xử lý. Mọi biến chia sẻ giữa ISR và `main` phải khai báo `volatile`.

### 8.4 Mẫu vòng lặp chính không blocking (nên dùng từ sớm)

```c
/* USER CODE BEGIN PV */
volatile uint8_t  flag_button = 0;
uint32_t last_blink = 0, last_log = 0;
uint32_t blink_period = 500;
/* USER CODE END PV */

while (1)
{
    uint32_t now = HAL_GetTick();

    /* Nhiệm vụ 1: nháy LED */
    if (now - last_blink >= blink_period) {
        last_blink = now;
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }

    /* Nhiệm vụ 2: gửi log mỗi 1 s */
    if (now - last_log >= 1000) {
        last_log = now;
        char buf[48];
        int n = snprintf(buf, sizeof(buf), "period=%lu ms\r\n", blink_period);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, n, 100);
    }

    /* Nhiệm vụ 3: xử lý nút (cờ được set trong EXTI callback) */
    if (flag_button) {
        flag_button = 0;
        blink_period = (blink_period == 500) ? 100 : 500;
    }
}
```

Mẫu này (gọi là _cooperative scheduler_ / _super loop with timers_) giải quyết được 90% bài tập cơ bản và là bước đệm tự nhiên sang RTOS sau này.

### 8.5 Chống dội nút (debounce) — kiểu đơn giản, đúng chuẩn

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_irq = 0;
    uint32_t now = HAL_GetTick();
    if (GPIO_Pin == BTN_Pin && (now - last_irq) > 200) {   // bỏ qua < 200 ms
        last_irq = now;
        flag_button = 1;
    }
}
```

---

## 9. PHA 8 — Build, Nạp, Debug

### 9.1 Quy trình

```
1. Project → Build Project (Ctrl+B)        → 0 errors, 0 warnings
2. Cắm USB Mini-B vào cổng ST-LINK (không phải cổng USB User)
3. Run → Debug As → STM32 C/C++ Application  (hoặc Run để chỉ nạp và chạy)
4. Debugger dừng ở main() → F8 (Resume) để chạy
```

### 9.2 Bộ công cụ debug theo thứ tự nên dùng

| Công cụ                           | Dùng khi nào                                               | Cách mở                                           |
| --------------------------------- | ---------------------------------------------------------- | ------------------------------------------------- |
| **Breakpoint + Step (F5/F6)**     | Kiểm tra logic có chạy tới đó không                        | Double-click lề trái                              |
| **Live Expressions** ⭐           | Xem biến thay đổi _trong lúc chạy_ — hữu ích nhất          | Window → Show View → Live Expressions             |
| **SFRs view**                     | Xem trực tiếp giá trị thanh ghi (GPIOG->ODR, TIM3->CNT...) | Window → Show View → SFRs                         |
| **SWV ITM Data Console**          | `printf` qua SWO, không cần dây UART                       | Cấu hình Debug Configuration → Serial Wire Viewer |
| **UART + terminal**               | Log dài, chạy thực tế không cần debugger                   | PuTTY / TeraTerm / Serial Monitor                 |
| **Oscilloscope / logic analyzer** | Khi nghi ngờ tín hiệu vật lý (PWM, I2C, SPI)               | Phần cứng ngoài                                   |

### 9.3 Cây chẩn đoán lỗi — "Nạp xong mà không chạy"

```
Không có gì xảy ra
│
├─ Debugger không kết nối được?
│   ├─ Kiểm tra cắm đúng cổng USB ST-LINK
│   ├─ Thử: Run → Debug Configurations → Debugger → Reset behaviour = "Connect under reset"
│   └─ Nếu lần trước quên bật SYS Debug = Serial Wire → giữ nút RESET khi bắt đầu nạp
│
├─ Chương trình kẹt trong Error_Handler()?
│   → Đặt breakpoint trong Error_Handler(), xem call stack
│   → 90% là SystemClock_Config() lỗi → kiểm tra HSE = BYPASS
│
├─ Rơi vào HardFault_Handler()?
│   → Dùng con trỏ NULL, tràn mảng, chia 0, hoặc dùng ngoại vi chưa bật clock
│
├─ Chạy tới while(1) nhưng LED không sáng?
│   ├─ Đúng chân chưa? (đối chiếu lại UM1670)
│   ├─ Clock của GPIO port đã bật chưa? (MX_GPIO_Init có được gọi không)
│   ├─ Mức logic đúng chưa? (LED active-high hay low)
│   └─ Dùng SFRs xem GPIOG->ODR có đổi bit không → nếu có đổi mà LED tắt ⇒ lỗi phần cứng/chân
│
├─ Timer không sinh ngắt?
│   ├─ Có gọi HAL_TIM_Base_Start_IT() chưa? (không phải _Start)
│   ├─ NVIC đã tick enable chưa?
│   ├─ Clock Source = Internal Clock chưa?
│   └─ Callback có đúng tên HAL_TIM_PeriodElapsedCallback không (sai tên = không ai gọi)
│
├─ LED nháy nhưng sai nhịp?
│   → Tính lại: clock timer = PCLKx × 2 khi prescaler ≠ 1
│
└─ UART ra ký tự rác?
    ├─ Baudrate hai đầu khác nhau
    ├─ Chưa nối chung GND
    ├─ Clock hệ thống thực tế ≠ clock CubeMX giả định
    └─ Nhầm TX/RX (phải nối chéo: TX↔RX)
```

---

## 10. Ví dụ chạy trọn framework

**Đề bài:** _"Viết chương trình cho STM32F429I-DISC1: LED xanh nháy chu kỳ 500 ms. Mỗi lần nhấn nút USER, chu kỳ đổi luân phiên giữa 500 ms và 100 ms. Mỗi giây gửi chu kỳ hiện tại ra máy tính qua UART."_

### Pha 1 — Phân tích

| Mục                 | Nội dung                                                                               |
| ------------------- | -------------------------------------------------------------------------------------- |
| Input               | Nút USER (digital, bất kỳ lúc nào, cần chống dội)                                      |
| Output              | LED xanh (digital, 500/100 ms); Chuỗi text ra PC (1 Hz, UART)                          |
| Ràng buộc thời gian | Có — "500 ms", "mỗi giây" ⇒ dùng `HAL_GetTick()` hoặc TIM; phản hồi nút tức thì ⇒ EXTI |
| Trạng thái          | `blink_period` ∈ {500, 100}                                                            |
| Tiêu chí xong       | LED đổi nhịp khi nhấn; terminal in đúng giá trị                                        |

### Pha 2 — Ngoại vi

| Chức năng | Ngoại vi          | Mô hình                           |
| --------- | ----------------- | --------------------------------- |
| LED       | GPIO Output       | Non-blocking bằng `HAL_GetTick()` |
| Nút       | GPIO Input + EXTI | Interrupt + debounce              |
| Log       | USART1            | Polling (`HAL_UART_Transmit`)     |

### Pha 3 — Ánh xạ board (kiểm chứng trong UM1670)

| Tín hiệu     | Chân | Ghi chú                                           |
| ------------ | ---- | ------------------------------------------------- |
| LED xanh LD3 | PG13 | Active high                                       |
| Nút USER B1  | PA0  | Active high ⇒ EXTI **Rising edge**, Pull-**down** |
| USART1_TX    | PA9  | Nối TX module USB-TTL vào RX                      |
| USART1_RX    | PA10 |                                                   |

### Pha 4 — Tra tài liệu

- Datasheet → _Alternate function mapping_: PA9/PA10 = `USART1_TX/RX` tại **AF7**. ✅
- RM0090 → _EXTI_: PA0 thuộc **EXTI line 0**, vector `EXTI0_IRQn`.
- RM0090 → _RCC_: USART1 nằm trên **APB2**.

### Pha 5 — Tính toán

- SYSCLK 180 MHz (M=8, N=360, P=2), APB1 /4, APB2 /2.
- Không dùng timer phần cứng → chỉ cần SysTick 1 ms (mặc định).
- UART 115200-8-N-1.

### Pha 6 — CubeMX

- RCC: HSE = **BYPASS**; SYS: Debug = **Serial Wire**
- Clock: HCLK = 180
- PG13 → `GPIO_Output`, label `LED_GREEN`
- PA0 → `GPIO_EXTI0`, Pull-down, Trigger = **Rising edge**, label `BTN`
- NVIC: tick `EXTI line0 interrupt`
- USART1 → Asynchronous, 115200
- Generate Code

### Pha 7 — Code

```c
/* USER CODE BEGIN PV */
volatile uint8_t flag_button = 0;
uint32_t blink_period = 500;
/* USER CODE END PV */

/* USER CODE BEGIN 2 */
char msg[] = "System started\r\n";
HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
/* USER CODE END 2 */

/* USER CODE BEGIN WHILE */
uint32_t last_blink = 0, last_log = 0;
while (1)
{
    uint32_t now = HAL_GetTick();

    if (now - last_blink >= blink_period) {
        last_blink = now;
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }

    if (now - last_log >= 1000) {
        last_log = now;
        char buf[40];
        int n = snprintf(buf, sizeof(buf), "period = %lu ms\r\n", blink_period);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, n, 100);
    }

    if (flag_button) {
        flag_button = 0;
        blink_period = (blink_period == 500) ? 100 : 500;
    }
}
/* USER CODE END WHILE */

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_irq = 0;
    uint32_t now = HAL_GetTick();
    if (GPIO_Pin == BTN_Pin && (now - last_irq) > 200) {
        last_irq = now;
        flag_button = 1;
    }
}
/* USER CODE END 4 */
```

Nhớ `#include <string.h>` và `#include <stdio.h>` trong `USER CODE BEGIN Includes`.

### Pha 8 — Verify

- LED nháy ~1 Hz (bật 500 ms, tắt 500 ms) → chu kỳ đầy đủ 1 s.
- Nhấn nút → nhanh hơn rõ rệt.
- Terminal 115200 in `period = 500 ms` mỗi giây.

---

## 11. Sai lầm thường gặp của người mới (đọc lại trước mỗi bài)

1. ❌ Mở CubeMX trước khi phân tích xong đề → quay lại điền Phiếu Pha 1.
2. ❌ Quên HSE = **BYPASS** trên board Discovery F429.
3. ❌ Quên SYS Debug = **Serial Wire** → khoá luôn chip (cứu bằng "Connect under reset").
4. ❌ Viết code ngoài vùng `USER CODE` → mất sạch khi regenerate.
5. ❌ Dùng `HAL_TIM_Base_Start()` thay vì `HAL_TIM_Base_Start_IT()` khi cần ngắt.
6. ❌ Quên nhân đôi clock timer khi APB prescaler ≠ 1.
7. ❌ Biến chia sẻ giữa ISR và main không khai báo `volatile` → trình biên dịch tối ưu mất.
8. ❌ Gọi `HAL_Delay()` bên trong ISR → treo hệ thống (SysTick ưu tiên thấp hơn).
9. ❌ Địa chỉ I2C quên dịch trái 1 bit (`addr << 1`) khi dùng HAL.
10. ❌ Không nối chung GND giữa board và module ngoài.
11. ❌ Dùng `HAL_Delay` cho nhiều nhiệm vụ song song → dùng mẫu `HAL_GetTick()` ở §8.4.
12. ❌ Tin bảng chân chép trên mạng thay vì mở UM1670/schematic.

---

## 12. Lộ trình luyện tập đề xuất

| Tuần | Bài tập                                               | Kỹ năng mới                                       |
| ---- | ----------------------------------------------------- | ------------------------------------------------- |
| 1    | Nháy LED (polling) → nháy LED (SysTick non-blocking)  | CubeMX cơ bản, GPIO Output, build/flash/debug     |
| 1    | Đọc nút bằng polling → bằng EXTI                      | GPIO Input, EXTI, NVIC, debounce, `volatile`      |
| 2    | LED nháy bằng TIM ngắt update                         | Timer, PSC/ARR, clock tree, callback              |
| 2    | PWM điều chỉnh độ sáng LED theo biến trở              | TIM PWM, ADC polling                              |
| 3    | `printf` ra UART, nhận lệnh từ terminal bằng ngắt     | USART TX/RX, ngắt UART, parse chuỗi               |
| 3    | ADC + DMA circular, tính trung bình động              | DMA, buffer, callback half/full                   |
| 4    | Đọc gyroscope L3GD20 qua SPI5 (có sẵn trên board)     | SPI, datasheet cảm biến, thanh ghi thiết bị ngoài |
| 4    | Hiển thị số liệu lên LCD (dùng BSP trong STM32CubeF4) | BSP, LTDC/SDRAM ở mức sử dụng                     |

Sau tuần 4, việc chạy một mô hình TFLite Micro / CMSIS-NN trên board sẽ trở thành bài toán quen thuộc: nó chỉ là "input từ ngoại vi → buffer → inference → output ra ngoại vi", đúng khung framework này.

---

## 13. Phiếu rút gọn — in ra dán bàn

```
□ 1. Viết lại đề bằng 1 câu của tôi
□ 2. Liệt kê INPUT / OUTPUT / ràng buộc thời gian / trạng thái
□ 3. Dịch sang danh sách ngoại vi (dùng bảng §2.2)
□ 4. Chọn mô hình: polling / interrupt / DMA
□ 5. Mở UM1670 + schematic → chốt chân, kiểm tra chân có bị chiếm không
□ 6. Mở Datasheet (AF mapping) → xác nhận chân hợp lệ
□ 7. Mở RM0090 → trả lời 3 câu: clock? chu trình? sự kiện ngắt?
□ 8. Tính trên giấy: PLL, clock timer, PSC/ARR, baudrate
□ 9. CubeMX theo checklist 12 bước (RCC BYPASS! SYS Serial Wire!)
□ 10. Code trong USER CODE, ISR ngắn, volatile
□ 11. Build → Debug → Live Expressions
□ 12. Verify theo tiêu chí "xong bài" đã viết ở bước 2
```

---

_Ghi chú: mọi số chân và thông số board trong tài liệu này cần được tự kiểm chứng lại trong UM1670 và schematic MB1075 ứng với revision board thực tế. Chính thao tác kiểm chứng đó là kỹ năng cốt lõi mà framework này muốn xây dựng._
