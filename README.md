# Ứng dụng quản lý người dùng và nhóm trên Linux

**Ba trang dữ liệu riêng:** Người dùng, Nhóm và Nhật ký được chuyển bằng thanh điều hướng bên trái. Nút Danh sách nhóm cũng mở trang Nhóm, không hiện thông báo văn bản nữa. Trang Nhóm có bảng tên nhóm/GID/số thành viên/nhóm chính/nhóm phụ; nhấp đúp một hàng mở bảng thành viên (tài khoản, UID, vai trò). Trang Nhật ký đọc tối đa 2.000 bản ghi gần nhất từ tệp nhật ký, mới nhất trước, với cột thời gian/kết quả/nội dung. Mỗi trang bảng có tìm kiếm và Làm mới. Nhật ký nhỏ dưới trang Người dùng chỉ là phần xem nhanh, trang Nhật ký là bảng đầy đủ.

**Giao diện cập nhật theo TCP_Socket:** bảng màu lấy từ `ServerGUI.java` và `UIUtils.java` của dự án mẫu: nền `#171d27`, thanh bên `#1b222d`, bề mặt `#1f2733`, viền `#374354`, màu nhấn `#7797c2`, màu xanh trạng thái `#7db197`. Bảng, nút, hộp thoại và nhật ký dùng tông tối đồng bộ. Giữ nguyên nghiệp vụ và lệnh quản trị. Tên hiển thị là “Ứng dụng quản lý người dùng và nhóm trên Linux”, không còn chữ Ubuntu trên giao diện.

**Cập nhật từ bản cũ:** đóng ứng dụng đang chạy, tải lại ZIP, giải nén vào thư mục mới. Mở Terminal ở thư mục có `CMakeLists.txt`, chạy `cmake -S . -B build`, `cmake --build build -j2`, rồi `sudo -H ./build/linux-user-manager`. Không cần cài lại các gói nếu đã cài trước đó. Tài khoản thật và nhật ký thuộc hệ điều hành vẫn còn.

**Đề tài chốt:** Xây dựng ứng dụng quản lý người dùng và nhóm trên Linux bằng C++ và Qt.

**English:** Development of a User and Group Management Application on Linux Using C++ and Qt.

Đây là bản chuyển sang C++17 và Qt Widgets 5, thay cho toàn bộ ứng dụng Python trước đó. Không cần Python, pip, Tkinter, cơ sở dữ liệu hay script Bash riêng. Dữ liệu thật đọc từ `/etc/passwd`, `/etc/group`, `/etc/shadow`; thao tác hệ thống qua lệnh Ubuntu bằng `QProcess`, không thông qua shell.

## 1. Cài và chạy trên Ubuntu 20.04

Tải `linux-user-manager-qt.zip` trong Firefox của Ubuntu, giải nén. Mở thư mục `linux-user-manager-qt`, nhấp phải chỗ trống → Open in Terminal.

```bash
sudo apt update
sudo apt install build-essential cmake qtbase5-dev xterm
cmake -S . -B build
cmake --build build -j2
cmake --build build --target test
sudo -H ./build/linux-user-manager
```

Để xem chi tiết kết quả kiểm thử, chạy:

```bash
cd build
ctest --output-on-failure
cd ..
```

Khi hỏi mật khẩu sudo, tự nhập mật khẩu Ubuntu rồi Enter. Terminal không hiện ký tự khi nhập. Không gửi mật khẩu vào chat. Lần sau chỉ cần mở Terminal ở thư mục dự án và chạy `sudo -H ./build/linux-user-manager`.

Nếu sudo báo không kết nối được màn hình X11/Wayland, giữ nguyên lỗi để kiểm tra cấu hình desktop. Không dùng `xhost +` để tắt bảo vệ màn hình. Ứng dụng này cần desktop đồ họa Ubuntu và quyền root; không chạy trên Windows, web browser hoặc terminal không có DISPLAY.

## 2. Chức năng thật

| Chức năng | Hành vi trên Ubuntu |
|---|---|
| Tạo người dùng | `useradd -m -U -s /bin/bash`, tạo home và nhóm riêng; chưa có mật khẩu |
| Tạo nhóm | `groupadd` |
| Thêm vào nhóm | `usermod -aG`, giữ các nhóm phụ cũ |
| Đặt mật khẩu | Mở Terminal chạy `passwd`, người dùng tự nhập hai lần |
| Khóa | `usermod -L -e 1`, khóa mật khẩu và đặt ngày hết hạn |
| Mở khóa | `usermod -U -e ''`, cần có mật khẩu; bỏ ngày hết hạn |
| Xóa | `userdel`, giữ dữ liệu home, cần nhập đúng `DELETE tên_tài_khoản` |
| Chi tiết | `id` và `chage -l` |
| Tìm kiếm / Làm mới | Lọc tên và đọc lại dữ liệu hệ thống từ hệ thống |
| Nhật ký | Lưu `/var/log/linux-user-manager-qt/events.log`, tải 100 dòng cuối khi mở lại |

