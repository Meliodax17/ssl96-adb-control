# Hướng dẫn build ra file HEX

Firmware cho module SSL 96-pixel ADB, chạy trên board Lumissil USB-Tool EB-V01C
(STM32F103C8T6). Tên file kết quả: **`SSL96_ADB_TPS9266x.hex`**

---

## Dùng hằng ngày: bấm đúp vào `tools\SSL96.cmd`

Không cần gõ lệnh. Cửa sổ điều khiển mở ra với:

- **Sáu đèn trạng thái** cho IC301 đến IC306, xanh là tốt, vàng là ghi được nhưng đọc lỗi,
  đỏ là không liên lạc được
- **Thanh trượt độ sáng**, kèm dòng cảnh báo tự tính số LED sáng đồng thời và điện áp cần
  cho chuỗi ANODE#1, đổi sang đỏ ngay khi vượt ngân sách 40V
- Nút **Bật tất cả**, **Tắt hết**, **Về bảng mặc định**, **Chẩn đoán lại**
- Sáu nút bật riêng từng IC
- Ô kết quả hiện nguyên văn báo cáo từ board

Bản dòng lệnh `tools\read_diag_usb.ps1` vẫn giữ nguyên cho ai cần chạy tự động.
Cả hai dùng chung lớp giao tiếp USB trong `tools\ssl96_hid.ps1`.

---

## Trạng thái: build đã chạy được

Biên dịch thành công lần đầu ngày 14/09/2026. Toàn bộ 35 file nguồn, build sạch mất 23,6 giây.

| Mục | Giá trị |
|---|---|
| FLASH | 13 828 / 65 536 byte (21,1%) |
| RAM | 2 552 / 20 480 byte (12,5%) |
| File HEX | 38 965 byte |

Ảnh chương trình đã kiểm chứng: con trỏ stack ban đầu là `0x20005000` đúng đỉnh RAM, vector
reset trỏ tới `0x08003245` khớp điểm vào của ELF, file HEX bắt đầu ở vùng FLASH `0x08000000`.
Nạp chip được.

### Ghi chú quan trọng về đường dẫn

Project **phải** nằm ở đường dẫn ngắn. Trước đây nó ở trong Downloads lồng bốn tầng, khiến
đường dẫn của `startup_stm32f10x_md.s` dài 305 ký tự và `misc.c` dài 265 ký tự, vượt giới hạn
**260 ký tự** của Windows. Khi đó `arm-none-eabi-gcc.exe` và `armcc.exe` của Keil không mở được
file. Điều đánh lừa là `ls` trong bash vẫn thấy file bình thường, vì bash dùng API khác.

Ở `C:\SSL96` đường dẫn dài nhất chỉ còn 96 ký tự nên vấn đề đã hết. **Đừng chuyển project trở
lại thư mục sâu.** Nếu bắt buộc, dùng task *Build: bien dich qua o dia ao* để tạo tạm ổ đĩa ảo.
`tools/build.ps1` tự kiểm tra độ dài ngay khi khởi động nên sẽ báo lỗi rõ ràng chứ không để
build hỏng giữa chừng.

---

## Hai chỗ đã phải sửa để build được

Ghi lại để sau này không sửa ngược trở lại.

**1. `Libraries/CMSIS/CM3/CoreSupport/core_cm3.c`** — ba hàm `__STREXB`, `__STREXH`, `__STREXW`
dùng ràng buộc `"=r"` cho toán hạng kết quả. GCC vì thế có thể chọn cùng một thanh ghi cho cả
kết quả lẫn giá trị, sinh ra lệnh `strexb r0, r0, [r1]` mà kiến trúc ARM không cho phép, và
trình dịch hợp ngữ báo `registers may not be the same`. Đã đổi thành `"=&r"` đúng theo bản vá
chính thức của CMSIS. Đây là lỗi của CMSIS đời cũ khi gặp binutils mới, không phải lỗi mã của
mình. Nhánh dành cho Keil trong cùng file viết khác nên không ảnh hưởng.

**2. `tools/build.ps1`** — Windows PowerShell 5.1 bọc mỗi dòng stderr của chương trình ngoài
vào một bản ghi lỗi, nên chỉ một dòng cảnh báo của gcc cũng làm dừng cả script khi
`$ErrorActionPreference = 'Stop'`. Đã thêm hàm `Invoke-Native` hạ mức xuống `Continue` quanh
mỗi lần gọi chương trình ngoài và chỉ dựa vào mã thoát để kết luận.

---

## Dùng Antigravity IDE (cách chính)

Antigravity là bản phái sinh của VS Code nên nó đọc thư mục `.vscode`. Mọi thứ đã cấu hình sẵn.

