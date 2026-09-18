/**
  ******************************************************************************
  * @file    tps_diag.h
  * @brief   Bao cao chan doan chuoi TPS9266x, gui ve may tinh qua USB HID
  *
  *   Muc dich: bien trieu chung "chi 1 chuoi sang" thanh so lieu doc duoc.
  *   Voi tung dia chi 0..5 no lam bon phep thu va bao cao ket qua:
  *
  *     1. DOC   thanh ghi ICID (0xFF)      -> thiet bi co tra loi khong
  *     2. GHI   mot gia tri vao WIDTH01H   -> thiet bi co nhan lenh khong
  *     3. DOC LAI chinh thanh ghi vua ghi  -> duong doc co dung khong
  *     4. DOC   thanh ghi STATUS (0x85)    -> co loi TRIM / MTP / nhiet khong
  *
  *   Bon phep nay tach bach duoc duong GHI va duong DOC, la dieu ma den bao
  *   tren board khong lam duoc.
  *
  *   Bao cao duoc gui ve may tinh bang chinh cong USB cua board (HID 64 byte,
  *   VID 0x0483 PID 0x5750). Khong can dau them day, khong can ST-LINK.
  *   Doc bang:  powershell -File tools\read_diag_usb.ps1
  ******************************************************************************
  */
#ifndef __TPS_DIAG_H
#define __TPS_DIAG_H

#include "stm32f10x.h"

/* ------------------------------------------------------------------------- */
/*  Giao thuc bao cao tren USB HID                                           */
/*                                                                           */
/*  May tinh gui goi OUT 64 byte, byte[0] = DIAG_CMD_REQUEST.                */
/*  Board tra loi bang nhieu goi IN 64 byte, moi goi co dang:                */
/*                                                                           */
/*      byte 0 : DIAG_TAG            dau hieu day la goi bao cao             */
/*      byte 1 : so thu tu goi, bat dau tu 0                                 */
/*      byte 2 : tong so goi                                                 */
/*      byte 3 : so ky tu hop le trong goi nay (0..DIAG_CHUNK_TEXT)          */
/*      byte 4..63 : noi dung chu                                            */
/* ------------------------------------------------------------------------- */
#define DIAG_TAG            0xD1u
#define DIAG_CMD_REQUEST    0xD1u   /* may tinh xin ban bao cao gan nhat     */
#define DIAG_CMD_RERUN      0xD2u   /* may tinh yeu cau chay lai roi gui     */
#define DIAG_CMD_BCAST      0xD3u   /* bat sang moi kenh bang lenh broadcast */
#define DIAG_CMD_RESTORE    0xD4u   /* tra ve bang do sang binh thuong       */
#define DIAG_CMD_ONLY_IC    0xD5u   /* chi bat sang MOT IC; report[1]=0..5   */
#define DIAG_CMD_ALL_LEVEL  0xD6u   /* dat MOI kenh ve cung mot muc sang;
                                       report[1..2] = WIDTH 0..1023        */
#define DIAG_CMD_ONLY_STRING 0xD7u  /* cap nhat MOT KENH cua MOT IC;
                                       report[1] = IC (0..5),
                                       report[2] = Kenh (0..15),
                                       report[3..4] = WIDTH 0..1023 (LE)   */
#define DIAG_CMD_IC_LEVEL   0xD8u   /* dat CA 16 KENH cua MOT IC ve mot muc,
                                       KHONG dung toi cac IC khac;
                                       report[1] = IC (0..5),
                                       report[2..3] = WIDTH 0..1023 (LE)   */

#define DIAG_REPORT_SIZE    64u
#define DIAG_CHUNK_HDR      4u
#define DIAG_CHUNK_TEXT     (DIAG_REPORT_SIZE - DIAG_CHUNK_HDR)   /* = 60 */

/* Suc chua toi da cua mot ban bao cao, tinh bang ky tu */
#define DIAG_TEXT_MAX       3000u

/* Khoi tao bo dem bao cao. Goi mot lan luc khoi dong. */
void Diag_Init(void);

/* Ghi noi dung vao bo dem bao cao */
void Diag_Puts(const char *s);
void Diag_Hex8(uint8_t v);
void Diag_Dec(uint32_t v);
void Diag_Nl(void);

/*  Chay toan bo bao cao cho ca 6 IC va luu vao bo dem.
 *  Ham nay CO ghi thu vao thanh ghi WIDTH01H cua tung IC, nen sau khi goi
 *  phai nap lai bang do sang. */
void Diag_RunFullReport(void);

/*  Gui ban bao cao dang co trong bo dem ve may tinh qua USB HID.
 *  Tra ve 1 neu gui xong, 0 neu USB chua san sang hoac may tinh khong doc. */
uint8_t Diag_SendOverUsb(void);

/*  Xu ly mot goi OUT vua nhan tu may tinh.
 *  Tra ve 1 neu goi do la lenh cua phan chan doan va da duoc xu ly. */
uint8_t Diag_HandleUsbCommand(const uint8_t *report);

#endif /* __TPS_DIAG_H */
