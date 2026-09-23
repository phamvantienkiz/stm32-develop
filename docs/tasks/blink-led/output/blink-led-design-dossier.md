# Design dossier — Blink LED (LD3 Green)

| | |
|---|---|
| Task ID | `blink-led` |
| Board | STMicroelectronics STM32F429I-DISC1 (MB1075) |
| MCU | STM32F429ZIT6 (Arm® Cortex®-M4F, LQFP144, 2 MB Flash, 256 KB SRAM) |
| Date | 2026-09-20 |
| Documents consulted | UM1670 Rev 6 (Discovery User Manual), MB1075 Rev C.01 (Schematic), DS9773 Rev 7 (STM32F429xx Datasheet), RM0090 Rev 19 (STM32F429 Reference Manual) |

---

## 0. Open items — confirm before building

| # | Item | Why it matters | How to confirm | Status |
|---|---|---|---|---|
| 1 | LD3 pin & polarity | Driving the wrong pin or wrong level results in no blink | UM1670 §7.5 p.20; MB1075 Sheet 6 | **CONFIRMED**: PG13, Active High |
| 2 | HSE clock source type | Selecting Crystal instead of Bypass causes `HAL_RCC_OscConfig()` to timeout and trap in `Error_Handler()` | UM1670 §7.12.1 p.22; MB1075 Sheet 5 | **CONFIRMED**: HSE is 8 MHz BYPASS from ST-LINK MCO |
| 3 | SYS Debug configuration | Omitting Serial Wire debug disconnects the SWD probe on next reboot | RM0090 §8.3.2 p.270; UM1670 §7.3 p.16 | **CONFIRMED**: PA13 (SWDIO) & PA14 (SWCLK) must remain Serial Wire |

---

## 1. Problem analysis

**Restated in one sentence:** Điều khiển đèn LED người dùng LD3 (màu xanh lá) trên bo mạch STM32F429I-DISC1 nhấp nháy tuần hoàn với chu kỳ sáng 1 giây và tắt 1 giây (chu kỳ toàn phần 2 giây, tần số 0.5 Hz).

### Assumptions made

| Assumption | Why | Impact if wrong |
|---|---|---|
| Chu kỳ nhấp nháy: 1s ON, 1s OFF | Đề bài yêu cầu: "nhấp nháy 1 lần mỗi 1 giây (sáng 1 giây, tắt 1 giây và lặp lại liên tục)" | Ảnh hưởng đến tham số định thời hằng số `BLINK_PERIOD_MS = 1000` |
| Sử dụng mô hình non-blocking `HAL_GetTick()` làm nòng cốt, kèm ví dụ `HAL_Delay()` | `HAL_Delay()` làm CPU bị nghẽn (blocking), trong khi `HAL_GetTick()` chuẩn phong cách embedded chuyên nghiệp | Code sẵn sàng để mở rộng thêm nút nhấn hay ngoại vi khác mà không bị delay chặn lại |
| Khởi tạo cấu hình bo mạch ở chế độ tối giản ("Default Mode = No") | Tránh sinh mã dư thừa cho LCD, SDRAM, MEMS không liên quan đến bài tập | Giảm thiểu mã nguồn, tăng tốc độ nạp/biên dịch và dễ học |

### Inputs

| Source | Signal type | Rate / trigger | Electrical notes |
|---|---|---|---|
| Không có (hệ thống tự hoạt động theo thời gian) | N/A | N/A | Không dùng nút nhấn hay cảm biến ngoài trong pha cơ bản này |

### Outputs

| Destination | Signal type | Rate | Electrical notes |
|---|---|---|---|
| LD3 Green LED (onboard) | Digital Output (Push-Pull) | Chuyển trạng thái mỗi 1000 ms (0.5 Hz) | Kết nối qua trở hạn dòng R26 (510 Ω) xuống GND. Dòng tải xấp xỉ ~2.7 mA (trong ngưỡng 25 mA tối đa của I/O STM32F429) `[MB1075 Sheet 6; DS9773 Table 12 p.82]` |

### Timing constraints

