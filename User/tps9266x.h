/**
  ******************************************************************************
  * @file    tps9266x.h
  * @brief   Driver giao thuc UART cho TPS92664-Q1 (MASTER) + TPS92667-Q1 (SLAVE)
  *          dung tren module SSL 96-pixel ADB.
  *
  *          Tai lieu tham chieu:
  *            - TI SLUSE18  (TPS92664-Q1)  muc 7.3.10 / 7.5
  *            - TI SLUSEI7  (TPS92667-Q1)  muc 7.3.10 / 7.5
  *
  *          Phan cung: Lumissil USB-Tool EB-V01C FOR CAN (STM32F103C8T6)
  *            USART1 REMAP  -> PB6 = TXD1, PB7 = RXD1 -> TJA1042 (U4)
  *            PA8           -> NEN (chan S cua TJA1042, R15 khong gan)
  *            Baud 1 Mbps, 8 data bit, 1 stop bit, khong parity (8N1)
  ******************************************************************************
  */
#ifndef __TPS9266X_H
#define __TPS9266X_H

#include "stm32f10x.h"

/* ------------------------------------------------------------------------- */
/*  Cau hinh he thong                                                        */
/* ------------------------------------------------------------------------- */
#define TPS_DEV_COUNT            6      /* IC301 .. IC306                    */
#define TPS_CH_PER_DEV           16     /* 16 kenh bypass switch / IC        */
#define TPS_PIXEL_COUNT          (TPS_DEV_COUNT * TPS_CH_PER_DEV)  /* 96 */
#define TPS_WIDTH_MAX            1023   /* PWM 10 bit                        */

/* Dia chi logic (do chan ADDRx tren module quyet dinh) */
#define TPS_ADDR_IC301           0      /* TPS92664-Q1  MASTER              */
#define TPS_ADDR_IC302           1      /* TPS92667-Q1  SLAVE               */
#define TPS_ADDR_IC303           2
#define TPS_ADDR_IC304           3
#define TPS_ADDR_IC305           4
#define TPS_ADDR_IC306           5
#define TPS_ADDR_BROADCAST       0xFF   /* gia dia chi -> DEVID 0xBF         */

/* Bat ACK (0x7F) sau moi lenh ghi don thiet bi. 1 = bat (khuyen nghi). */
#define TPS_USE_ACK              1

/* ------------------------------------------------------------------------- */
/*  Ban do thanh ghi (volatile) - chung cho ca 92664 va 92667                */
/* ------------------------------------------------------------------------- */
#define TPS_REG_MTPCFG           0x00
#define TPS_REG_OUTCTRL          0x01
#define TPS_REG_TWLMT            0x02   /* chi co tren TPS92664             */
#define TPS_REG_SLEWL            0x03
#define TPS_REG_SLEWH            0x04
#define TPS_REG_DEFWIDTH01       0x05   /* 0x05 .. 0x14                     */
#define TPS_REG_PHASE_BASE       0x15   /* 0x15 .. 0x28  (20 byte)          */
#define TPS_REG_WIDTH_BASE       0x29   /* 0x29 .. 0x3C  (20 byte)          */
#define TPS_REG_BANK_PHASE_BASE  0x3D
#define TPS_REG_BANK_WIDTH_BASE  0x51
#define TPS_REG_SYSCFG           0x80
#define TPS_REG_CMWTAP           0x81
#define TPS_REG_PWMTICK          0x82
#define TPS_REG_CLK_SYNC         0x84
/* CLK_SYNC (0x84), bang 7-42 SLUSE18 */
#define TPS_CLKSYNC_VDD_OV_FLT   0x04   /* da tung co su co qua ap VDD      */
#define TPS_CLKSYNC_CLK_IS_EXT   0x02   /* 1 = dang chay bang clock NGOAI   */
#define TPS_REG_STATUS           0x85
#define TPS_REG_FAULT_OPEN_L     0x86
#define TPS_REG_FAULT_OPEN_H     0x87
#define TPS_REG_FAULT_SHORT_L    0x88
#define TPS_REG_FAULT_SHORT_H    0x89
#define TPS_REG_FAULT_RESFET_L   0x8A
#define TPS_REG_FAULT_RESFET_H   0x8B
#define TPS_REG_CERRCNT          0x8C
#define TPS_REG_ADC1             0x8D   /* chi co tren TPS92664             */
#define TPS_REG_ADC2             0x8E   /* chi co tren TPS92664             */
#define TPS_REG_DIETEMP          0x8F   /* CHI CO tren TPS92664 (MASTER)    */
#define TPS_REG_CS               0x90   /* chi co tren TPS92664             */
#define TPS_REG_ICID             0xFF