### Lần đầu: cài trình biên dịch

Máy hiện chưa có trình biên dịch nào. Mở Antigravity, bấm `Ctrl+Shift+P`, gõ **Tasks: Run Task**,
chọn:

> **Cai dat: tai Arm GNU Toolchain**

Task này tải Arm GNU Toolchain và giải nén vào `tools/arm-gnu/` ngay trong project. Không cần
quyền quản trị, không đụng vào PATH của hệ thống. Muốn gỡ thì xoá thư mục đó là xong.

File tải khoảng 400 MB nên lần đầu hơi lâu. Nếu máy không ra được Internet, tải file zip ở máy
khác rồi chỉ đường dẫn:

```powershell
powershell -ExecutionPolicy Bypass -File tools\get_toolchain.ps1 -ZipFile D:\arm-gnu.zip
```

Cần bản Windows, tên dạng `arm-gnu-toolchain-<phiên bản>-mingw-w64-i686-arm-none-eabi.zip`.

### Build

Bấm **`Ctrl+Shift+B`**. Xong.

Kết quả nằm ở `build/`:

| File | Dùng để |
|---|---|
| `SSL96_ADB_TPS9266x.hex` | Nạp chip |
| `SSL96_ADB_TPS9266x.bin` | Nạp theo địa chỉ tuyệt đối |
| `SSL96_ADB_TPS9266x.elf` | Debug |
| `SSL96_ADB_TPS9266x.map` | Xem hàm nào chiếm bao nhiêu bộ nhớ |

Cuối mỗi lần build script in ra dung lượng FLASH và RAM đã dùng trên tổng 64 KB và 20 KB của
chip, kèm phần trăm. Quá 100% thì script báo lỗi ngay thay vì để ra file hỏng.

### Danh sách task

`Ctrl+Shift+P` → **Tasks: Run Task**:

| Task | Việc |
|---|---|
| Build: bien dich ra file HEX | Chỉ biên dịch file đã thay đổi (mặc định, `Ctrl+Shift+B`) |
| Build: bien dich lai tu dau | Xoá hết rồi biên dịch lại toàn bộ |
| Build: bien dich qua o dia ao | Dùng khi đường dẫn quá 260 ký tự, xem mục đầu trang |
| Build: xoa ket qua build | Xoá thư mục `build/` |
| Flash: nap chip bang ST-LINK | Nạp file HEX mới nhất |
| Build va nap chip | Biên dịch xong nạp luôn |
| Cai dat: tai Arm GNU Toolchain | Chỉ chạy một lần |
| Build bang Keil MDK | Dùng project Keil thay vì GCC |

Lỗi biên dịch hiện thẳng trong tab **Problems** và bấm được để nhảy tới dòng lỗi, nhờ
`problemMatcher` kiểu `$gcc` trong `tasks.json`.

### Nạp chip

Cắm ST-LINK vào cổng SWD của board, rồi chạy task **Flash: nap chip bang ST-LINK**.

Script tự tìm `ST-LINK_CLI.exe`, nếu không có thì tìm `STM32_Programmer_CLI.exe` của
STM32CubeProgrammer. Máy này đã có ST-LINK Utility nên chạy được ngay.

Nó cũng tự chọn file HEX mới nhất giữa bản build bằng GCC và bản build bằng Keil, nên không sợ
nạp nhầm bản cũ.

### Debug

File `.vscode/launch.json` đã có sẵn cấu hình cho extension **Cortex-Debug**
(`marus25.cortex-debug`) chạy qua OpenOCD. Cần cài thêm extension đó và OpenOCD. Nếu chỉ nạp
chip mà không cần debug từng dòng thì bỏ qua phần này.

### IntelliSense

`.vscode/c_cpp_properties.json` đã khai báo đủ đường dẫn header và hai macro
`USE_STDPERIPH_DRIVER`, `STM32F10X_MD`, trùng khớp với `tools/build.ps1`. Nhờ vậy IDE nhảy được
tới định nghĩa hàm và không báo lỗi giả ở các thanh ghi STM32.

**Lưu ý:** nếu sau này thêm file `.c` mới, phải thêm vào **cả hai** chỗ: mảng `$sources` trong
`tools/build.ps1` và danh sách file trong `RVMDK/Project.uvprojx` nếu còn dùng Keil.

---

## Chạy bằng dòng lệnh, không cần IDE

Các script trong `tools/` dùng độc lập được:

```powershell
powershell -ExecutionPolicy Bypass -File tools\build.ps1
powershell -ExecutionPolicy Bypass -File tools\build.ps1 -Rebuild
powershell -ExecutionPolicy Bypass -File tools\build.ps1 -Clean
powershell -ExecutionPolicy Bypass -File tools\flash.ps1
```

