# Đề cương báo cáo đồ án

## Tên đề tài

**Xây dựng ứng dụng quản lý người dùng và nhóm trên Linux bằng C++ và Qt.**

**Development of a User and Group Management Application on Linux Using C++ and Qt.**

## 1. Mục tiêu và phạm vi

Gom các thao tác quản trị người dùng, nhóm và trạng thái tài khoản Linux vào giao diện đồ họa. Làm việc với tài khoản cục bộ thật có tiền tố `ugm_`, Giảm thao tác nhập lệnh và kiểm tra lỗi trước khi gọi tiện ích quản trị. Hướng đến máy ảo Ubuntu phục vụ học tập, không thay thế bộ quản lý danh tính doanh nghiệp.

## 2. Cơ sở lý thuyết

- Tài khoản, UID, GID, nhóm chính, nhóm phụ, thư mục home và shell.
- `/etc/passwd`, `/etc/group`, `/etc/shadow` và quyền đọc tệp nhạy cảm.
- `useradd`, `groupadd`, `usermod`, `userdel`, `passwd`, `id`, `chage`.
- C++17, lớp, xử lý ngoại lệ; Qt Widgets, signal/slot, QProcess và CMake.
- Mã nguồn mở: giấy phép MIT của dự án; thư viện Qt có điều khoản riêng, không được thay thế bởi MIT của dự án.

## 3. Phân tích yêu cầu

Đầu vào là tên người dùng, tên nhóm, tài khoản được chọn và chuỗi xác nhận xóa. Đầu ra là dữ liệu hệ thống, kết quả lệnh và nhật ký. Ràng buộc: cần root, desktop Ubuntu, tài khoản cục bộ đúng phạm vi; không quản lý root, nhóm quản trị hoặc tài khoản của người đang chạy sudo.

## 4. Thiết kế

`main`: khởi tạo Qt và kiểm tra Linux/root. `Accounts`: đọc dữ liệu, kiểm tra điều kiện, gọi QProcess, kiểm tra mã thoát, ghi log. `Window`: bảng tài khoản, tìm kiếm, nút tác vụ, hộp thoại và nhật ký. `policy.h`: quy tắc thuần C++, kiểm thử được mà không cần root.

Luồng tác vụ: chọn tài khoản → nhập thông tin → kiểm tra điều kiện → xác nhận nếu cần → chạy chương trình bằng danh sách đối số → kiểm tra mã thoát → ghi nhật ký → đọc lại hệ thống → cập nhật bảng. Không dựng lệnh shell từ dữ liệu người dùng.

Mật khẩu: mở Terminal cho người dùng tự nhập qua `passwd`; ứng dụng chỉ đọc trạng thái sau khi kết thúc, không giữ mật khẩu. Không đưa nội dung `/etc/shadow` hoặc hash vào ảnh báo cáo.

## 5. Triển khai

Trình bày môi trường Ubuntu, phiên bản compiler/Qt/CMake và cấu trúc mã. Minh họa xử lý đầu vào, bảo vệ tài khoản, danh sách đối số QProcess, bảo toàn nhóm phụ, xác nhận xóa và quyền tệp log. Dùng ảnh chụp thật sau khi chạy bản C++/Qt, không dùng lại ảnh Tkinter cũ.

## 6. Ma trận kiểm thử trên Ubuntu

| Trường hợp | Kết quả cần đối chiếu |
|---|---|
| Khởi động không sudo | Từ chối thao tác quản trị, hướng dẫn lệnh chạy |
| Chưa có tài khoản ugm_ | Bảng trống; không tự tạo tài khoản |
| Tên `root`, ký tự shell, khoảng trắng, quá dài | Từ chối trước khi chạy lệnh |
| Tạo tài khoản và nhóm | `getent`, home và nhóm chính khớp |
| Tạo trùng tên | Báo lỗi, không báo thành công |
| Thêm hai nhóm phụ | `id` hiển thị cả hai nhóm |
| Đặt mật khẩu | Người dùng tự nhập; `passwd -S` phản ánh trạng thái thật |
| Hủy/không mở được Terminal mật khẩu | Không báo đã đặt mật khẩu thành công |
| Khóa và mở khóa | `passwd -S`, `chage -l` khớp; phiên cũ không bị ngắt |
| Mở khóa khi chưa có mật khẩu | Từ chối |
| Sửa tài khoản gọi sudo hoặc UID hệ thống | Từ chối |
| Xóa sai xác nhận | Tài khoản vẫn tồn tại |
| Xóa đúng xác nhận | Tài khoản mất khỏi passwd, home vẫn còn |
| Đóng/mở ứng dụng | Tài khoản và nhật ký vẫn còn |
| Lệnh thất bại / tệp log không an toàn | Báo lỗi hoặc cảnh báo, không giả lập kết quả |

Kiểm thử `policy-tests` chỉ kiểm tra quy tắc, không chứng minh tích hợp Qt và hệ điều hành. Ghi kết quả thực tế của từng dòng sau khi chạy trên VM.

## 7. Đánh giá và hướng phát triển

Đánh giá độ chính xác thao tác, xử lý lỗi và khả năng trình diễn. Hạn chế: GUI chạy root, giới hạn tiền tố, lệnh đồng bộ, không hỗ trợ quản trị đồng thời hoặc LDAP/AD. Hướng phát triển: tách helper có đặc quyền với GUI không đặc quyền, tiến trình quản trị bất đồng bộ, thêm kiểm thử VM dùng một lần và chức năng bỏ thành viên khỏi nhóm.

## Tài liệu tham khảo

Tra cứu tại máy Ubuntu: `man useradd`, `man usermod`, `man userdel`, `man groupadd`, `man passwd`, `man chage`. Tra cứu Qt Assistant hoặc tài liệu chính thức Qt 5 cho QWidget, QTableWidget, QProcess và signal/slot. Báo cáo cần ghi phiên bản thực tế, không tự điền kết quả chưa thử.