Phạm vi tài khoản và nhóm có tiền tố `ugm_`. Ví dụ: `ugm_ngan`, `ugm_linux`. Đây là tên tài khoản hệ thống để tách phạm vi đồ án, dùng cho phạm vi quản trị của ứng dụng. Giới hạn tên tối đa 31 ký tự, chỉ chữ thường ASCII, số và `_`, ký tự đầu sau tiền tố phải là chữ cái.

Ứng dụng không sửa root, UID dưới 1000, UID 65534 hay tài khoản gọi sudo. Không cấp quyền sudo, không chỉnh nhóm hệ thống hoặc LDAP/AD. Không quản trị đồng thời bằng nhiều phiên. Các trạng thái bảng nói rõ khóa mật khẩu hoặc hết hạn, không suy diễn rằng mọi dịch vụ đều cho phép hoặc từ chối đăng nhập.

Đặt mật khẩu ưu tiên `xterm`, dự phòng `gnome-terminal --wait`. Ứng dụng không nhập, lưu hoặc ghi log mật khẩu. Việc mở Terminal không được coi là đặt mật khẩu thành công. Sau khi Terminal kết thúc, đọc lại trạng thái tài khoản; có thể dùng `sudo passwd -S ugm_ngan` để xác minh. Nếu tài khoản bị hết hạn do Khóa, cần Mở khóa sau khi đặt mật khẩu.

Khóa không chấm dứt phiên đã đăng nhập. Xóa giữ nguyên home; Ubuntu có thể tự xóa nhóm riêng cùng tên theo cấu hình. Không tạo lại cùng tên khi còn dữ liệu cũ mà chưa xem xét quyền sở hữu. Nhật ký được bảo vệ khỏi đường dẫn symlink ở thư mục và tệp đích, tệp chỉ root đọc/ghi; nếu ghi thất bại, GUI thông báo cảnh báo, không khẳng định đã hoàn tác thao tác hệ thống.

## 3. Kịch bản thuyết trình

1. Giới thiệu đề tài, C++17, Qt Widgets, QProcess và kiến thức tài khoản Linux.
2. Mở ứng dụng. Ban đầu bảng trống nếu chưa có tài khoản `ugm_`.
3. Tạo `ugm_ngan`, tạo nhóm `ugm_linux` và `ugm_sinhvien`.
4. Chọn người dùng, thêm lần lượt hai nhóm. Mở Chi tiết để xem cả hai được giữ.
5. Đặt mật khẩu thật trong Terminal. Tự nhập; không ghi mật khẩu lên slide.
6. Khóa, Làm mới, xem trạng thái hết hạn. Mở khóa rồi đối chiếu `chage -l`.
7. Đóng ứng dụng, mở lại: tài khoản và nhật ký vẫn còn.
8. Xóa: thử chuỗi xác nhận sai, sau đó nhập đúng. Xem nhật ký và đối chiếu hệ thống.

Chỉ dùng các tài khoản thực hành do bạn tạo trong máy ảo. Các thay đổi giữ lại sau khi đóng ứng dụng và khởi động lại Ubuntu.

## 4. Kiểm tra độc lập trên Ubuntu

```bash
id ugm_ngan
getent passwd ugm_ngan
getent group ugm_linux
sudo passwd -S ugm_ngan
sudo chage -l ugm_ngan
sudo tail -n 20 /var/log/linux-user-manager-qt/events.log
```

Sau xóa, `getent passwd ugm_ngan` không còn kết quả; thư mục home vẫn còn. Không dùng việc đăng nhập từ root để kết luận khóa tài khoản đã hoạt động với tài khoản thường.

## 5. Cấu trúc mã nguồn

```text
linux-user-manager-qt/
  CMakeLists.txt
  src/main.cpp            Dịch vụ tài khoản, nhật ký và giao diện Qt
  src/policy.h            Kiểm tra tên, UID, mật khẩu và xác nhận xóa
  src/theme.h             Giao diện, màu sắc, kiểu bảng và thành phần Qt
  tests/policy_tests.cpp   Kiểm thử C++ độc lập, không thay đổi tài khoản
  DE_CUONG.md             Đề cương và ma trận kiểm thử Ubuntu
  TEST_RESULTS.md         Kết quả và giới hạn xác minh
  LICENSE                 MIT
```

Hai lớp chính: `Accounts` phụ trách dữ liệu và lệnh hệ thống, `Window` phụ trách giao diện. Cấu hình và cấp quyền dùng CMake cùng công cụ Linux. Mã nguồn ứng dụng viết bằng C++ và Qt Widgets.

Lệnh quản trị chạy đồng bộ, chờ tối đa 20 giây mỗi lệnh. Trong thời gian đó GUI có thể tạm ngừng nhận thao tác. Nếu quá thời gian, ứng dụng dừng tiến trình và yêu cầu làm mới vì lệnh có thể đã thay đổi một phần trạng thái. Phần nhập mật khẩu chạy trong tiến trình Terminal riêng; không chặn vòng lặp giao diện.
