🎙️ Thiết kế và triển khai máy thu – phát âm thanh dùng ESP32-C3
1. Giới thiệu

Trong bối cảnh các hệ thống nhúng ngày càng được ứng dụng rộng rãi, việc xây dựng một thiết bị thu âm và phát lại âm thanh độc lập có ý nghĩa thực tiễn cao trong học tập và nghiên cứu.
Dự án này tập trung vào việc thiết kế một máy ghi âm kỹ thuật số sử dụng ESP32-C3, cho phép người dùng ghi âm, lưu trữ, phát lại và quản lý các bản ghi âm trực tiếp trên thiết bị mà không cần kết nối máy tính.

Hệ thống khai thác giao tiếp I2S cho xử lý âm thanh, SD Card cho lưu trữ dữ liệu và OLED để hiển thị trạng thái hoạt động, tạo nên một thiết bị nhỏ gọn nhưng đầy đủ chức năng.

2. Mục tiêu của dự án

Thiết kế hệ thống thu và phát âm thanh số trên nền tảng ESP32-C3

Ghi âm từ microphone I2S và lưu dưới dạng file WAV chuẩn PCM

Phát lại âm thanh thông qua loa sử dụng giao tiếp I2S

Cho phép người dùng chuyển chế độ, chọn file và xóa file bằng nút nhấn

Hiển thị trực quan trạng thái hoạt động trên màn hình OLED

3. Chức năng chính

🎤 Ghi âm âm thanh

Thu tín hiệu từ microphone I2S

Lưu dữ liệu trực tiếp vào thẻ SD

💾 Lưu trữ file WAV

Định dạng WAV 16-bit, 16 kHz, mono

Tự động tạo và cập nhật header WAV

🔊 Phát lại âm thanh

Phát file WAV thông qua loa I2S

📂 Quản lý bản ghi

Chuyển file (Next)

Xóa file đang chọn và tự động sắp xếp lại

🖥️ Hiển thị OLED

Trạng thái hệ thống

Chế độ hoạt động

File đang xử lý

4. Phần cứng sử dụng

ESP32-C3 – vi điều khiển trung tâm

Microphone I2S – thu âm thanh

Loa / DAC I2S – phát âm thanh

Thẻ nhớ SD – lưu trữ dữ liệu

OLED SSD1306 (128×64) – hiển thị

Các nút nhấn vật lý – điều khiển thiết bị

5. Sơ đồ chân kết nối
5.1 Giao tiếp I2S
Tín hiệu	GPIO
BCLK	GPIO 5
WS (LRCK)	GPIO 6
SD (MIC)	GPIO 4
DIN (SPK)	GPIO 7
5.2 Thẻ nhớ SD (SPI)
Tín hiệu	GPIO
CS	GPIO 8
SCK	GPIO 1
MISO	GPIO 3
MOSI	GPIO 2
5.3 OLED (I2C)
Tín hiệu	GPIO
SDA	GPIO 19
SCL	GPIO 18
5.4 Nút nhấn
Nút	GPIO	Chức năng
BTN_REC	9	Ghi / Phát
BTN_MODE	10	Chuyển chế độ
BTN_NEXT	21	Chuyển file
BTN_DEL	20	Xóa file
6. Nguyên lý hoạt động của hệ thống

Hệ thống hoạt động theo hai chế độ chính:

6.1 Chế độ RECORD

Là chế độ mặc định khi khởi động

Người dùng nhấn BTN_REC:

Lần 1: bắt đầu ghi âm

Lần 2: kết thúc ghi và lưu file

File được lưu với tên tăng dần:

rec001.wav
rec002.wav
rec003.wav

6.2 Chế độ PLAY

Chuyển sang bằng BTN_MODE

Người dùng có thể:

Chọn file bằng BTN_NEXT

Phát file bằng BTN_REC

Xóa file bằng BTN_DEL

7. Định dạng và xử lý âm thanh

Chuẩn file: WAV (PCM)

Sample rate: 16,000 Hz

Độ phân giải: 16 bit

Số kênh: Mono

Header WAV được tạo và cập nhật thủ công nhằm đảm bảo file có thể phát chính xác trên các thiết bị tiêu chuẩn như máy tính hoặc điện thoại.

8. Giao diện hiển thị

Màn hình OLED hiển thị các trạng thái như:

MODE: RECORD / MODE: PLAY

RECORDING

PLAYING

SAVED

DELETED

Điều này giúp người dùng dễ dàng theo dõi hoạt động của hệ thống trong quá trình sử dụng.

9. Thư viện phần mềm

Các thư viện sử dụng trong dự án:

Adafruit SSD1306

Adafruit GFX Library

SD

SPI

Wire

10. Kết luận

Dự án đã xây dựng thành công một máy ghi âm kỹ thuật số độc lập sử dụng ESP32-C3, đáp ứng đầy đủ các chức năng thu, lưu trữ, phát lại và quản lý âm thanh.
Hệ thống có cấu trúc rõ ràng, dễ mở rộng và phù hợp cho các bài tập lớn, đồ án môn học hoặc nghiên cứu về xử lý âm thanh trên vi điều khiển.

11. Hướng phát triển

Hiển thị thời gian ghi âm

Ghi âm stereo

Gửi file qua WiFi / Bluetooth

Xây dựng menu điều khiển nâng cao

👤 Tác giả

Thuận Đinh
Dự án học tập và nghiên cứu hệ thống nhúng – ESP32-C3