/* MTPCFG (0x00) */
#define TPS_MTPCFG_EN_VDD_OV     0x40
#define TPS_MTPCFG_FS_PIN        0x20   /* ghi 1 de thoat che do Fail-Safe  */

/* OUTCTRL (0x01) */
#define TPS_OUTCTRL_DRV_CHK      0x80
#define TPS_OUTCTRL_TX_STR       0x10
#define TPS_OUTCTRL_SYNC_STR     0x08
#define TPS_OUTCTRL_SYNCOEN      0x04
#define TPS_OUTCTRL_SYNCPEN      0x02
#define TPS_OUTCTRL_LVDS_TX      0x01   /* chi TPS92664: phat clock LVDS    */

/* SYSCFG (0x80) */
#define TPS_SYSCFG_SEPTR         0x80   /* 0 = CAN physical, 1 = UART truc tiep */
#define TPS_SYSCFG_LEDADCEN      0x40   /* chi TPS92664                     */
#define TPS_SYSCFG_ACKEN         0x20
#define TPS_SYSCFG_PSON          0x10
#define TPS_SYSCFG_CMWEN         0x08
#define TPS_SYSCFG_CSGAIN        0x04   /* chi TPS92664                     */
#define TPS_SYSCFG_CHPMP_SS      0x02
#define TPS_SYSCFG_CSEN          0x01   /* chi TPS92664                     */

/* STATUS (0x85) */
#define TPS_STATUS_PWM_ERR       0x80
#define TPS_STATUS_CHPMP_ERR     0x40
#define TPS_STATUS_VLED_ERR      0x20
#define TPS_STATUS_LIMP_HOME     0x10
#define TPS_STATUS_TRIM_ERR      0x08
#define TPS_STATUS_TW            0x04
#define TPS_STATUS_MTP_ERR       0x02
#define TPS_STATUS_PWR           0x01   /* 0 = da co power cycle -> init lai */

/* Ma nhan dang IC (thanh ghi ICID 0xFF) */
#define TPS_ICID_92664           0x96
#define TPS_ICID_92667           0x98

#define TPS_ACK_BYTE             0x7F

/* ------------------------------------------------------------------------- */
/*  Ma loi tra ve                                                            */
/* ------------------------------------------------------------------------- */
#define TPS_OK                   0
#define TPS_ERR_PARAM            1      /* tham so khong hop le             */
#define TPS_ERR_TX_TIMEOUT       2      /* UART khong gui duoc              */
#define TPS_ERR_ECHO             3      /* khong nhan duoc echo tu CAN xcvr */
#define TPS_ERR_RX_TIMEOUT       4      /* thiet bi khong tra loi           */
#define TPS_ERR_CRC              5      /* CRC cua goi tra ve bi sai        */
#define TPS_ERR_ACK              6      /* khong nhan duoc ACK 0x7F         */

/* ------------------------------------------------------------------------- */
/*  Ket qua chan doan                                                        */
/* ------------------------------------------------------------------------- */
typedef struct
{
    uint8_t present;      /* 1 = doc duoc ICID hop le                        */
    uint8_t icid;         /* 0x96 = TPS92664, 0x98 = TPS92667                */
    uint8_t status;       /* thanh ghi STATUS (0x85)                        */
    uint8_t faultOpenL;
    uint8_t faultOpenH;
    uint8_t faultShortL;
    uint8_t faultShortH;
    uint8_t crcErrCnt;    /* CERRCNT                                        */
    int16_t dieTempC;     /* chi hop le voi IC301 (TPS92664)                */
} TPS_DevInfo;

/* ------------------------------------------------------------------------- */
/*  API                                                                      */
/* ------------------------------------------------------------------------- */

/* Tre thoi gian (vong lap ban, doc lap SysTick) */
void     TPS_DelayUs(uint32_t us);
void     TPS_DelayMs(uint32_t ms);

/* CRC-16-IBM: poly 0xA001 (phan chieu cua 0x8005), gia tri khoi tao 0x0000 */
uint16_t TPS_Crc16Ibm(const uint8_t *buf, uint16_t len);

