# Firefly RPC

Firefly RPC là chương trình C++ hiển thị Discord Rich Presence. Chương trình kết nối Discord qua IPC, giữ thời gian bắt đầu, hiển thị nút GitHub và tự thử kết nối lại sau mỗi 30 giây nếu Discord chưa sẵn sàng hoặc bị ngắt.

## Yêu cầu

- Cài và đăng nhập ứng dụng Discord bản desktop. Discord trên trình duyệt không cung cấp IPC cho Firefly.
- Cần có trình biên dịch C++17 để biên dịch từ mã nguồn. Firefly không cần thư viện C++ bên ngoài.
- Giữ thư mục dự án ở vị trí cố định sau khi thiết lập tự khởi chạy.

## Windows

### 1. Cài trình biên dịch

Chọn một trong hai cách:

- Cài MinGW-w64 và đảm bảo lệnh `g++` dùng được trong terminal.
- Cài Visual Studio hoặc Visual Studio Build Tools với workload **Desktop development with C++**, sau đó mở **Developer Command Prompt**.

### 2. Biên dịch

Mở terminal tại thư mục chứa `firefly.cpp`.

Với MinGW-w64:

```bat
g++ -std=c++17 -O2 firefly.cpp -o firefly.exe
```

Với MSVC Developer Command Prompt:

```bat
cl /std:c++17 /O2 /EHsc firefly.cpp
```

### 3. Chạy

Mở Discord và đăng nhập, sau đó chạy trong terminal:

```bat
firefly.exe
```

Có thể chạy bằng cách nhấp đúp `firefly.exe`. Khi chạy trong terminal, nhấn `Ctrl+C` để dừng.

### Tự chạy khi đăng nhập Windows

1. Nhấn `Win+R`, nhập `shell:startup`, rồi nhấn Enter.
2. Tạo shortcut tới `firefly.exe` trong thư mục Startup vừa mở.
3. Nếu muốn hạn chế cửa sổ console, tạo shortcut tới `firefly.exe` và chọn **Run: Minimized** trong thuộc tính shortcut.

## Ubuntu / Linux

### 1. Cài công cụ biên dịch

Trên Ubuntu hoặc Debian:

```bash
sudo apt update
sudo apt install g++
```

### 2. Biên dịch

Mở terminal tại thư mục dự án:

```bash
g++ -std=c++17 -O2 firefly.cpp -o firefly
```

### 3. Chạy

Mở Discord bản desktop và đăng nhập, sau đó chạy:

```bash
./firefly
```

Nhấn `Ctrl+C` để dừng chương trình.

### Tự chạy khi đăng nhập Ubuntu

Trong thư mục dự án, chạy script cài đặt:

```bash
./install-autostart.sh
```

Script sẽ biên dịch chương trình, cài systemd user service và bật service. Firefly sẽ tự chạy khi bạn đăng nhập Ubuntu và được khởi động lại nếu thoát. Nếu chuyển thư mục dự án sang vị trí khác, hãy chạy lại script.

Các lệnh quản lý service:

```bash
systemctl --user status firefly.service
systemctl --user restart firefly.service
systemctl --user disable --now firefly.service
```

Xem nhật ký:

```bash
journalctl --user -u firefly.service -f
```

## Xử lý lỗi

- **`RPC disconnected or unavailable`**: mở Discord desktop, chờ đăng nhập xong rồi chạy lại Firefly. Firefly cũng tự thử kết nối lại sau 30 giây.
- **Không tìm thấy `g++`**: cài trình biên dịch theo hướng dẫn của hệ điều hành.
- **Service có trạng thái `bad-setting` trên Ubuntu**: chạy lại `./install-autostart.sh` để cập nhật unit file, sau đó kiểm tra bằng `systemctl --user status firefly.service`.

## Tệp chính

- `firefly.cpp`: mã nguồn C++.
- `install-autostart.sh`: cài tự khởi chạy cho Ubuntu/Linux.
- `test.js`: phiên bản Node.js ban đầu.
