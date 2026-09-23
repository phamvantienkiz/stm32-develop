/**
 * ============================================================================
 *  blink-led — Điều khiển đèn LED LD3 (màu xanh lá) nhấp nháy 1s sáng, 1s tắt
 * ============================================================================
 *
 *  Board  : STM32F429I-DISC1 (MB1075)     MCU : STM32F429ZIT6
 *  Design : xem blink-led-design-dossier.md (sơ đồ chân §3, CubeMX checklist §6)
 *
 *  HƯỚNG DẪN SỬ DỤNG FILE MÃ NGUỒN NÀY:
 *  ------------------------------------
 *  Đây là file mã nguồn tham chiếu chuẩn (reference code). Bạn không nên ghi đè
 *  toàn bộ file main.c do CubeMX sinh ra. Thay vào đó:
 *  1. Cấu hình CubeMX theo checklist từng bước trong file blink-led-design-dossier.md (§6).
 *  2. Nhấn Generate Code trong STM32CubeIDE.
 *  3. Sao chép từng đoạn mã bên dưới vào đúng các khối chú thích `/* USER CODE BEGIN ... */`
 *     tương ứng trong file Core/Src/main.c của dự án bạn vừa tạo.
 *
 *  CÁC GIẢ ĐỊNH PHẦN CỨNG ĐÃ ĐƯỢC XÁC MINH:
 *  ----------------------------------------
 *  - LD3 (Green) kết nối với chân PG13 [UM1670 §7.5 p.20; MB1075 Sheet 6].
 *  - Cực tính: Active High (PG13 = High -> LED sáng; PG13 = Low -> LED tắt) [MB1075 Sheet 6].
 *  - Nguồn xung nhịp: HSE 8 MHz Bypass từ ST-LINK MCO [UM1670 §7.12.1 p.22].
 * ============================================================================
 **/

/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PTD */
/* Không có cấu trúc dữ liệu tùy biến cho bài tập cơ bản này */
/* USER CODE END PTD */

/* USER CODE BEGIN PD */
/* Thời gian sáng / tắt của đèn LED (mili-giây) theo yêu cầu đề bài: sáng 1s, tắt 1s */
#define BLINK_PERIOD_MS    1000U
/* USER CODE END PD */

/* USER CODE BEGIN PV */
/* Lưu mốc thời gian tick lần đảo trạng thái LED gần nhất.
 * Vì biến chỉ sử dụng bên trong vòng lặp main() (không chia sẻ với ngắt ISR)
 * nên khai báo static là an toàn và không cần từ khóa 'volatile'. */
static uint32_t t_last_blink = 0;
/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Khai báo nguyên mẫu hàm cục bộ nếu có */
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

int main(void)
{
    /* Khởi tạo phần cứng cơ bản: Flash prefetch, bộ đệm lệnh và SysTick (mặc định 1 ms) */
    HAL_Init();

    /* Cấu hình cây xung nhịp hệ thống lên 180 MHz từ nguồn HSE 8 MHz Bypass (xem dossier §5)
     * LƯU Ý: Nếu cấu hình nhầm HSE thành Crystal, hàm này sẽ trả về HAL_ERROR
     * và nhảy vào Error_Handler() khiến bo mạch bị treo! */
    SystemClock_Config();

    /* Khởi tạo các ngoại vi được sinh bởi STM32CubeMX */
    MX_GPIO_Init();

    /* USER CODE BEGIN 2 */
    /* Đảm bảo ban đầu LED ở trạng thái TẮT (mức LOW vì mạch Active High) */
    HAL_GPIO_WritePin(LD3_GREEN_GPIO_Port, LD3_GREEN_Pin, GPIO_PIN_RESET);

    /* Khởi tạo mốc thời gian ban đầu bằng tick hiện tại của SysTick */
    t_last_blink = HAL_GetTick();
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* --- KỸ THUẬT 1 (KHUYÊN DÙNG): Non-blocking thời gian với HAL_GetTick() ---
         * 
         * Quy tắc an toàn số học: LUÔN dùng phép trừ số nguyên không dấu `(now - t_last) >= PERIOD`.
         * Cách viết này hoàn toàn an toàn khi biến uint32_t của SysTick bị tràn số sau ~49.7 ngày.
         * Tuyệt đối KHÔNG viết `now >= (t_last + PERIOD)` vì sẽ sai nghiêm trọng khi tràn số.
         * 
         * Lợi ích: Vòng lặp while(1) không bị treo, CPU hoàn toàn rảnh rỗi để có thể
         * làm thêm tác vụ khác như đọc nút nhấn, xử lý truyền thông... */
        if ((now - t_last_blink) >= BLINK_PERIOD_MS)
        {
            t_last_blink = now;
            
            /* Đảo trạng thái chân PG13 (nếu đang sáng -> tắt, nếu đang tắt -> sáng) */
            HAL_GPIO_TogglePin(LD3_GREEN_GPIO_Port, LD3_GREEN_Pin);
        }

        /* --- PHƯƠNG ÁN 2 (THAY THẾ ĐƠN GIẢN): Dùng HAL_Delay() ---
         * Nếu bạn chỉ muốn làm quen dạng "Hello World" đơn giản nhất:
         * 
         * HAL_GPIO_TogglePin(LD3_GREEN_GPIO_Port, LD3_GREEN_Pin);
         * HAL_Delay(1000);
         * 
         * Hạn chế: HAL_Delay() bắt CPU phải chạy vòng lặp chờ rỗng (blocking), 
         * không thể làm việc gì khác trong suốt 1 giây này. */
    }
    /* USER CODE END WHILE */
}

/* USER CODE BEGIN 4 */
/* Không sử dụng ngắt ngoại vi nào trong bài tập này */
/* USER CODE END 4 */

/**
  * @brief  Hàm xử lý lỗi hệ thống khi có lỗi cấu hình xung nhịp hay ngoại vi.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();

    /* USER CODE BEGIN Error_Handler_Debug */
    /* Mặc định CubeMX để Error_Handler() là vòng lặp while(1) vô tận, không có bất kỳ
     * dấu hiệu trực quan nào, khiến lập trình viên tưởng bo mạch bị hỏng/chết nguồn.
     * Ở đây chúng ta bật liên tục LED đỏ LD4 (PG14) trực tiếp qua thanh ghi BSRR
     * để báo hiệu rõ ràng cho người dùng biết hệ thống đã gặp lỗi. */
    GPIOG->BSRR = GPIO_PIN_14; /* Set bit 14 để bật LED đỏ cảnh báo lỗi */
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}
