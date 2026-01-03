# Auto Navigator - Hướng dẫn sử dụng

Auto Navigator cho phép bạn tự động điều hướng bằng cách truyền các lệnh từ file hoặc serial.

## Cách sử dụng

### 1. Load từ file

Tạo file trong SPIFFS (ví dụ: `/data/auto_nav_example.txt`) với các lệnh, mỗi lệnh một dòng:

```
up
down
select
delay:1000
right
select
exit
```

Sau đó gửi qua Serial:
```
auto:load:/data/auto_nav_example.txt
auto:start
```

### 2. Load từ string (trực tiếp qua Serial)

Gửi qua Serial:
```
auto:exec:up,down,select,delay:1000,right,select,exit
auto:start
```

### 3. Các lệnh điều hướng

- `up` hoặc `u` - Đi lên
- `down` hoặc `d` - Đi xuống
- `left` hoặc `l` - Đi trái
- `right` hoặc `r` - Đi phải
- `select` hoặc `s` hoặc `enter` hoặc `e` - Chọn
- `exit` hoặc `x` hoặc `back` hoặc `b` - Thoát
- `delay:1000` - Delay 1000ms (có thể dùng số bất kỳ)
- `wait` - Chờ (chưa implement đầy đủ)

### 4. Các lệnh điều khiển Auto Navigator

- `auto:load:/path/to/file.txt` - Load lệnh từ file
- `auto:exec:command1,command2,command3` - Load lệnh từ string
- `auto:start` - Bắt đầu tự động điều hướng
- `auto:stop` - Dừng tự động điều hướng
- `auto:run` - Chạy tất cả lệnh ngay lập tức (không chờ delay)
- `auto:clear` - Xóa hàng đợi lệnh
- `auto:status` - Hiển thị trạng thái hiện tại

## Ví dụ sử dụng

### Ví dụ 1: Điều hướng WiFi

File: `/data/wifi_nav.txt`
```
down
down
select
delay:2000
select
```

Gửi qua Serial:
```
auto:load:/data/wifi_nav.txt
auto:start
```

### Ví dụ 2: Điều hướng Login

File: `/data/login_nav.txt`
```
down
select
delay:1000
right
right
right
select
delay:500
right
right
select
```

Gửi qua Serial:
```
auto:load:/data/login_nav.txt
auto:start
```

### Ví dụ 3: Điều hướng nhanh (không delay)

Gửi qua Serial:
```
auto:exec:up,down,select,exit
auto:run
```

## Lưu ý

1. **Delay mặc định**: 200ms giữa các lệnh (có thể thay đổi bằng code)
2. **Format file**: Mỗi lệnh một dòng, có thể dùng dấu phẩy hoặc khoảng trắng để phân cách
3. **Case insensitive**: Tất cả lệnh không phân biệt hoa/thường
4. **Auto execution**: Khi `auto:start`, các lệnh sẽ tự động chạy trong `loop()` với delay giữa các lệnh

## Tích hợp vào code

Auto Navigator đã được tích hợp vào `main.cpp`:
- Tự động khởi tạo trong `setup()`
- Tự động xử lý serial input trong `handleSerialNavigation()`
- Tự động execute commands trong `loop()` khi enabled

Không cần thêm code, chỉ cần gửi lệnh qua Serial Monitor!

