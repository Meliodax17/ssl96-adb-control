/**
  ******************************************************************************
  * @file    adb_dimming.h
  * @brief   Ban do 96 pixel <-> 26 vi tri LED x 4 chuoi (A/B/C/D) va bang do
  *          sang mac dinh cho module SSL 96-pixel ADB.
  *
  *  So do chuoi noi tiep thuc te tren so do nguyen ly (khong phai mux):
  *      ANODE#1 (CN100 chan 7)  -> IC301 -> IC302 -> IC305 -> IC306 -> CATHODE
  *      ANODE#2 (CN100 chan 9)  -> IC303 -> CATHODE
  *      ANODE#3 (CN100 chan 11) -> IC304 -> CATHODE
  *
  *  Nhanh ANODE#1 noi tiep 4 IC (~64 LED, can khoang 190-210 V de dan het
  *  chuoi). Khong thu nghiem nhanh nay bang nguon phong thi nghiem thong
  *  thuong; hay chan doan qua thanh ghi STATUS thay vi do dien tro tinh.
  ******************************************************************************
  */
#ifndef __ADB_DIMMING_H
#define __ADB_DIMMING_H

#include "stm32f10x.h"
#include "tps9266x.h"

#define ADB_LED_COUNT     26     /* vi tri LED 1..26                         */
#define ADB_STRING_COUNT  4      /* chuoi A, B, C, D                         */

/* Chi so chuoi */
#define ADB_STR_A         0
#define ADB_STR_B         1
#define ADB_STR_C         2
#define ADB_STR_D         3

/* Mat na chuoi dung cho ADB_SetStrings() */
#define ADB_MASK_A        0x01
#define ADB_MASK_B        0x02
#define ADB_MASK_C        0x04
#define ADB_MASK_D        0x08
#define ADB_MASK_AB       (ADB_MASK_A | ADB_MASK_B)
#define ADB_MASK_CD       (ADB_MASK_C | ADB_MASK_D)
#define ADB_MASK_ALL      0x0F

/* Nhan cua mot pixel: thuoc chuoi nao, o vi tri LED nao */
typedef struct
{
    uint8_t str;   /* ADB_STR_A .. ADB_STR_D */
    uint8_t led;   /* 1 .. 26                */
} ADB_PixelLabel;

/* Pixel 0..95 -> (IC, kenh):  ic = pixel / 16, ch = pixel % 16 */
#define ADB_PIXEL_TO_DEV(p)  ((uint8_t)((p) / TPS_CH_PER_DEV))
#define ADB_PIXEL_TO_CH(p)   ((uint8_t)((p) % TPS_CH_PER_DEV))

/* Nhan cua 96 pixel, theo dung thu tu kenh WIDTH01..WIDTH16 cua tung IC */
extern const ADB_PixelLabel ADB_PIXEL_LABEL[TPS_PIXEL_COUNT];

/* Bang do sang mac dinh, da quy doi ra gia tri WIDTH 10 bit (0..1023).
   Chi so: [IC][kenh]. */
extern const uint16_t ADB_WIDTH_DEFAULT[TPS_DEV_COUNT][TPS_CH_PER_DEV];

/* ------------------------------------------------------------------------- */
/*  API - moi ham chi thay doi bo dem trong RAM.                             */
/*  Phai goi TPS_FlushAll() sau do de gui xuong IC.                          */
/* ------------------------------------------------------------------------- */

/* Nap bang do sang mac dinh. LUU Y: phai goi SAU TPS_Init(), vi TPS_Init()
   xoa trang bo dem WIDTH. */
void ADB_LoadDefault(void);

/* Nap bang mac dinh nhung ty le theo phan tram (0..100) */
void ADB_LoadDefaultScaled(uint8_t percent);

/*  Gioi han do rong xung TOI DA cua tung kenh (cat ngon, khong ty le).
 *
 *  Dung khi bo nguon khong du dien ap cho ca chuoi noi tiep. Board nguon
 *  32LT3365 chi cho duoi 40V, tuc khoang 13 LED sang dong thoi. Chuoi
 *  ANODE#1 co 4 IC = 64 kenh, nen moi kenh chi duoc sang khoang 1/5 chu ky
 *  (WIDTH <= 205) thi so LED sang cung luc moi nam trong ngan sach do.
 *
 *  Mat nguoi tich phan theo thoi gian, nen ca 64 LED van trong nhu dang
 *  sang du tai moi thoi diem chi khoang 13 con thuc su dan dien.
 *
 *  Chi co tac dung khi cac kenh da duoc RAI DEU PHA (xem TPS_SetPhaseSpread).
 *  Phai goi SAU khi nap bang do sang. */
void ADB_ClampWidth(uint16_t maxWidth);

/* Dat do sang cho mot pixel (0..95) */
void ADB_SetPixel(uint8_t pixel, uint16_t width);

/* Dat do sang theo toa do (chuoi, vi tri LED). Tra ve 1 neu toa do ton tai. */
uint8_t ADB_SetLed(uint8_t str, uint8_t led, uint16_t width);

/* Tim pixel theo toa do. Tra ve 0xFF neu toa do khong co pixel. */
uint8_t ADB_FindPixel(uint8_t str, uint8_t led);

/* Chi giu lai cac chuoi trong mat na, cac chuoi con lai tat han */
void ADB_SetStrings(uint8_t strMask);

/* Tat cac pixel co vi tri LED nam trong khoang [ledFrom .. ledTo] - dung de
   tao vung toi ADB truoc xe doi dien. Truyen ledFrom > ledTo de khong tat gi. */
void ADB_SetDarkWindow(uint8_t ledFrom, uint8_t ledTo);

#endif /* __ADB_DIMMING_H */