| Constraint | Source in the statement | Consequence for the design |
|---|---|---|
| Thời gian sáng: 1.0 giây ± 1% | Đề bài yêu cầu | Độ trễ giữa các lần đổi mức logic là 1000 ms |
| Thời gian tắt: 1.0 giây ± 1% | Đề bài yêu cầu | Độ trễ giữa các lần đổi mức logic là 1000 ms |
| Chu kỳ tổng: 2.0 giây | Đề bài yêu cầu | Tần số chớp LED = 0.5 Hz |

### State to maintain

| Variable | Type | Meaning | Shared with an ISR? |
|---|---|---|---|
| `t_last_blink` | `uint32_t` | Lưu thời điểm tick (`HAL_GetTick()`) lần đảo trạng thái LED gần nhất | Không (chỉ dùng trong `main()` while-loop) |

### Acceptance criteria

- [ ] Bo mạch khởi động bình thường, không bị kẹt trong `Error_Handler()`.
- [ ] Đèn LED LD3 (xanh lá cây) sáng rõ ràng trong đúng 1 giây (1000 ms).
- [ ] Đèn LED LD3 tắt hoàn toàn trong đúng 1 giây (1000 ms).
- [ ] Quá trình lặp lại tuần hoàn, nhịp nhàng, có thể quan sát bằng mắt thường hoặc đo chân PG13 bằng Logic Analyzer / Oscilloscope với chu kỳ $T = 2.0\text{ s} \pm 10\text{ ms}$.

---

## 2. Peripheral selection

| Requirement | Peripheral | Processing model | Why this, not the alternative |
|---|---|---|---|
| Điều khiển chân bật/tắt LED LD3 | **GPIO Output** (Port G, Pin 13) | Push-pull, No pull | LD3 đã có mạch phần cứng với trở nối mass (GND); chế độ Push-Pull cho phép cấp điện áp 3.3V trực tiếp vào Anode của LED `[MB1075 Sheet 6]` |
| Định thời 1000 ms | **SysTick (`HAL_GetTick()`)** | Polling non-blocking | SysTick đã được khởi tạo mặc định 1 ms bởi HAL. Dùng `HAL_GetTick()` tránh blocking CPU như `HAL_Delay()`, không cần chiếm dụng thêm Hardware Timer (TIMx) cho bài toán cơ bản |

**Escalation justification:** Không cần nâng cấp lên Timer Interrupt hay DMA cho bài toán blink LED đơn lẻ. SysTick 1 ms là quá đủ cho yêu cầu định thời 1 giây của người dùng.

---

## 3. Pin map

| Signal | Pin | Mode | Pull | Speed / AF | Active level | Source |
|---|---|---|---|---|---|---|
| `LD3_GREEN` | **PG13** (LQFP144 Pin 128) | `GPIO_Output (Push-Pull)` | `No pull` | `Low` (hoặc `Medium`) | **Active High** (High = LED Sáng, Low = LED Tắt) | `[UM1670 §7.5 p.20, Table 7 p.29; MB1075 Sheet 6]` |

### Pin availability check

- Chân PG13 trên bo STM32F429I-DISC1 được nối trực tiếp vào LED LD3 (Green) và đưa ra header mở rộng **P1 pin 29** `[UM1670 Table 7 p.29]`.
- Chân PG13 **không** bị xung đột với SDRAM, LCD hay Gyroscope `[UM1670 Table 7 p.29]`.

### Pins deliberately avoided

| Pin | Why |
|---|---|
| **PA13 / PA14** | Chân nạp / gỡ lỗi SWD (`SWDIO` / `SWCLK`). Tuyệt đối không cấu hình làm GPIO thường vì sẽ làm mất kết nối nạp mạch ST-LINK `[UM1670 §7.3 p.19]`. |
| **PH0 / PH1** | Chân nguồn dao động ngoài HSE OSC_IN từ mạch nạp ST-LINK MCO `[UM1670 §7.12.1 p.22]`. |

---

## 4. Facts established from the documentation

### GPIO Port G (`GPIOG`)

