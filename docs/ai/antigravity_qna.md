# Giải đáp: Năng lực VLM của Gemini và Quản lý môi trường Python trong Antigravity CLI

Dưới đây là phần phân tích chi tiết cho hai vấn đề bạn đang quan tâm trong quá trình vận hành Antigravity CLI.

---

## 1. Năng lực đọc PDF phức tạp của Gemini (Gemini 3.1 Pro / 3.8)

**Câu trả lời ngắn gọn:** Có, các model Gemini (bao gồm dòng Pro) **chính xác là các Vision Language Models (VLMs) được xây dựng đa phương thức từ trong lõi (natively multimodal)**. Chúng hoàn toàn có khả năng xử lý các file PDF kỹ thuật phức tạp của hãng STMicroelectronics.

**Phân tích chi tiết:**
Nhiều AI trước đây xử lý PDF bằng cách chạy một lớp OCR (nhận dạng ký tự quang học) tách biệt để "bốc" toàn bộ chữ ra thành một file text phẳng (flattened text), từ đó làm mất đi hoàn toàn bối cảnh về layout, hình ảnh, hay cột/bảng. Nhưng kiến trúc của Gemini lại khác:
1. **Đa phương thức từ gốc (Native Multimodality):** Khi đưa một file PDF (như *Datasheet* hay *Reference Manual* của hãng ST), Gemini "nhìn" file đó vừa dưới dạng văn bản (text tokens) vừa dưới dạng hình ảnh không gian (visual tokens).
2. **Khả năng xử lý bảng biểu và sơ đồ:** Trong tài liệu STM32, các sơ đồ khối (Block Diagram), bảng Alternate Function Mapping, hay biểu đồ Timing... đều là những dữ liệu không thể đọc nếu chỉ dùng text. Khả năng VLM cho phép Gemini nội suy các đường dẫn tín hiệu trong ảnh mạch hoặc đối chiếu chính xác hàng/cột của các thanh ghi (Registers) mà không bị lệch dòng, hay làm vỡ bố cục bảng.
3. **Cửa sổ ngữ cảnh khổng lồ (Massive Context Window):** Các file PDF của STM32 như RM0090 thường dài gần 2000 trang. Các phiên bản Gemini Pro sở hữu ngữ cảnh lên đến hàng triệu token, cho phép model tải và đối chiếu chéo toàn bộ dữ liệu văn bản lẫn hình ảnh kỹ thuật trong bộ nhớ mà không cần cắt xén quá nhiều.

*Tóm lại, bạn hoàn toàn có thể tin tưởng giao cho Gemini xử lý các trang tài liệu PDF kỹ thuật chứa sơ đồ khối hay bảng biểu phần cứng.*

---

## 2. Việc thực thi script Python và quản lý môi trường (Environment) trong Antigravity CLI

**Câu trả lời ngắn gọn:** Antigravity CLI chạy lệnh Shell/PowerShell trực tiếp trên máy host (Windows laptop). Nếu không cấu hình hoặc yêu cầu rõ ràng, nó sẽ dùng môi trường Python mặc định đang active ở terminal, **có nguy cơ cài thẳng vào System Environment của Windows**. Tuy nhiên, có những cơ chế thiết kế (như dùng `uv` hoặc tự động tạo `.venv`) để giải quyết vấn đề này triệt để.

**Phân tích chi tiết về luồng thực thi:**

1. **Cơ chế gọi lệnh (Command Execution):**
   Khi một Skill yêu cầu chạy lệnh bash/powershell, Antigravity CLI sẽ mở một subprocess (ví dụ PowerShell) ngay tại thư mục làm việc (workspace). Nếu trong script đó AI gọi lệnh `pip install thư_viện` (ví dụ `PyMuPDF`, `pdfplumber`), việc nó cài vào đâu phụ thuộc hoàn toàn vào **phiên terminal lúc bạn khởi chạy `agy`**.
   - Nếu lúc gõ lệnh mở `agy`, bạn chưa activate môi trường ảo nào, gói tin sẽ bị cài đè lên thư viện Python toàn cục (Global/System Environment) của Windows. Đây là điều tối kỵ vì sẽ gây rác hệ thống và dẫn đến xung đột phiên bản phần mềm.

2. **Cách quản lý đúng và an toàn trong Workspace:**
   Để Antigravity CLI cài đặt vào thư mục `.venv` an toàn ngay trong không gian dự án, có những phương pháp sau:
   - **Tích hợp `uv` (Khuyến nghị số 1, nhanh nhất):** Hệ sinh thái Antigravity đi kèm với skill quản lý package siêu tốc `uv`. Thay vì chạy `pip install` thuần, agent (hoặc định nghĩa trong Skill) có thể dùng `uv run script.py` hoặc `uv pip install`. Lệnh `uv` sẽ tự động tạo và quản lý thư viện ẩn ngay trong workspace hiện hành mà không chạm vào môi trường hệ thống.
   - **Chủ động tạo `.venv` bằng tay hoặc qua AI:** Bạn có thể thiết lập quy tắc (Rule) trong file `AGENTS.md` hoặc yêu cầu thẳng qua prompt: *"Trước khi chạy script python có thư viện lạ, hãy tạo môi trường ảo bằng lệnh `python -m venv .venv`, kích hoạt nó (ví dụ chạy `./.venv/Scripts/Activate.ps1`) rồi mới dùng `pip install`"*. Lúc đó các thư viện sẽ bị nhốt hoàn toàn trong folder `.venv` của dự án.

**Kết luận cho vấn đề thứ hai:**
Để an toàn nhất cho máy tính Windows, bạn nên gài một rule trong `AGENTS.md` hoặc quy định trong file định nghĩa của các Skill rằng: *"Khi cần chạy Python script có dependencies mới, bắt buộc phải dùng `uv run` hoặc tự động tạo `.venv` cục bộ, tuyệt đối không gọi trực tiếp `pip install` ở System level"*. Như vậy môi trường hệ thống của bạn sẽ luôn được bảo vệ hoàn hảo.