Trình biên dịch nằm chỗ khác thì chỉ đường dẫn thư mục `bin`:

```powershell
powershell -ExecutionPolicy Bypass -File tools\build.ps1 -ToolchainDir "D:\arm-gnu\bin"
```

`build.ps1` không cần `make` hay CMake. Nó tự gọi `arm-none-eabi-gcc` cho từng file rồi link.
Biên dịch tăng dần theo thời gian sửa file, nên lần build thứ hai trở đi chỉ dịch lại phần đã đổi.

---

## Dùng Keil MDK (nếu có)

Project Keil `RVMDK/Project.uvprojx` vẫn dùng được và đã được cập nhật:

- Thêm `tps9266x.c` và `adb_dimming.c` vào nhóm `User`
- Đổi tên file xuất thành `SSL96_ADB_TPS9266x` (tên cũ có dấu `%` và dấu cách, gây lỗi script)
- Bật sẵn `Create HEX File` và bước sinh thêm file `.bin`

Mở bằng µVision rồi bấm F7, hoặc chạy `RVMDK\build_hex.bat`, hoặc dùng task
**Build bang Keil MDK** trong IDE.

Ngoài ra thư mục `gcc/` có sẵn `Makefile` cho ai quen dùng `make`. Cần cài thêm `make`, còn
`tools/build.ps1` thì không.

---

## Tình trạng công cụ trên máy này

Kiểm tra lúc thiết lập:

| Công cụ | Trạng thái |
|---|---|
| Antigravity IDE | **Có** |
| arm-none-eabi-gcc | **Có** — Arm GNU Toolchain 13.2.rel1 tại `tools\arm-gnu` |
| ST-LINK_CLI.exe | **Có**, nạp chip chạy được ngay |
| Độ dài đường dẫn | **Đạt** — dài nhất 96 ký tự |
| Keil MDK (UV4.exe) | Không có, nhưng không cần |
| make, cmake, ninja | Không có, `build.ps1` không dùng tới |

Trình biên dịch nằm gọn trong `tools\arm-gnu`, không cài vào hệ thống và không sửa PATH. Muốn gỡ
thì xoá thư mục đó.

**Chưa nạp lên phần cứng thử.** Firmware đã biên dịch sạch và ảnh chương trình hợp lệ, nhưng
chưa chạy thật trên board nên hành vi thực tế còn phải kiểm chứng.

Còn một cảnh báo lúc biên dịch, ở `User/I2C.c`. Macro `SDA_read` trong `I2C.h` thiếu ngoặc nên
`!SDA_read` bị hiểu thành `(!GPIOB->IDR) & GPIO_Pin_7`, luôn ra 0. Đây là lỗi tiềm ẩn có thật
trong mã I2C cũ của nhà cung cấp, nhưng firmware này không dùng tới I2C nên không ảnh hưởng.
Thực tế còn không được phép gọi `I2C_GPIO_Init()`, vì nó chiếm PB6 và PB7 vốn đang dùng cho
UART1. `main.c` đã ghi chú cảnh báo chuyện này.

---

## Nếu build báo lỗi

**`undefined reference` tới một hàm cũ của chương trình LT3365**
Còn file nào đó vẫn gọi hàm đã bị gỡ. Tìm tên hàm trong `User/main_LT3365_backup.c.txt` để xem
nó làm gì, rồi bỏ lời gọi hoặc viết lại theo API mới trong `User/tps9266x.h`.

**`multiple definition`**
Nhiều khả năng `main.c` cũ vẫn còn trong danh sách build. Bản sao lưu để đuôi `.txt` chính là để
tránh việc này; đừng đổi nó về `.c`.

**Tràn FLASH hoặc RAM**
Chip chỉ có 64 KB FLASH và 20 KB RAM. Chương trình cũ chứa các bảng PWM rất lớn (`Pwm1` đến
`Pwm5`, vài chục KB) nhưng chúng đã bị gỡ trong bản mới nên dung lượng phải giảm đáng kể. Xem
file `.map` để biết phần nào chiếm nhiều nhất.

**Task báo không tìm thấy `powershell`**
Đổi `"command": "powershell"` trong `.vscode/tasks.json` thành `"pwsh"` nếu máy chỉ có
PowerShell 7.

**Nạp chip thất bại**
Kiểm tra ST-LINK đã cắm chưa, dây SWD (SWDIO = PA13, SWCLK = PA14, GND) đã nối đúng chưa, board
đã có nguồn chưa.