| Question | Answer | Source |
|---|---|---|
| **Which bus and clock feeds it?** | Nằm trên bus **AHB1**; xung nhịp cấp bởi bit `GPIOGEN` trong thanh ghi `RCC_AHB1ENR` | `[RM0090 §2.3 p.65; §6.3.10 p.242]` |
| **What starts it, where does data flow, which flag says done?** | Sau khi bật bit `RCC_AHB1ENR_GPIOGEN`, thanh ghi cấu hình `GPIOG->MODER` được ghi giá trị `01b` tại vị trí pin 13 (bits 27:26) để thành General Purpose Output. Dữ liệu xuất ra qua thanh ghi `ODR` (Output Data Register) hoặc chốt qua `BSRR` (Bit Set/Reset Register). Không cần cờ báo done | `[RM0090 §8.4.1 p.281; §8.4.5 p.283; §8.4.7 p.284]` |
| **Which events raise an interrupt / DMA request?** | Không dùng ngắt / DMA cho việc xuất tín hiệu GPIO đơn giản này | `[RM0090 §8.2 p.270]` |
| **Relevant registers and bits** | - `RCC->AHB1ENR` (bit 6: `GPIOGEN` = 1)<br>- `GPIOG->MODER` (bits 27:26 = `01` cho General Purpose Output)<br>- `GPIOG->OTYPER` (bit 13 = `0` cho Push-Pull)<br>- `GPIOG->PUPDR` (bits 27:26 = `00` cho No Pull)<br>- `GPIOG->ODR` (bit 13: 1 = High, 0 = Low)<br>- `GPIOG->BSRR` (bit 13: Set pin 13; bit 29: Reset pin 13) | `[RM0090 §8.4 p.281-285]` |

---

## 5. Clock and timing calculations

### System clock tree

Bo mạch STM32F429I-DISC1 được cấp nguồn xung nhịp HSE 8 MHz lấy từ chân MCO của vi điều khiển ST-LINK/V2-B `[UM1670 §7.12.1 p.22]`. Cấu hình chuẩn để chạy tối đa 180 MHz:

```
HSE (Bypass 8 MHz) → /M (8) → f_VCO_in (1 MHz) → *N (360) → f_VCO_out (360 MHz) → /P (2) → SYSCLK = 180 MHz
AHB Prescaler /1  → HCLK  = 180 MHz  (Cấp cho CPU Cortex-M4, SysTick, GPIO Port A..K)
APB1 Prescaler /4 → PCLK1 = 45 MHz   (Tối đa cho APB1 là 45 MHz)
APB2 Prescaler /2 → PCLK2 = 90 MHz   (Tối đa cho APB2 là 90 MHz)
```
*Giới hạn bus: Datasheet STM32F429ZI Table 13 & RM0090 Figure 13.*

> **Lưu ý đơn giản hóa:** Nếu người dùng chỉ muốn chạy cấu hình xung nhịp mặc định nội (HSI 16 MHz), SysTick vẫn luôn được bộ thư viện HAL chuẩn hóa về đúng tần số 1 ms thông qua hàm `HAL_Init()` và `HAL_InitTick()`.

### Derived values

| Value | Derivation | Result |
|---|---|---|
| `SysTick frequency` | Được HAL cấu hình tự động: $f_{\text{SysTick}} = 1000\text{ Hz}$ (chu kỳ 1 ms) | `1 ms tick` |
| `Blink half-period` | Yêu cầu sáng 1s, tắt 1s: $T_{\text{toggle}} = 1000\text{ ms} = 1000\text{ ticks}$ | `1000 ms` |
| `Blink full period` | $T = 2 \times 1000\text{ ms} = 2000\text{ ms} = 2.0\text{ s}$ | `2.0 s (0.5 Hz)` |
| `Wrap-safe comparison` | Dùng phép trừ số nguyên không dấu: `(uint32_t)(now - t_last) >= 1000` | Tránh tràn số sau 49.7 ngày |

### Timing budget

- Hàm `HAL_GPIO_TogglePin()` chỉ tiêu tốn xấp xỉ 10–20 chu kỳ CPU (dưới 0.2 micro-giây ở 180 MHz).
- Vòng lặp `while(1)` non-blocking kiểm tra điều kiện sau mỗi vòng lặp rảnh rỗi mà không tiêu tốn thời gian chờ vô ích. Jitter định thời với SysTick 1 ms là $< 1\text{ ms}$ (sai số $< 0.1\%$).

---

## 6. STM32CubeMX configuration

Thực hiện từng bước theo đúng thứ tự dưới đây trong STM32CubeMX (hoặc trình tích hợp trong STM32CubeIDE):

