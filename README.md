# Smart Window IoT System

Hệ thống cửa sổ thông minh (Smart Window) dựa trên vi điều khiển ESP32, cho phép tự động đóng/mở cửa dựa trên điều kiện thời tiết thực tế (Mưa, tia UV, cường độ ánh sáng) và lịch trình thời gian thực. Hệ thống có thể được giám sát và điều khiển từ xa thông qua nền tảng đám mây **Blynk IoT**.

---

## Tính năng nổi bật

* **Chế độ Tự động (Auto Mode):** * **Bảo vệ thời tiết (Ưu tiên cao nhất):** Tự động đóng cửa ngay lập tức khi trời mưa, nắng gắt (> 50,000 Lux), hoặc chỉ số tia cực tím đạt ngưỡng nguy hiểm (UV Index >= 6).
    * **Lịch trình thông minh (Timer):** Tự động mở cửa đón gió vào ban ngày và đóng cửa vào ban đêm theo khung giờ được cài đặt trước qua RTC.
* **Chế độ Thủ công (Manual Mode):** Cho phép người dùng ra lệnh đóng/mở cửa trực tiếp trên điện thoại hoặc thông qua nút nhấn vật lý tại chỗ.
* **Đồng bộ thời gian chuẩn xác:** Tích hợp module RTC DS3231 kết hợp với giao thức NTP (Network Time Protocol) để tự động cập nhật giờ chuẩn quốc tế mỗi giờ một lần.
* **Giao tiếp Multi-I2C:** Khai báo và sử dụng đồng thời 2 tuyến bus I2C vật lý độc lập trên ESP32 để tối ưu hóa việc đọc dữ liệu từ RTC và cảm biến ánh sáng.
* **Chống sụt áp (Brownout Bypass):** Can thiệp thanh ghi vô hiệu hóa mạch phát hiện sụt áp, giúp ESP32 không bị reset khi động cơ Servo kéo dòng khởi động.
* **Tự phục hồi kết nối:** Tự động dò tìm và kết nối lại với WiFi/Blynk khi rớt mạng, đồng thời khóa các biến giả lập để đảm bảo an toàn.

---

## Yêu cầu Phần cứng & Sơ đồ chân (Pinout)

Hệ thống sử dụng vi điều khiển trung tâm **ESP32 DevKit V1**. Dưới đây là bảng đấu nối thiết bị ngoại vi:

| Khối chức năng | Linh kiện | Chân ESP32 | Ghi chú / Giao thức |
| :--- | :--- | :--- | :--- |
| **Chấp hành** | Động cơ Servo (MG90S) | `GPIO 27` | Điều khiển góc quay qua PWM |
| **Thời tiết** | Cảm biến Mưa | `GPIO 33` | Tín hiệu Analog (ADC) |
| **Thời tiết** | Cảm biến UV (GUVA-S12SD)| `GPIO 32` | Tín hiệu Analog (ADC) |
| **Tương tác** | Nút nhấn vật lý | `GPIO 15` | Digital Input (Nên dùng trở kéo lên) |
| **Thời gian** | Module RTC (DS3231) | `SDA: 22`, `SCL: 23` | Chuẩn I2C (Bus 0) - Cố định địa chỉ `0x68` |
| **Ánh sáng** | Cảm biến (GY-30/BH1750)| `SDA: 5`, `SCL: 18` | Chuẩn I2C (Bus 1) - Cố định địa chỉ `0x23` |

---

## Thư viện cài đặt (Dependencies)

Để biên dịch thành công mã nguồn, bạn cần cài đặt các thư viện sau thông qua *Library Manager* trong Arduino IDE:

1.  `Blynk` (by Volodymyr Shymanskyy)
2.  `ESP32Servo` (by Kevin Harrington, John K. Bennett)
3.  `RTClib` (by Adafruit)
4.  `BH1750` (by Christopher Laws)

*(Các thư viện `Wire.h`, `WiFi.h`, `time.h`, `soc/soc.h` đã được tích hợp sẵn trong bộ core ESP32).*

---

## Cấu hình Blynk (Virtual Pins Mapping)

