# Giải thích Lý thuyết: Hardware Timer và Check Hardware

Tài liệu này giải đáp các câu hỏi kỹ thuật về cấu trúc Timer trong STM32 và cơ chế kiểm tra phần cứng (Hardware Check) như đã nêu trong yêu cầu đầu vào.

## 1. Cơ chế hoạt động của Hardware Timer

Timer trong STM32 không chỉ đơn thuần là bộ đếm giờ (delay), mà nó là một cỗ máy trạng thái phần cứng (Hardware State Machine) hoạt động hoàn toàn độc lập với CPU lõi ARM.

*   **Clock nội (Internal Clock):** Là nhịp đập từ hệ thống Clock Tree (ở đây là HSI 16 MHz thông qua APB1 bus). Tốc độ này quá nhanh để đếm thời gian cho con người. `[RM0090 §17.3.1]`
*   **Thanh ghi chia tần (Prescaler - PSC):** Hoạt động như hộp số xe máy, nó "giảm tốc" nhịp đập của Clock nội. Với PSC = 15999, cứ 16000 nhịp Clock nội mới tạo ra 1 nhịp (tick) cho bộ đếm Timer. Lõi đếm (CNT) giờ đây sẽ nhảy số mỗi 1 mili-giây. `[RM0090 §17.3.2]`
*   **Thanh ghi chu kỳ (Auto-Reload Register - ARR):** Là cái vạch đích. Bộ đếm `CNT` sẽ tăng dần từ 0, khi chạm tới giá trị của `ARR` (bằng 999), nó sẽ tự động reset về 0 ở nhịp tiếp theo (sinh ra 1 chu kỳ đúng 1000 nhịp = 1 giây). `[RM0090 §17.4.8]`
*   **Cờ ngắt tràn (Update Interrupt Flag - UIF):** Ngay khoảnh khắc `CNT` chạm `ARR` và reset về 0, phần cứng sẽ tự động dựng cờ UIF lên bằng 1. Nếu ngắt được kích hoạt (NVIC), cờ UIF này sẽ phát một luồng điện "đánh thức" CPU, buộc CPU tạm dừng việc đang làm để chạy vào hàm `HAL_TIM_PeriodElapsedCallback()`.

## 2. Weak Function và `HAL_TIM_Base_Start_IT()`

*   **`HAL_TIM_Base_Start_IT(&htim3)`:** Hàm này đóng vai trò "bật công tắc". Nó cho phép clock bắt đầu chảy vào thanh ghi `CNT` của TIM3, đồng thời bật cho phép ngắt (Interrupt) để TIM3 có quyền gõ cửa CPU khi tràn `ARR`.
*   **Cơ chế Weak Function (Ghi đè hàm yếu):** 
    * Thư viện HAL của ST định nghĩa sẵn một hàm `__weak void HAL_TIM_PeriodElapsedCallback()`. Chữ `__weak` có nghĩa là: *"Nếu người dùng (programmer) không viết hàm này, thì khi ngắt xảy ra, hệ thống sẽ chạy hàm trống rỗng này để khỏi bị lỗi chương trình"*.
    * Khi chúng ta viết lại hàm này trong `main.c` (bỏ đi chữ `__weak`), trình biên dịch C sẽ lập tức loại bỏ hàm cũ của ST và trỏ luồng thực thi vào hàm do chúng ta tự viết. Đây là một mô hình lập trình kế thừa cực kỳ thông minh của C.

## 3. Khái niệm "Check Hardware bằng Timer"

Trong môi trường công nghiệp, bạn không thể tin tưởng 100% vào việc CPU đang chạy đúng. "Check Hardware" (Sanity Check) ở bài tập này thể hiện ở 2 mức độ:

1.  **Nhịp sinh tồn (System Heartbeat):** Bằng việc thiết lập một ngắt chu kỳ cứng (Hardware Timer) in ra một chuỗi "Uptime: X s" qua UART mỗi giây, hệ thống liên tục chứng minh được 3 điều:
    *   Lõi CPU không bị kẹt trong một vòng lặp vô hạn `while(1)` cứng nào đó.
    *   Hệ thống ngắt (NVIC) vẫn đang phản hồi tốt.
    *   Đường truyền UART TX vẫn thông suốt.
    *(Nếu máy tính nhận không thấy bản tin này trong vòng 2 giây, phần mềm giám sát trên PC biết ngay là MCU đã bị treo hoặc đứt cáp).*

2.  **Kiểm chứng Clock Tree (Timing Validation):** Lệnh `CHECK_HW` chúng ta sắp viết sẽ đọc trực tiếp giá trị thanh ghi `TIM3->CNT`, `TIM3->PSC`, `TIM3->ARR`. Nếu cấu hình xung nhịp 16MHz bị sai (ví dụ set nhầm thạch anh ngoài), số giây in ra trên Terminal sẽ chạy nhanh hơn hoặc chậm hơn rõ rệt so với đồng hồ đeo tay của bạn. Nếu nó trùng khớp đúng 1 giây thực tế = 1 giây Uptime, ta khẳng định Clock Tree và phần cứng dao động hoàn toàn khỏe mạnh.

## 4. Xử lý tràn số biến `uptime_seconds`

Bạn có thắc mắc: *"Biến đếm giây tăng liên tục sẽ bị tràn số (Integer Overflow) thế nào?"*
* Biến chúng ta dùng là `uint32_t`. Giá trị tối đa của nó là $2^{32} - 1 = 4,294,967,295$.
* Với tốc độ tăng 1 đơn vị mỗi giây (1 Hz), thời gian để tràn số là:
  $4,294,967,295 \text{ giây} \approx 136 \text{ năm}$.
* Trong thực tế các hệ thống nhúng thông thường (như router, máy CNC), tuổi thọ thiết bị hoặc thời gian giữa các lần bảo trì/khởi động lại ngắn hơn 136 năm rất nhiều, do đó ta không cần code thêm logic xử lý tràn (rollover) cho biến này.

## 5. Sự ưu tiên ngắt (Preemption Priority)

Khi 2 ngắt cùng xảy ra một lúc, hoặc ngắt này đang xử lý thì ngắt kia tới, bộ NVIC sẽ ưu tiên dựa vào Priority:
* **UART:** Nhận ký tự ở tốc độ `115200 bps`, tức là chỉ có khoảng ~86 micro-giây cho mỗi byte bay vào thanh ghi dịch (Shift Register). Nếu CPU không ra lấy kịp, byte tiếp theo sẽ đè lên byte cũ (gây lỗi Overrun Error - ORE). Do đó **UART mang tính chất Time-Critical (cấp bách).**
* **TIM3:** Đếm giây 1Hz. Ngắt chỉ xảy ra 1 giây 1 lần, và việc in ra chữ Heartbeat có thể trễ vài chục mili-giây mà chẳng ảnh hưởng gì đến số đếm nội bộ của Timer (bởi phần cứng đếm độc lập với CPU).
👉 **Kết luận:** Luôn phải đặt Priority của ngắt UART nhỏ hơn (ưu tiên cao hơn) ngắt Timer. Trong cấu hình CubeMX chúng ta đã để `UART = 1` và `TIM3 = 2`.