```
□ 1. Tạo project mới:
     - File → New → STM32 Project.
     - Chọn tab "Board Selector" → Nhập: STM32F429I-DISC1 → Chọn bo mạch → Next.
     - Hộp thoại hỏi: "Initialize all peripherals with their default Mode?"
       → Chọn "No" (Để chỉ cấu hình các chân cần thiết, tránh tạo mã rác cho LCD/SDRAM).
     - Project Name: blink-led   | Language: C   | Targeted Project Type: STM32Cube.

□ 2. Cấu hình RCC (System Core → RCC):
     - Lựa chọn 1 (Khuyên dùng cho bài tập cơ bản - Đơn giản, độ ổn định tuyệt đối):
         + High Speed Clock (HSE): "Disable" (Hệ thống sẽ tự dùng bộ dao động nội HSI 16 MHz, không phụ thuộc cầu chì SB18).
         + Low Speed Clock (LSE): "Disable".
     - Lựa chọn 2 (Nếu muốn chạy tối đa 180 MHz từ xung ngoại):
         + High Speed Clock (HSE): Chọn "BYPASS Clock Source" (Bắt buộc kiểm tra cầu chì SB18 trên bo có được nối không).

□ 3. Cấu hình SYS (System Core → SYS):
     - Debug: Chọn "Serial Wire" (BẮT BUỘC: để không bị mất kết nối nạp/debug qua ST-LINK).
     - Timebase Source: Chọn "SysTick" (Mặc định).

□ 4. Tab Clock Configuration:
     - Với Lựa chọn 1 (HSI 16 MHz - Mặc định): Để nguyên mặc định HCLK = 16 MHz. Hệ thống chạy chuẩn từng giây ngay lập tức.
     - Với Lựa chọn 2 (HSE Bypass): Nhập Input frequency = 8 MHz, PLL Source = HSE, HCLK = 180 MHz.

□ 5. Cấu hình chân GPIO cho LED (Pinout view & System Core → GPIO):
     - Tìm chân PG13 trên sơ đồ chip (hoặc gõ PG13 vào ô tìm kiếm góc dưới bên phải).
     - Click chuột trái vào chân PG13 → Chọn "GPIO_Output".
     - Vào mục System Core → GPIO → Click chọn chân PG13 và cấu hình trong bảng thông số:
         + GPIO output level: Low (Mặc định khi bật nguồn là tắt LED).
         + GPIO mode: Output Push Pull.
         + GPIO Pull-up/Pull-down: No pull.
         + Maximum output speed: Low.
         + User Label: LD3_GREEN (CubeMX sẽ tự sinh hằng số LD3_GREEN_Pin và LD3_GREEN_GPIO_Port trong main.h).

□ 6. Cấu hình Project Manager → Code Generator:
     - Tích chọn: "Generate peripheral initialization as a pair of '.c/.h' files per peripheral".
     - Tích chọn: "Keep User Code when re-generating" (Bảo vệ code người dùng khi cấu hình lại).

□ 7. Sinh mã (Code Generation):
     - Nhấn Alt + K hoặc menu Project → Generate Code.
     - Nhấn "Open Project" để chuyển sang giao diện lập trình trong STM32CubeIDE.
```

### Configuration traps relevant to this task

| Trap | Symptom if you hit it | Correct setting |
|---|---|---|
| Chọn HSE là `Crystal/Ceramic Resonator` | Chương trình chạy vào `SystemClock_Config()` sẽ bị timeout và nhảy vào vòng lặp vô tận trong `Error_Handler()`, LED không bao giờ sáng | Chọn chính xác **BYPASS Clock Source** trong `System Core → RCC → HSE` `[UM1670 §7.12.1 p.22]` |
| Quên bật `Debug: Serial Wire` | Sau lần nạp đầu tiên, IDE không thể nhận diện chip và báo lỗi "ST-LINK target not found" | Chọn **Serial Wire** trong `System Core → SYS → Debug` |
| Viết mã ngoài khối `/* USER CODE BEGIN ... */` | Mỗi lần mở lại file `.ioc` để sửa cấu hình và nhấn Generate Code, toàn bộ code tự viết bị xóa sạch | Luôn đặt code bên trong các cặp thẻ `USER CODE BEGIN` và `USER CODE END` |