Hệ thống sử dụng các Virtual Pins sau để truyền nhận dữ liệu với ứng dụng Blynk:

| Virtual Pin | Chức năng trên Dashboard | Kiểu Dữ liệu | Chiều dữ liệu |
| :--- | :--- | :--- | :--- |
| **V1** | Cảnh báo trạng thái mưa | Integer (0/1) | ESP32 -> Blynk |
| **V2** | Cường độ ánh sáng (Lux) | Float | ESP32 -> Blynk |
| **V3** | Chỉ số bức xạ UV (UV Index) | Float | ESP32 -> Blynk |
| **V4** | Công tắc Đóng/Mở cửa (Manual) | Boolean | Blynk <-> ESP32 |
| **V5** | Giờ đóng cửa (Lịch trình) | Integer | Blynk -> ESP32 |
| **V6** | Phút đóng cửa (Lịch trình) | Integer | Blynk -> ESP32 |
| **V7** | Phản hồi trạng thái Cửa (1: Đóng, 0: Mở)| Boolean | ESP32 -> Blynk |
| **V8** | Giờ mở cửa (Lịch trình) | Integer | Blynk -> ESP32 |
| **V9** | Phút mở cửa (Lịch trình) | Integer | Blynk -> ESP32 |
| **V10** | Giờ hệ thống thực tế (RTC) | Integer | ESP32 -> Blynk |
| **V11** | Công tắc Bật/Tắt chế độ Timer | Boolean | Blynk <-> ESP32 |
| **V12** | Chuyển đổi Chế độ (0: Auto, 1: Manual) | Integer | Blynk <-> ESP32 |
| **V13** | Phút hệ thống thực tế (RTC) | Integer | ESP32 -> Blynk |
| **V14** | Thiết lập ngưỡng UV đóng cửa (Mặc định: 6)| Integer | Blynk -> ESP32 |
| **V15** | Thiết lập độ nhạy mưa (Mặc định: 1500) | Integer | Blynk -> ESP32 |
| **V16** | Bật/Tắt giả lập lỗi tia UV cao | Boolean | Blynk -> ESP32 |
| **V17** | Bật/Tắt giả lập lỗi Ánh sáng mạnh | Boolean | Blynk -> ESP32 |

---

## Hướng dẫn sử dụng nút nhấn vật lý (Non-blocking Button Logic)

Hệ thống tối ưu hóa 1 nút nhấn vật lý duy nhất để thực thi 3 tác vụ khác nhau, hoạt động dựa trên thời gian giữ phím (Holding Time):

* **Nhấn và nhả nhanh (< 5 giây):** Đảo trạng thái Đóng/Mở cửa. *(Lưu ý: Chỉ có tác dụng khi hệ thống đang ở Chế độ Thủ công `Mode 1`).*
* **Giữ phím từ 5 giây đến 10 giây:** Bật / Tắt tính năng hẹn giờ (Timer) ở Chế độ Tự động.
* **Giữ phím trên 10 giây:** Chuyển đổi qua lại giữa **Chế độ Tự động** (Auto Mode) và **Chế độ Thủ công** (Manual Mode).

---

## Hướng dẫn Cài đặt

1.  **Thiết lập phần cứng:** Đấu nối dây đúng theo bảng Pinout. Đảm bảo nguồn cấp 5V đủ dòng (khuyến nghị >= 2A) do Servo MG90S khi kéo tải tiêu thụ dòng khá lớn.
2.  **Cấu hình phần mềm:**
    * Thay thế các thông số `BLYNK_TEMPLATE_ID`, `BLYNK_TEMPLATE_NAME`, và `BLYNK_AUTH_TOKEN` bằng thông tin từ dự án Blynk của bạn.
    * Cập nhật `ssid` (Tên WiFi) và `pass` (Mật khẩu WiFi) cho phù hợp với mạng nội bộ.
3.  **Biên dịch & Nạp Code:** Kết nối mạch ESP32 với máy tính, chọn đúng Board `DOIT ESP32 DEVKIT V1` và nạp mã nguồn. Mở Serial Monitor ở tốc độ Baud `9600` để theo dõi quá trình khởi tạo tín hiệu I2C và đồng bộ thời gian NTP.