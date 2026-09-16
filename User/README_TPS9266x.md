# Firmware điều khiển module SSL 96-pixel ADB (TPS92664 + TPS92667)

Bản viết lại của project `USB_TOOL_6CS3365B_AutoLED`, chuyển từ giao thức **LT3365** cũ
sang giao thức **TPS9266x** của module SSL 96-pixel ADB.

## 1. Vì sao phải viết lại

Chương trình cũ nói chuyện với chip LT3365: INIT byte 0x80 / 0x8F / 0x98, bảng device ID
`{0xB4, 0xA8, 0x20, 0xE2, 0xA6, 0x32}`, mỗi IC 12 kênh. Module SSL 96-pixel dùng
TPS92664-Q1 + 5 × TPS92667-Q1: INIT byte khác hẳn, DEVID khác, mỗi IC 16 kênh, và phải bật
clock LVDS từ IC master thì 5 IC slave mới hoạt động. Không có byte nào trùng nhau, nên
không thể vá dần mà phải thay lớp giao thức.

## 2. File mới

| File | Nội dung |
|---|---|
| `tps9266x.h` / `tps9266x.c` | Lớp giao thức UART: CRC, khung lệnh, đọc/ghi thanh ghi, khởi tạo chuỗi 6 IC, chẩn đoán |
| `adb_dimming.h` / `adb_dimming.c` | Bản đồ 96 pixel ↔ 26 vị trí LED × 4 chuỗi A/B/C/D, bảng độ sáng mặc định |
| `main.c` | Chương trình ứng dụng (đã thay hoàn toàn) |
| `main_LT3365_backup.c.txt` | Bản `main.c` gốc, giữ để đối chiếu |
| `main_LT3365_backup.h.txt` | Bản `main.h` gốc |

Hai file `.c` mới đã được thêm vào `RVMDK/Project.uvprojx` (nhóm `User`). Bản project gốc
nằm ở `Project.uvprojx.bak`.

## 3. Đấu nối phần cứng

```
STM32F103C8T6            TJA1042 (U4)        CN100 trên module
  PB6 = TXD1  --R17-->     TXD                1 = CAN_H
  PB7 = RXD1  <-R20---     RXD                3 = CAN_L
  PA8 = NEN   --R15-->     S  (R15 NC)        5 = SUPPLY 5V
                           CANH/CANL ------>  6 = GND
                                              7/9/11 = ANODE #1/#2/#3
                                              8/10   = CATHODE
```

**Quan trọng:** USART1 phải bật remap đầy đủ (PB6/PB7). Chân PA9/PA10 trên board này đi tới
TJA1020 phần LIN mà chính sơ đồ ghi "Not connected". Cũng **không được gọi `I2C_GPIO_Init()`**
vì hàm đó biến PB6/PB7 thành SCL/SDA và làm mất đường CAN.

Tham số UART: **1 Mbps, 8 data bit, 1 stop bit, không parity**, đúng theo SLUSE18 mục 7.3.10.2.

## 4. Giao thức đã kiểm chứng

Khung lệnh sinh ra từ code đã được đối chiếu từng byte với 3 ví dụ trong SLUSE18 mục 7.3.10.7:

| Trường hợp | Khung sinh ra | Datasheet |
|---|---|---|
| Broadcast ghi SYSCFG = 0x02 | `87 BF 80 02 F9 51` | khớp |
| Ghi 3 byte tới địa chỉ 4, reg 0x29 | `1E 64 29 55 56 57 14 64` | khớp |
| Đọc 2 byte từ địa chỉ 5, reg 0x00 | `CC 25 00 DA AF` | khớp |
| CRC gói trả lời (data `40 10`) | `0C30` | khớp |

Hai điểm rất dễ sai, đã xử lý đúng trong `tps9266x.c`:

- **DEVID không phải là địa chỉ 0–5.** Phải tra bảng: `0x20, 0x61, 0xE2, 0xA3, 0x64, 0x25`,
  broadcast là `0xBF`.
- **Phạm vi CRC khác nhau giữa lệnh và trả lời.** Lệnh ghi/đọc: CRC phủ
  `INIT + DEVID + REGADDR + DATA`. Gói trả lời của thiết bị: CRC **chỉ** phủ phần DATA.

Sai một trong hai điều trên thì mọi lệnh đều im lặng thất bại, không có dấu hiệu chẩn đoán nào.

## 5. Thứ tự khởi tạo

Thứ tự trong `TPS_Init()` không được đảo:

1. Reset giao thức: kéo đường RX của các IC xuống thấp 200 µs.
2. Broadcast `SYSCFG` (SEPTR = 0 cho lớp vật lý CAN, bật ACKEN). Phải làm trước mọi lệnh ghi
   đơn thiết bị, vì lệnh ghi đơn chỉ sinh ACK sau khi ACKEN đã bật; broadcast không bao giờ
   có ACK nên an toàn ở cả hai trạng thái.