---

## 7. Implementation plan

### Program structure

```
[Khởi động]
    HAL_Init()               → Khởi tạo tick 1 ms (SysTick) và Flash prefetch
    SystemClock_Config()     → Cấu hình PLL 180 MHz từ nguồn HSE Bypass 8 MHz
    MX_GPIO_Init()           → Bật clock GPIOG, cấu hình PG13 làm Push-Pull Output, mức Low
           │
           ▼
[Vòng lặp while(1)]
    Lấy tick hiện tại: now = HAL_GetTick()
    Kiểm tra: (now - t_last_blink >= 1000) ?
       ├── ĐÚNG:
       │    ├── t_last_blink = now
       │    └── HAL_GPIO_TogglePin(LD3_GREEN_GPIO_Port, LD3_GREEN_Pin)
       └── SAI:
            Tiếp tục vòng lặp (CPU rảnh, sẵn sàng làm việc khác)
```

### Where each fragment goes

| CubeMX marker | What you add |
|---|---|
| `/* USER CODE BEGIN PD */` | `#define BLINK_PERIOD_MS  1000` |
| `/* USER CODE BEGIN PV */` | `static uint32_t t_last_blink = 0;` |
| `/* USER CODE BEGIN 2 */` | `HAL_GPIO_WritePin(LD3_GREEN_GPIO_Port, LD3_GREEN_Pin, GPIO_PIN_RESET);` (Đảm bảo LED tắt ban đầu) |
| `/* USER CODE BEGIN WHILE */` | Vòng lặp kiểm tra thời gian không khóa CPU (`now - t_last_blink >= BLINK_PERIOD_MS`) |

### Key API calls and why

| Call | Why this variant |
|---|---|
| `HAL_GPIO_TogglePin(GPIOx, GPIO_Pin)` | Đảo trạng thái hiện tại của pin trong 1 lệnh duy nhất, không cần phải đọc trạng thái rồi dùng `if/else` |
| `HAL_GetTick()` | Trả về thời gian tính bằng mili-giây từ khi khởi động (32-bit integer). Cho phép viết logic non-blocking |
| `HAL_Delay(1000)` | *(Phương án phụ)* Dùng vòng lặp khóa CPU trong 1000 ms. Thích hợp cho demo siêu cơ bản nhưng không khuyên dùng cho dự án thực tế |

Mã nguồn tham chiếu đầy đủ được lưu tại `blink-led-main.c`.

---

## 8. Bring-up and verification

### Incremental stages

| Stage | What to build | What you should observe | If it fails, suspect |
|---|---|---|---|
| **1. Empty build & flash** | Biên dịch project rỗng sinh ra từ CubeMX và nhấn Debug | Trình gỡ lỗi nạp thành công, dừng tại dòng đầu tiên của hàm `main()` | Cáp USB cắm cổng CN1 (ST-LINK), driver ST-LINK, chưa gạt jumper CN4 |
| **2. Clock pass** | Cho chương trình chạy qua hàm `SystemClock_Config()` (nhấn F6 step over) | Chương trình chạy qua bình thường và đến `MX_GPIO_Init()` | Cấu hình HSE bị chọn nhầm thành Crystal thay vì BYPASS, khiến rơi vào `Error_Handler()` |
| **3. Manual GPIO toggle** | Đặt breakpoint ngay sau lệnh toggle hoặc chạy lệnh bật tắt | Đèn LED LD3 màu xanh lá bật sáng khi set High, tắt khi set Low | Nhầm nhãn chân (chọn PG14 thay vì PG13), GPIO clock chưa bật |
| **4. Full cycle test** | Nhấn F8 (Resume) cho chương trình chạy tự do | Đèn LED LD3 chớp tắt đều đặn: 1 giây sáng, 1 giây tắt | Kiểm tra giá trị hằng số `BLINK_PERIOD_MS` |

### Verification checklist

- [ ] LED LD3 (xanh lá cây) chớp tắt với chu kỳ chuẩn: 1s sáng, 1s tắt.
- [ ] Quan sát biến `now` và `t_last_blink` trong cửa sổ **Live Expressions** tăng dần đều mỗi 1000 ms.
- [ ] Không có hiện tượng đơ bo mạch, debugger duy trì kết nối ổn định.