/* Ha tang UART (USART1 remap PB6/PB7, 1 Mbps, 8N1) */
void     TPS_UartInit(void);
void     TPS_CommsReset(void);

/* Truy cap thanh ghi. addr = 0..5 hoac TPS_ADDR_BROADCAST */
uint8_t  TPS_WriteRegs(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t n);
uint8_t  TPS_WriteReg8(uint8_t addr, uint8_t reg, uint8_t val);
uint8_t  TPS_ReadRegs (uint8_t addr, uint8_t reg, uint8_t *out,  uint8_t n);
uint8_t  TPS_ReadReg8 (uint8_t addr, uint8_t reg, uint8_t *out);

/* Tra ve byte DEVID ung voi dia chi 0..5, hoac 0xBF neu addr la broadcast.
   Phan chan doan dung ham nay de in dung byte thuc su duoc gui len bus. */
uint8_t  TPS_DevIdOf(uint8_t addr);

/* Mat na cac IC ghi WIDTH that bai o lan TPS_FlushAll() gan nhat.
   Bit 0 ung voi IC301. Bang 0 nghia la ca 6 con deu nhan duoc du lieu. */
extern uint8_t TPS_FlushFailMask;

/*  Thoi gian cho giua hai khung o che do SEPTR = 0 (lop vat ly CAN).
 *  Datasheet doi toi thieu 0.5 bit time CONG voi do tre vong lai cua bo thu
 *  phat CAN. O 1 Mbps mot bit la 1 us. De rong rai cho chac. */
#define TPS_TURNAROUND_US   30u

/*  So lan thu toi da cho moi giao dich (1 = khong thu lai).
 *  Giua hai lan thu co phat Communications Reset. */
extern uint8_t  TPS_MaxTries;

/*  So lan bo dem nhan bi tran. Khac 0 nghia la vi dieu khien khong doc kip
 *  byte vong lai, khung se bi lech va sinh loi chap chon. */
extern uint32_t TPS_OverrunCount;

/*  Tong so lan phai thu lai mot giao dich. Cang lon thi bus cang kem on dinh. */
extern uint32_t TPS_RetryCount;

/*  Ghi WIDTH cho moi kenh cua MOI IC bang lenh broadcast.
 *  Broadcast khong doi thiet bi tra loi, chi can thiet bi nghe duoc, nen day
 *  la cach duy nhat phan biet "IC diec mot chieu" voi "IC chet han". */
uint8_t  TPS_BroadcastWidthAll(uint16_t width);

/*  Ghi WIDTH cho moi kenh cua MOT dia chi bat ky 0..15.
 *  Dung de dieu khien IC bi ho chan ADDRx nen nam ngoai dai 0..5. */
uint8_t  TPS_WriteWidthDirect(uint8_t addr, uint16_t width);

/*  Dem so byte toi tren duong nhan trong ms mili giay khi KHONG phat gi.
    Bus lanh manh phai cho ket qua 0. transceiverOff=1 se tat bo thu phat
    CAN de biet nhieu den tu ngoai cap hay sinh ra tren board. */
uint32_t TPS_MeasureBusNoise(uint32_t ms, uint8_t transceiverOff);

/* Khoi tao toan bo chuoi 6 IC. Tra ve TPS_OK neu thanh cong. */
uint8_t  TPS_Init(void);

/* Bo dem WIDTH trong RAM (chua gui xuong IC) */
void     TPS_SetWidth(uint8_t dev, uint8_t ch, uint16_t width);
uint16_t TPS_GetWidth(uint8_t dev, uint8_t ch);
void     TPS_SetWidthAll(uint16_t width);

/* Gui bo dem WIDTH xuong IC (goi ghi 20 byte tai 0x29) */
uint8_t  TPS_FlushWidth(uint8_t dev);
uint8_t  TPS_FlushAll(void);

/* Tien ich */
uint8_t  TPS_AllOff(void);
uint8_t  TPS_SetPhaseSpread(uint8_t dev);
uint8_t  TPS_ReadDevInfo(uint8_t addr, TPS_DevInfo *info);
uint8_t  TPS_CheckPowerCycle(uint8_t *needReinit);

/* Bien trang thai chan doan (cap nhat boi TPS_Init) */
extern uint8_t  TPS_LastError;
extern uint8_t  TPS_DevOnline;   /* bitmask: bit0 = IC301 ... bit5 = IC306 */

#endif /* __TPS9266X_H */