3. Ghi `OUTCTRL.LVDS_TX = 1` cho IC301. **Bước quyết định:** 5 con TPS92667 không có dao động
   nội dùng cho logic, chúng nằm trong Fail-Safe cho tới khi nhận được clock ngoài trên
   CLK_H/CLK_L từ IC301. Trước bước này chúng hoàn toàn câm.
4. Broadcast lại `SYSCFG` cho các slave vừa tỉnh.
5. Dò từng IC bằng thanh ghi `ICID` (0xFF): `0x96` = TPS92664, `0x98` = TPS92667.
6. Ghi `WIDTH = 0` và rải pha cho từng IC.
7. Đặt `STATUS.PWR = 1` để sau này phát hiện mất nguồn.
8. Broadcast `MTPCFG.FS_PIN = 1` để rời Fail-Safe sang chế độ Normal.

Trước bước 8, mọi switch chạy theo mức chân FS nên LED không sáng. **Đây là thiết kế bình
thường, không phải hỏng phần cứng.**

## 6. Nghĩa của thanh ghi WIDTH

`WIDTH = 1023` → switch bypass mở → LED sáng tối đa.
`WIDTH = 0` → switch đóng gần hết chu kỳ → LED tắt.

Đây là ngược với trực giác "thời gian bật của switch bypass", rất dễ bị đảo dấu.

## 7. Cách dùng

```c
TPS_UartInit();
TPS_DelayMs(100);
TPS_Init();          /* hàm này XOÁ TRẮNG bộ đệm WIDTH */
ADB_LoadDefault();   /* nạp bảng độ sáng SAU khi TPS_Init() xong */
TPS_FlushAll();      /* gửi xuống IC */
```

Gọi `ADB_LoadDefault()` trước `TPS_Init()` thì bảng sẽ bị xoá sạch, LED tắt hết dù khởi tạo
báo thành công.

Các hàm tiện ích khác chỉ sửa bộ đệm RAM, phải gọi `TPS_FlushAll()` sau đó:

- `ADB_SetLed(ADB_STR_A, 16, 1023)` — đặt độ sáng theo toạ độ chuỗi/vị trí LED
- `ADB_SetStrings(ADB_MASK_AB)` — chỉ giữ chuỗi A và B
- `ADB_SetDarkWindow(10, 14)` — tạo vùng tối ADB từ LED 10 tới LED 14
- `ADB_LoadDefaultScaled(50)` — bảng mặc định ở 50%

## 8. Phím bấm và đèn báo

| Thao tác | Tác dụng |
|---|---|
| KEY1 (PB0) | Chuyển chế độ: tắt → bảng mặc định → chỉ chuỗi A+B → vùng tối chạy ngang → tất cả 50% |
| KEY2 (PB1) | Tắt toàn bộ LED và chạy lại chẩn đoán |
| PC13 sáng liên tục | Giao tiếp bình thường |
| PC13 nháy 100 ms | Lỗi giao tiếp với module |

## 9. Chẩn đoán

Mảng `g_devInfo[6]` (xem bằng debugger) chứa cho từng IC: `icid`, `status`, các thanh ghi lỗi
hở/chập LED, `crcErrCnt`, và nhiệt độ die (chỉ IC301).

Đọc `STATUS` (0x85) để phân biệt hỏng thật với trạng thái bình thường:

- `TRIM_ERR = 1` → IC hỏng thật, mọi lệnh ghi bị chặn vĩnh viễn.
- `MTP_ERR = 1` mà `TRIM_ERR = 0` → MTP chưa nạp, **bình thường** với thiết kế này vì
  firmware luôn tự ghi thanh ghi volatile.
- `PWR = 0` → module vừa mất nguồn, firmware tự khởi tạo lại.

**Không kết luận hỏng phần cứng từ phép đo điện trở tĩnh ANODE–CATHODE.** Điện trở đó phụ
thuộc hoàn toàn vào trạng thái switch (Fail-Safe hay Normal, và nếu Normal thì phụ thuộc giá
trị WIDTH đang đặt), nên một giá trị điện trở thấp có thể là hoàn toàn bình thường.

## 10. Cảnh báo về nhánh ANODE #1

Theo sơ đồ, ANODE #1 nối tiếp 4 IC (IC301 → IC302 → IC305 → IC306), khoảng 64 LED, cần
khoảng 190–210 V để dẫn hết chuỗi. ANODE #2 và #3 mỗi nhánh chỉ 1 IC (~16 LED, 48–54 V).
Không thử nhánh ANODE #1 bằng nguồn phòng thí nghiệm thông thường; hãy chẩn đoán qua UART.

## 11. Việc chưa làm

- Kênh điều khiển qua USB HID từ phần mềm trên máy tính: khung xử lý đã có sẵn trong vòng lặp
  chính của `main.c` nhưng phần thân để trống, vì tài liệu không mô tả định dạng lệnh mà phần
  mềm Lumissil gửi xuống.
- Chưa biên dịch thử: máy này không cài Keil MDK cũng như trình biên dịch ARM nào khác. Code
  đã được kiểm tra cân bằng cú pháp và đối chiếu giao thức, nhưng **cần build trong Keil** để
  xác nhận.