### Failure decision tree for this task

```
LED LD3 không chớp tắt hoặc lỗi xem giá trị khi Debug
│
├─ Debugger không kết nối được với bo mạch
│    ├─ Cáp cắm vào cổng USB ST-LINK (cổng trên, Mini-USB CN1), không cắm vào Micro-USB (CN6)?
│    ├─ 2 jumper trên header CN4 có được cắm kín không? (Bắt buộc cắm để kết nối chip mục tiêu)
│    └─ Giữ nút RESET (B2 màu đen) trên bo mạch, bấm nút Debug trên IDE rồi thả nút RESET ra
│
├─ Chương trình bị dừng kẹt trong Error_Handler() hoặc chạy sai thời gian (3-4 giây mới chớp)
│    ├─ Khắc phục nhanh nhất: Đặt RCC → High Speed Clock (HSE) = "Disable", dùng xung nội HSI 16 MHz
│    └─ Nếu dùng HSE: Phải chọn "BYPASS Clock Source", kiểm tra bo mạch có nối cầu chì SB18 hay không
│
├─ Cửa sổ SFRs báo "Error reading value" khi click RD vào GPIOG > ODR
│    ├─ NGUYÊN NHÂN: Vi điều khiển đang CHẠY (RUNNING). Trình xem SFRs chỉ đọc được khi chip TẠM DỪNG (SUSPEND)
│    └─ KHẮC PHỤC: Nhấn nút Suspend (Pause màu vàng trên thanh công cụ) hoặc đặt Breakpoint rồi mới bấm RD
│
├─ Cửa sổ Live Expressions báo "Failed to evaluate expression" khi gõ GPIOG->ODR
│    ├─ NGUYÊN NHÂN: GDB không hiểu macro C (#define GPIOG ...). GPIOG không phải là biến trong RAM
│    └─ KHẮC PHỤC:
│         + Cách 1 (Trực tiếp): Gõ thẳng địa chỉ bộ nhớ `*(uint32_t*)0x40021814`, chuyển sang Binary để xem
│         + Cách 2 (Chuẩn thực tế): Gán vào một biến toàn cục C `volatile uint32_t debug_odr;` trong code
│
└─ Chương trình đang chạy trong while(1) nhưng LED không sáng
     ├─ Kiểm tra chân: Có phải PG13 không? (Chân PG14 là LED đỏ LD4, chân PDx thuộc bo F407 không phải bo F429)
     └─ Kiểm tra phần cứng LED hoặc jumper kết nối bo mạch
```

---

## 9. What to learn from this task

1. **Hiểu bản chất phần cứng từ sơ đồ nguyên lý (Schematic):** LED được nối từ chân MCU qua điện trở xuống GND nghĩa là **Active High** (mức 1 = sáng, mức 0 = tắt). Luôn tra schematic trước khi đoán logic kích hoạt.
2. **Đặc thù xung nhịp bo Discovery (HSE Bypass):** Xung dao động 8 MHz không đến từ thạch anh hàn riêng cho chip F429 mà được cấp từ chân MCO của vi điều khiển ST-LINK. Do đó RCC HSE bắt buộc phải chọn **BYPASS**.
3. **Tư duy lập trình Non-blocking:** Tập thói quen dùng `HAL_GetTick()` thay vì `HAL_Delay()`. Kỹ thuật này cho phép vi điều khiển làm nhiều việc cùng lúc (chớp nhiều LED ở các chu kỳ khác nhau, đọc nút bấm tức thời...) mà không bị nghẽn CPU.

---

## 10. Possible extensions

1. **Thêm LED đỏ LD4 (PG14) chớp so le:** Cho PG14 sáng khi PG13 tắt và ngược lại (anti-phase blink).
2. **Tương tác với nút nhấn USER B1 (PA0):** Mỗi lần nhấn nút USER (PA0, Active High), chu kỳ chớp LED đổi giữa 1000 ms và 200 ms.
3. **Nâng cấp lên Hardware Timer Interrupt:** Cấu hình Timer TIM3 tạo ngắt chu kỳ 1000 ms để đảo trạng thái LED trong hàm callback `HAL_TIM_PeriodElapsedCallback()`.
