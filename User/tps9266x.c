/**
  ******************************************************************************
  * @file    tps9266x.c
  * @brief   Driver giao thuc UART cho TPS92664-Q1 / TPS92667-Q1 (module SSL 96px)
  ******************************************************************************
  */

#include "tps9266x.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <string.h>

/* ------------------------------------------------------------------------- */
/*  Bang tra cuu giao thuc (SLUSE18 bang 7-2 va bang 7-7)                    */
/* ------------------------------------------------------------------------- */

/* So byte du lieu hop le trong mot khung */
static const uint8_t s_lenTab[10]      = {   1,    2,    3,    4,    5,    8,   12,   16,   20,   32 };
static const uint8_t s_initWriteTab[10]= {0x87, 0x99, 0x1E, 0xAA, 0xAD, 0x13, 0x2D, 0x33, 0xB5, 0xB4};
static const uint8_t s_initReadTab[10] = {0x4B, 0xCC, 0xD2, 0x55, 0xE5, 0x26, 0xE1, 0x66, 0x7A, 0x78};

/* DEVID byte cho vung nho VOLATILE, theo dia chi 0..5 (bang 7-7).
   LUU Y: KHONG duoc dung truc tiep gia tri dia chi 0..5 lam DEVID. */
/*  Byte DEVID cua CA 16 dia chi ma ho TPS9266x ho tro (cot VOLATILE cua
 *  bang 7-7, SLUSE18). Module chi dung 6 dia chi dau, nhung phan chan
 *  doan can quet het 16 de phat hien truong hop chan ADDRx bi dat sai:
 *  khi do IC van song nhung nam o mot dia chi khac han. */
static const uint8_t s_devIdTab[16] = {
    0x20, 0x61, 0xE2, 0xA3, 0x64, 0x25, 0xA6, 0xE7,
    0xA8, 0xE9, 0x6A, 0x2B, 0xEC, 0xAD, 0x2E, 0x6F
};
#define TPS_DEVID_BROADCAST   0xBF

/* ------------------------------------------------------------------------- */
/*  Bien noi bo                                                              */
/* ------------------------------------------------------------------------- */
uint8_t  TPS_LastError     = TPS_OK;
uint8_t  TPS_DevOnline     = 0;
uint8_t  TPS_FlushFailMask = 0;
uint32_t TPS_OverrunCount  = 0;
uint32_t TPS_RetryCount    = 0;

/*  So lan thu toi da cho moi giao dich. 1 = khong thu lai.
 *
 *  Vi sao can thu lai: o che do SEPTR = 0, cac IC KHONG duoc goi ten van
 *  phai dem so byte vong lai tren RX de giu dong bo may trang thai. Chi can
 *  mot byte bi mat la chung lech nhip va se lam ngo moi khung sau do, cho
 *  toi khi nhan duoc lenh Communications Reset. Vi vay khi mot giao dich
 *  that bai, cach dung la phat lenh reset giao thuc roi thu lai. */
uint8_t  TPS_MaxTries      = 3;

/* Bo dem WIDTH cho 6 IC x 16 kenh */
static uint16_t s_width[TPS_DEV_COUNT][TPS_CH_PER_DEV];

/* Che do echo cua lop vat ly CAN:
   2 = chua biet, 1 = co echo (SEPTR = 0, dung TJA1042), 0 = khong co echo */
/* Bien s_echoMode cu da bi go bo: xem chu thich trong TpsSendFrame. */

/*  1 = board noi qua bo thu phat CAN (SEPTR=0), moi byte gui deu vong lai
    tren RX va PHAI doc bo ngay. 0 = noi UART truc tiep, khong co echo.
    Board EB-V01C dung TJA1042 nen luon la 1. */
#define TPS_EXPECT_ECHO    1

#define TPS_FLAG_TIMEOUT   60000UL   /* ~4 ms @72MHz                        */

/* ------------------------------------------------------------------------- */
/*  Tre thoi gian                                                            */
/* ------------------------------------------------------------------------- */
void TPS_DelayUs(uint32_t us)
{
    volatile uint32_t i;
    while (us--)
    {
        for (i = 0; i < 12; i++) { __NOP(); }
    }
}

void TPS_DelayMs(uint32_t ms)
{
    while (ms--) { TPS_DelayUs(1000); }
}

/* ------------------------------------------------------------------------- */
/*  CRC-16-IBM                                                               */
/*  poly 0xA001 (phan chieu cua 0x8005), khoi tao 0x0000, RefIn/RefOut       */
/*  Da kiem chung voi 3 vi du trong SLUSE18 muc 7.3.10.7.                    */
/* ------------------------------------------------------------------------- */
uint16_t TPS_Crc16Ibm(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0x0000;
    uint16_t i;
    uint8_t  b;

    for (i = 0; i < len; i++)
    {
        crc ^= (uint16_t)buf[i];
        for (b = 0; b < 8; b++)
        {
            if (crc & 0x0001) { crc = (uint16_t)((crc >> 1) ^ 0xA001); }
            else              { crc = (uint16_t)(crc >> 1); }
        }
    }
    return crc;
}

/* ------------------------------------------------------------------------- */
/*  Lop vat ly UART                                                          */
/*                                                                           */
/*  Board Lumissil EB-V01C: USART1 duoc REMAP sang PB6 (TX) / PB7 (RX),      */
/*  noi toi TJA1042 qua R17/R20. PA9/PA10 tren board nay di toi TJA1020      */
/*  (phan LIN, khong lap linh kien) nen KHONG duoc dung.                     */
/* ------------------------------------------------------------------------- */
void TPS_UartInit(void)
{
    GPIO_InitTypeDef  gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_USART1|
                           RCC_APB2Periph_AFIO, ENABLE);

    /* PA8 = NEN (chan S cua TJA1042) -> muc thap = Normal mode.
       Tren board V01C dien tro R15 khong gan, chan S da duoc keo xuong bang
       R16; viec dieu khien PA8 chi de du phong, khong gay hai. */
    gpio.GPIO_Pin   = GPIO_Pin_8;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);

    /* USART1 full remap: PB6 = TX, PB7 = RX */
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

    gpio.GPIO_Pin   = GPIO_Pin_6;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin   = GPIO_Pin_7;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio);

    /* 1 Mbps, 8 data bit, 1 stop bit, khong parity (SLUSE18 muc 7.3.10.2) */
    usart.USART_BaudRate            = 1000000;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);

    /* Driver nay dung che do hoi dap (polling). Tat han ngat va DMA de
       khong xung dot voi DMA1_Channel5_IRQHandler cua uart.c. */
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);
    USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);

    USART_Cmd(USART1, ENABLE);

    /* Khong con trang thai echo can dat lai: che do echo la co dinh. */
}

/* Xoa sach bo dem nhan va co bao loi */
static void TpsRxFlush(void)
{
    volatile uint32_t dummy = 0;
    uint32_t guard = 64;

    while ((USART1->SR & (USART_FLAG_RXNE | USART_FLAG_ORE)) && guard--)
    {
        dummy = USART1->SR;
        dummy = USART1->DR;
    }
    (void)dummy;
}

/*  Doi mot byte toi. Tra ve 1 neu nhan duoc, 0 neu that bai.
 *
 *  Ban truoc CHI kiem tra co RXNE ma bo qua co ORE (tran bo dem). STM32F103
 *  chi co bo dem nhan mot byte; khi byte moi toi ma byte cu chua duoc doc
 *  thi ORE duoc bat va byte do MAT LUON. Neu khong phat hien, khung se bi
 *  lech byte va sinh ra loi CRC hoac het gio cho mot cach ngau nhien, dung
 *  kieu loi chap chon dang thay. Bay gio dem lai so lan tran de bao cao. */
static uint8_t TpsRxByte(uint8_t *out, uint32_t timeout)
{
    uint32_t sr;

    while (timeout--)
    {
        sr = USART1->SR;

        if (sr & USART_FLAG_ORE)
        {
            /* Doc DR de xoa co ORE theo dung trinh tu doc SR roi doc DR */
            (void)USART1->DR;
            TPS_OverrunCount++;
            return 0;
        }

        if (sr & USART_FLAG_RXNE)
        {
            *out = (uint8_t)(USART1->DR & 0xFF);
            return 1;
        }
    }
    return 0;
}

/*  Gui mot khung.
 *  Voi lop vat ly CAN (SEPTR = 0) tin hieu TX duoc TJA1042 phan hoi nguoc
 *  lai tren RX. STM32F103 chi co bo dem nhan 1 byte nen phai doc bo ngay
 *  tung byte echo, neu khong se bi loi tran (ORE) va mat byte tra loi.
 */
static uint8_t TpsSendFrame(const uint8_t *buf, uint8_t len)
{
    uint8_t  i, echo;
    uint32_t guard;

    TpsRxFlush();

    for (i = 0; i < len; i++)
    {
        guard = TPS_FLAG_TIMEOUT;
        while (!(USART1->SR & USART_FLAG_TXE))
        {
            if (guard-- == 0) { return TPS_ERR_TX_TIMEOUT; }
        }
        USART1->DR = buf[i];

#if TPS_EXPECT_ECHO
        /*  Board nay noi qua TJA1042 o che do SEPTR = 0, nen MOI byte gui di
         *  deu duoc vong lai tren RX. Bat buoc phai doc bo ngay tung byte,
         *  vi STM32F103 chi co bo dem nhan MOT byte.
         *
         *  KHONG con tu dong doan che do echo nua. Ban truoc khoi tao bien
         *  s_echoMode = 2 ("chua biet"), va chi mot lan doc echo that bai o
         *  byte dau tien la no chuyen han sang che do "khong co echo" va
         *  VINH VIEN thoi doc byte vong lai. Ke tu luc do moi byte gui di
         *  deu nam lai trong bo dem, gay tran lien tuc, va moi cau tra loi
         *  that cua IC deu chim trong dong byte chua doc -> ca sau IC cung
         *  cam lang cho toi khi cat nguon. Day la cai bay tu gay ra, khong
         *  phai hong phan cung. */
        if (!TpsRxByte(&echo, TPS_FLAG_TIMEOUT))
        {
            /* Bao loi de tang tren phat Communications Reset roi thu lai,
               thay vi lang le doi che do. */
            return TPS_ERR_ECHO;
        }
        (void)echo;
#else
        (void)echo;
#endif
    }

    guard = TPS_FLAG_TIMEOUT;
    while (!(USART1->SR & USART_FLAG_TC))
    {
        if (guard-- == 0) { return TPS_ERR_TX_TIMEOUT; }
    }

    return TPS_OK;
}

/*  Reset giao thuc: giu duong RX cua cac IC o muc thap it nhat
 *  192 chu ky clock he thong (192 / 16 MHz = 12 us). Dung 200 us cho chac. */
void TPS_CommsReset(void)
{
    GPIO_InitTypeDef gpio;

    USART_Cmd(USART1, DISABLE);

    gpio.GPIO_Pin   = GPIO_Pin_6;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    GPIO_ResetBits(GPIOB, GPIO_Pin_6);   /* TXD = 0 -> bus dominant         */
    TPS_DelayUs(200);
    GPIO_SetBits(GPIOB, GPIO_Pin_6);
    TPS_DelayUs(50);

    gpio.GPIO_Pin  = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &gpio);

    USART_Cmd(USART1, ENABLE);
    TpsRxFlush();
}

/* ------------------------------------------------------------------------- */
/*  Dung khung lenh                                                          */
/* ------------------------------------------------------------------------- */

/* Tra ve chi so trong bang do dai, hoac 0xFF neu do dai khong hop le */
static uint8_t TpsLenIndex(uint8_t n)
{
    uint8_t i;
    for (i = 0; i < 10; i++)
    {
        if (s_lenTab[i] == n) { return i; }
    }
    return 0xFF;
}

uint8_t TPS_DevIdOf(uint8_t addr)
{
    if (addr == TPS_ADDR_BROADCAST) { return TPS_DEVID_BROADCAST; }
    if (addr < 16u)                 { return s_devIdTab[addr];    }
    return 0xFF;
}

#define TpsDevIdOf(a) TPS_DevIdOf(a)

/*  Ghi n byte lien tiep bat dau tu thanh ghi reg.
 *  CRC duoc tinh tren TOAN BO phan dau khung: INIT + DEVID + REGADDR + DATA.
 */
static uint8_t TpsWriteRegsOnce(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t n)
{
    uint8_t  frame[3 + 32 + 2];
    uint8_t  idx, devid, err, ack;
    uint16_t crc;

    idx   = TpsLenIndex(n);
    devid = TpsDevIdOf(addr);
    if (idx == 0xFF || devid == 0xFF || data == 0) { return TPS_ERR_PARAM; }

    frame[0] = s_initWriteTab[idx];
    frame[1] = devid;
    frame[2] = reg;
    memcpy(&frame[3], data, n);

    crc = TPS_Crc16Ibm(frame, (uint16_t)(3 + n));
    frame[3 + n]     = (uint8_t)(crc & 0xFF);        /* CRCL gui truoc      */
    frame[3 + n + 1] = (uint8_t)(crc >> 8);          /* CRCH                */

    err = TpsSendFrame(frame, (uint8_t)(3 + n + 2));
    if (err != TPS_OK) { TPS_LastError = err; return err; }

#if TPS_USE_ACK
    /* Chi lenh ghi don thiet bi moi co ACK. Broadcast khong tra loi. */
    if (addr != TPS_ADDR_BROADCAST)
    {
        if (!TpsRxByte(&ack, TPS_FLAG_TIMEOUT)) { TPS_LastError = TPS_ERR_ACK; return TPS_ERR_ACK; }
        if (ack != TPS_ACK_BYTE)                { TPS_LastError = TPS_ERR_ACK; return TPS_ERR_ACK; }
        /* Thoi gian quay bus toi thieu 0.5 bit sau ACK (SLUSE18 7.3.10.4) */
        TPS_DelayUs(TPS_TURNAROUND_US);
    }
#else
    (void)ack;
#endif

    return TPS_OK;
}

/*  Ghi co thu lai. Giua hai lan thu co phat lenh Communications Reset de
 *  keo moi IC tren bus ve lai trang thai cho INIT, vi mot byte bi mat co
 *  the lam cac IC khong duoc goi ten lech nhip dem va lam ngo mai mai. */
uint8_t TPS_WriteRegs(uint8_t addr, uint8_t reg, const uint8_t *data, uint8_t n)
{
    uint8_t err = TPS_ERR_PARAM;
    uint8_t t;

    for (t = 0; t < TPS_MaxTries; t++)
    {
        if (t > 0)
        {
            TPS_RetryCount++;
            TPS_CommsReset();
            TPS_DelayUs(200);
        }
        err = TpsWriteRegsOnce(addr, reg, data, n);
        if (err == TPS_OK) { return TPS_OK; }
        if (err == TPS_ERR_PARAM) { break; }   /* sai tham so, thu lai vo ich */
    }
    TPS_LastError = err;
    return err;
}

uint8_t TPS_WriteReg8(uint8_t addr, uint8_t reg, uint8_t val)
{
    return TPS_WriteRegs(addr, reg, &val, 1);
}

/*  Doc n byte lien tiep bat dau tu thanh ghi reg.
 *  CRC cua khung lenh doc phu INIT + DEVID + REGADDR (khong co DATA).
 *  CRC cua goi TRA LOI chi phu phan DATA.
 */
static uint8_t TpsReadRegsOnce(uint8_t addr, uint8_t reg, uint8_t *out, uint8_t n)
{
    uint8_t  frame[3 + 2];
    uint8_t  rsp[32 + 2];
    uint8_t  idx, devid, err, i;
    uint16_t crc, crcRx;

    idx   = TpsLenIndex(n);
    devid = TpsDevIdOf(addr);
    if (idx == 0xFF || out == 0)                  { return TPS_ERR_PARAM; }
    if (devid == 0xFF || addr == TPS_ADDR_BROADCAST) { return TPS_ERR_PARAM; }

    frame[0] = s_initReadTab[idx];
    frame[1] = devid;
    frame[2] = reg;

    crc = TPS_Crc16Ibm(frame, 3);
    frame[3] = (uint8_t)(crc & 0xFF);
    frame[4] = (uint8_t)(crc >> 8);

    err = TpsSendFrame(frame, 5);
    if (err != TPS_OK) { TPS_LastError = err; return err; }

    for (i = 0; i < (uint8_t)(n + 2); i++)
    {
        if (!TpsRxByte(&rsp[i], TPS_FLAG_TIMEOUT))
        {
            TPS_LastError = TPS_ERR_RX_TIMEOUT;
            return TPS_ERR_RX_TIMEOUT;
        }
    }

    crc   = TPS_Crc16Ibm(rsp, n);                       /* chi tren DATA    */
    crcRx = (uint16_t)(((uint16_t)rsp[n + 1] << 8) | rsp[n]);
    if (crc != crcRx) { TPS_LastError = TPS_ERR_CRC; return TPS_ERR_CRC; }

    memcpy(out, rsp, n);

    /* Thoi gian quay bus truoc khung ke tiep (SEPTR = 0) */
    TPS_DelayUs(TPS_TURNAROUND_US);
    return TPS_OK;
}

/*  Doc co thu lai, xem chu thich o TPS_WriteRegs. */
uint8_t TPS_ReadRegs(uint8_t addr, uint8_t reg, uint8_t *out, uint8_t n)
{
    uint8_t err = TPS_ERR_PARAM;
    uint8_t t;

    for (t = 0; t < TPS_MaxTries; t++)
    {
        if (t > 0)
        {
            TPS_RetryCount++;
            TPS_CommsReset();
            TPS_DelayUs(200);
        }
        err = TpsReadRegsOnce(addr, reg, out, n);
        if (err == TPS_OK) { return TPS_OK; }
        if (err == TPS_ERR_PARAM) { break; }
    }
    TPS_LastError = err;
    return err;
}

uint8_t TPS_ReadReg8(uint8_t addr, uint8_t reg, uint8_t *out)
{
    return TPS_ReadRegs(addr, reg, out, 1);
}

/* ------------------------------------------------------------------------- */
/*  Bo dem WIDTH / PHASE                                                     */
/* ------------------------------------------------------------------------- */
void TPS_SetWidth(uint8_t dev, uint8_t ch, uint16_t width)
{
    if (dev >= TPS_DEV_COUNT || ch >= TPS_CH_PER_DEV) { return; }
    if (width > TPS_WIDTH_MAX) { width = TPS_WIDTH_MAX; }
    s_width[dev][ch] = width;
}

uint16_t TPS_GetWidth(uint8_t dev, uint8_t ch)
{
    if (dev >= TPS_DEV_COUNT || ch >= TPS_CH_PER_DEV) { return 0; }
    return s_width[dev][ch];
}

void TPS_SetWidthAll(uint16_t width)
{
    uint8_t d, c;
    if (width > TPS_WIDTH_MAX) { width = TPS_WIDTH_MAX; }
    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        for (c = 0; c < TPS_CH_PER_DEV; c++) { s_width[d][c] = width; }
    }
}

/*  Dong goi 16 gia tri 10 bit thanh 20 byte theo so do WIDTH / PHASE:
 *    byte 0..15 : gia tri[9:2] cua kenh 1..16
 *    byte 16    : [7:6]=k4[1:0] [5:4]=k3[1:0] [3:2]=k2[1:0] [1:0]=k1[1:0]
 *    byte 17    : nhom kenh 5..8
 *    byte 18    : nhom kenh 9..12
 *    byte 19    : nhom kenh 13..16
 */
static void TpsPack20(const uint16_t *val, uint8_t *out)
{
    uint8_t i, g;

    for (i = 0; i < 16; i++)
    {
        out[i] = (uint8_t)((val[i] >> 2) & 0xFF);
    }
    for (g = 0; g < 4; g++)
    {
        out[16 + g] = (uint8_t)(( (val[g * 4 + 0] & 0x03)      ) |
                                ( (val[g * 4 + 1] & 0x03) << 2 ) |
                                ( (val[g * 4 + 2] & 0x03) << 4 ) |
                                ( (val[g * 4 + 3] & 0x03) << 6 ));
    }
}

uint8_t TPS_FlushWidth(uint8_t dev)
{
    uint8_t packed[20];

    if (dev >= TPS_DEV_COUNT) { return TPS_ERR_PARAM; }

    TpsPack20(s_width[dev], packed);
    return TPS_WriteRegs(dev, TPS_REG_WIDTH_BASE, packed, 20);
}

/*  Gui bo dem WIDTH xuong TAT CA 6 IC.
 *
 *  CO Y KHONG loc theo TPS_DevOnline. Ly do: lenh GHI va lenh DOC di theo
 *  hai duong khac nhau. Tren board nay lenh ghi van chay tot (IC tra ACK)
 *  trong khi lenh doc co the that bai. Ban truoc cua ham nay bo qua moi IC
 *  khong tra loi lenh doc, nen nhung IC do khong bao gio nhan duoc WIDTH va
 *  nam toi vinh vien, du chung hoan toan nhan duoc lenh ghi. Do chinh la
 *  nguyen nhan lam chi mot chuoi sang.
 *
 *  Bay gio cu ghi cho ca 6 con, con nao that bai thi ghi nhan rieng vao
 *  TPS_FlushFailMask de phan chan doan bao cao.
 *
 *  (Dinh nghia ham nam ngay sau ham broadcast ben duoi.)
 */

/*  Ghi mot gia tri WIDTH cho TAT CA 16 kenh cua MOI IC bang lenh broadcast.
 *
 *  Diem mau chot: lenh broadcast KHONG doi thiet bi tra loi gi ca. No chi
 *  can thiet bi NGHE DUOC. Nho do phan biet duoc hai truong hop ma phep do
 *  thong thuong khong tach duoc:
 *
 *    - IC nghe duoc nhung khong noi duoc -> den VAN SANG
 *    - IC chet han hoac mat nguon        -> den VAN TOI
 *
 *  Dung de chan doan, khong dung trong van hanh binh thuong. */
/*  Ghi WIDTH cho moi kenh cua MOT dia chi bat ky trong dai 0..15.
 *
 *  Khac TPS_FlushWidth() o cho ham nay khong dung bo dem s_width (chi co 6
 *  phan tu) nen goi duoc toi ca nhung dia chi ngoai dai 0..5. Can thiet khi
 *  mot IC bi ho chan ADDRx va nam o dia chi la. */
/*  Do "nhieu nen" cua duong nhan: dem so byte toi trong mot khoang thoi gian
 *  ma vi dieu khien KHONG he phat gi.
 *
 *  Bus lanh manh thi con so nay phai bang 0, vi khi khong ai duoc hoi thi
 *  khong ai duoc phep noi.
 *
 *  CANH BAO ve tham so transceiverOff: tren board EB-V01C dien tro R15 KHONG
 *  duoc gan, nen PA8 thuc te KHONG noi toi chan S cua TJA1042 (chan do chi
 *  duoc keo xuong bang R16). Vi vay tham so nay hau nhu khong co tac dung
 *  tren board nay. Muon tach nguon nhieu thi phai RUT CAP CN100 bang tay roi
 *  do lai. Giu tham so o day de dung duoc tren ban mach co gan R15.
 *
 *  Tham so transceiverOff cho phep tat bo thu phat CAN (dua chan S len cao,
 *  che do Standby). So sanh hai lan do se biet nhieu tu dau ra:
 *
 *    - Co nhieu khi Normal, het nhieu khi Standby
 *        -> nhieu di VAO qua bo thu phat, tuc tu phia cap va module
 *    - Co nhieu o ca hai che do
 *        -> nhieu sinh ra ngay tren board dieu khien
 *    - Khong nhieu o ca hai
 *        -> bus yen tinh, van de chi xuat hien trong luc giao dich
 */
uint32_t TPS_MeasureBusNoise(uint32_t ms, uint8_t transceiverOff)
{
    uint32_t count = 0;
    uint32_t i, j;

    /* PA8 = chan S cua TJA1042: thap = Normal, cao = Standby */
    if (transceiverOff) { GPIO_SetBits(GPIOA, GPIO_Pin_8); }
    else                { GPIO_ResetBits(GPIOA, GPIO_Pin_8); }
    TPS_DelayMs(5);                 /* cho bo thu phat doi che do */

    TpsRxFlush();

    for (i = 0; i < ms; i++)
    {
        /* Mot vong xap xi 1 ms o 72 MHz, du cho phep do tuong doi */
        for (j = 0; j < 6000u; j++)
        {
            uint32_t sr = USART1->SR;
            if (sr & (USART_FLAG_RXNE | USART_FLAG_ORE))
            {
                (void)USART1->DR;
                count++;
            }
        }
    }

    GPIO_ResetBits(GPIOA, GPIO_Pin_8);   /* luon tra ve Normal */
    TPS_DelayMs(5);
    TpsRxFlush();

    return count;
}

uint8_t TPS_WriteWidthDirect(uint8_t addr, uint16_t width)
{
    uint16_t val[16];
    uint8_t  packed[20];
    uint8_t  i;

    if (width > TPS_WIDTH_MAX) { width = TPS_WIDTH_MAX; }
    for (i = 0; i < 16; i++) { val[i] = width; }
    TpsPack20(val, packed);

    return TPS_WriteRegs(addr, TPS_REG_WIDTH_BASE, packed, 20);
}

uint8_t TPS_BroadcastWidthAll(uint16_t width)
{
    uint16_t val[16];
    uint8_t  packed[20];
    uint8_t  i;

    if (width > TPS_WIDTH_MAX) { width = TPS_WIDTH_MAX; }
    for (i = 0; i < 16; i++) { val[i] = width; }
    TpsPack20(val, packed);

    return TPS_WriteRegs(TPS_ADDR_BROADCAST, TPS_REG_WIDTH_BASE, packed, 20);
}

uint8_t TPS_FlushAll(void)
{
    uint8_t d, err, first = TPS_OK;

    TPS_FlushFailMask = 0;

    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        err = TPS_FlushWidth(d);
        if (err != TPS_OK)
        {
            TPS_FlushFailMask |= (uint8_t)(1u << d);
            if (first == TPS_OK) { first = err; }
        }
    }
    return first;
}

/*  Rai pha deu 16 kenh tren chu ky PWM 1024 buoc de giam dot bien dong
 *  dien va nhieu EMI. */
/*  Do lech pha rieng cho tung IC, tinh theo VI TRI CUA NO TRONG CHUOI ANODE.
 *
 *  Vi sao can: cac IC tren cung mot duong anode noi TIEP nhau, nen dien ap
 *  bo nguon phai cap bang tong so LED dang sang tai CUNG MOT THOI DIEM cua
 *  ca chuoi. Neu moi IC dung chung mot mau pha thi kenh so 1 cua tat ca cac
 *  IC bat cung luc, va dinh tieu thu nhan len bang so IC tren chuoi.
 *
 *  Board nguon chi cho duoi 40V, tuc khoang 13 LED sang dong thoi. Chuoi
 *  ANODE#1 co 4 IC = 64 kenh. Rai deu 64 kenh tren chu ky 1024 buoc thi moi
 *  kenh cach nhau 16 buoc, va tai moi thoi diem chi mot phan nho cung sang.
 *
 *  Trong mot IC cac kenh cach nhau 64 buoc. Cong them do lech 16 buoc cho
 *  moi IC ke tiep trong chuoi la duoc 64 vi tri phan biet.
 *
 *  Bang duoi theo so do noi day thuc te cua module:
 *      ANODE#1 : IC301 -> IC302 -> IC305 -> IC306   (4 IC noi tiep)
 *      ANODE#2 : IC303 mot minh
 *      ANODE#3 : IC304 mot minh
 *  IC dung mot minh tren duong anode thi khong can lech, de 0. */
static const uint8_t s_phaseOffset[TPS_DEV_COUNT] = {
     0,   /* IC301 - ANODE#1, vi tri 1 */
    16,   /* IC302 - ANODE#1, vi tri 2 */
     0,   /* IC303 - ANODE#2, mot minh  */
     0,   /* IC304 - ANODE#3, mot minh  */
    32,   /* IC305 - ANODE#1, vi tri 3 */
    48    /* IC306 - ANODE#1, vi tri 4 */
};

uint8_t TPS_SetPhaseSpread(uint8_t dev)
{
    uint16_t phase[16];
    uint8_t  packed[20];
    uint8_t  i;
    uint16_t off;

    if (dev >= TPS_DEV_COUNT) { return TPS_ERR_PARAM; }
    off = (uint16_t)s_phaseOffset[dev];

    for (i = 0; i < 16; i++)
    {
        /* Modulo 1024 vi bo dem PWM chay vong tu 0 den 1023 */
        phase[i] = (uint16_t)(((uint16_t)(i * 64) + off) & 0x03FFu);
    }
    TpsPack20(phase, packed);
    return TPS_WriteRegs(dev, TPS_REG_PHASE_BASE, packed, 20);
}

uint8_t TPS_AllOff(void)
{
    uint8_t packed[20];
    uint8_t d, err, first = TPS_OK;

    TPS_SetWidthAll(0);
    memset(packed, 0, sizeof(packed));

    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        if (TPS_DevOnline != 0 && (TPS_DevOnline & (1u << d)) == 0) { continue; }
        err = TPS_WriteRegs(d, TPS_REG_WIDTH_BASE, packed, 20);
        if (err != TPS_OK && first == TPS_OK) { first = err; }
    }
    return first;
}

/* ------------------------------------------------------------------------- */
/*  Khoi tao chuoi thiet bi                                                  */
/* ------------------------------------------------------------------------- */
uint8_t TPS_Init(void)
{
    uint8_t d, err, val, icid;
    uint8_t sysCfg;
    uint8_t result = TPS_OK;

    /* Buoc 0: xoa bo dem WIDTH trong RAM. Moi profile do sang phai duoc nap
       SAU khi ham nay chay xong. */
    memset(s_width, 0, sizeof(s_width));
    TPS_DevOnline = 0;
    TPS_LastError = TPS_OK;

    /* Buoc 1: cho cac IC thoat POR va chot dia chi (>= 80 us sau POR) */
    TPS_DelayMs(20);
    TPS_CommsReset();
    TPS_DelayUs(500);

    /* Buoc 2: cau hinh SYSCFG bang lenh broadcast.
       SEPTR = 0 : lop vat ly CAN (TX duoc phan hoi tren RX)
       PSON  = 0 : PHASE dat thoi diem TAT LED (bat switch)
       CMWEN = 0 : tat watchdog truyen thong (firmware tu quan ly)
       CHPMP_SS = 1 : bat trai pho bom dien tich de giam EMI
       Phai lam TRUOC moi lenh ghi don thiet bi, vi lenh ghi don chi sinh ra
       ACK sau khi bit ACKEN da duoc bat. Broadcast khong bao gio co ACK nen
       an toan o ca hai trang thai. Luc nay chi IC301 nhan duoc lenh; cac
       slave se duoc cau hinh lai o buoc 4 sau khi da co clock. */
    sysCfg = TPS_SYSCFG_CHPMP_SS;
#if TPS_USE_ACK
    sysCfg |= TPS_SYSCFG_ACKEN;
#endif
    err = TPS_WriteRegs(TPS_ADDR_BROADCAST, TPS_REG_SYSCFG, &sysCfg, 1);
    if (err != TPS_OK)
    {
        /* Thu lai mot lan: khung dau tien cung la luc driver tu do xem bo
           thu phat CAN co phan hoi tin hieu TX tren RX hay khong. */
        TPS_CommsReset();
        TPS_DelayUs(500);
        err = TPS_WriteRegs(TPS_ADDR_BROADCAST, TPS_REG_SYSCFG, &sysCfg, 1);
    }
    if (err != TPS_OK) { TPS_LastError = err; return err; }
    TPS_DelayUs(100);

    /* Buoc 3: bat bo phat clock LVDS tren IC301 (TPS92664 - MASTER).
       IC302..IC306 la TPS92667, chung KHONG co dao dong noi dung cho logic
       va nam trong Fail-Safe cho toi khi nhan duoc clock ngoai tren
       CLK_H / CLK_L. Vi vay lenh nay phai di truoc moi lenh gui toi slave. */
    err = TPS_WriteReg8(TPS_ADDR_IC301, TPS_REG_OUTCTRL,
                        TPS_OUTCTRL_TX_STR | TPS_OUTCTRL_LVDS_TX);
    if (err != TPS_OK) { TPS_LastError = err; return err; }

    /* Cho cac slave bat duoc 16 xung clock lien tiep de chuyen sang clock ngoai */
    TPS_DelayMs(5);

    /* Buoc 4: phat lai SYSCFG cho cac slave vua tinh day */
    err = TPS_WriteRegs(TPS_ADDR_BROADCAST, TPS_REG_SYSCFG, &sysCfg, 1);
    if (err != TPS_OK) { TPS_LastError = err; return err; }
    TPS_DelayUs(100);

    /* Buoc 5: kiem tra tung IC bang thanh ghi nhan dang ICID (0xFF) */
    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        if (TPS_ReadReg8(d, TPS_REG_ICID, &icid) == TPS_OK)
        {
            if (icid == TPS_ICID_92664 || icid == TPS_ICID_92667)
            {
                TPS_DevOnline |= (uint8_t)(1u << d);
            }
        }
    }
    /*  Khong con thoat som khi khong con nao tra loi lenh doc.
     *  Lenh doc va lenh ghi di hai duong khac nhau; neu duong doc hong ma
     *  duong ghi con tot thi van phai tiep tuc cau hinh, neu khong toan bo
     *  den se toi trong khi phan cung hoan toan dung. Chi ghi nhan lai de
     *  bao cao. */
    if (TPS_DevOnline == 0)
    {
        TPS_LastError = TPS_ERR_RX_TIMEOUT;
        result        = TPS_ERR_RX_TIMEOUT;
    }

    /* Buoc 6: tat toan bo kenh TRUOC khi roi Fail-Safe, tranh loe sang.
       Lam cho CA 6 con, khong loc theo TPS_DevOnline: lenh ghi di duong
       khac lenh doc, mot IC khong tra loi lenh doc van co the nhan lenh ghi
       binh thuong. Xem chu thich o TPS_FlushAll. */
    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        if (TPS_FlushWidth(d) != TPS_OK) { result = TPS_LastError; }
        if (TPS_SetPhaseSpread(d) != TPS_OK) { result = TPS_LastError; }
    }

    /* Buoc 7: xoa co loi cu va danh dau da khoi tao (STATUS.PWR = 1).
       Sau nay doc lai bit nay: neu bang 0 tuc la IC da bi cup nguon. */
    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        TPS_WriteReg8(d, TPS_REG_STATUS, TPS_STATUS_PWR);
        TPS_WriteReg8(d, TPS_REG_CERRCNT, 0x00);
    }

    /* Buoc 8: roi Fail-Safe sang che do Normal (MTPCFG.FS_PIN = 1).
       Tu day WIDTH / PHASE trong bo nho volatile moi dieu khien cac switch.
       Truoc buoc nay tat ca switch chay theo muc chan FS -> LED khong sang,
       day la thiet ke binh thuong chu khong phai loi phan cung. */
    val = TPS_MTPCFG_EN_VDD_OV | TPS_MTPCFG_FS_PIN;
    err = TPS_WriteRegs(TPS_ADDR_BROADCAST, TPS_REG_MTPCFG, &val, 1);
    if (err != TPS_OK) { TPS_LastError = err; return err; }
    TPS_DelayUs(200);

    return result;
}

/* ------------------------------------------------------------------------- */
/*  Chan doan                                                                */
/* ------------------------------------------------------------------------- */
uint8_t TPS_ReadDevInfo(uint8_t addr, TPS_DevInfo *info)
{
    uint8_t buf[4];
    uint8_t err;

    if (info == 0 || addr >= TPS_DEV_COUNT) { return TPS_ERR_PARAM; }
    memset(info, 0, sizeof(TPS_DevInfo));

    err = TPS_ReadReg8(addr, TPS_REG_ICID, &info->icid);
    if (err != TPS_OK) { return err; }
    if (info->icid != TPS_ICID_92664 && info->icid != TPS_ICID_92667)
    {
        return TPS_ERR_CRC;
    }
    info->present = 1;

    if (TPS_ReadReg8(addr, TPS_REG_STATUS, &info->status) != TPS_OK) { return TPS_LastError; }

    /* 0x86..0x89 = FAULT_OPEN_L/H, FAULT_SHORT_L/H : doc lien tiep 4 byte */
    if (TPS_ReadRegs(addr, TPS_REG_FAULT_OPEN_L, buf, 4) == TPS_OK)
    {
        info->faultOpenL  = buf[0];
        info->faultOpenH  = buf[1];
        info->faultShortL = buf[2];
        info->faultShortH = buf[3];
    }

    TPS_ReadReg8(addr, TPS_REG_CERRCNT, &info->crcErrCnt);

    /* DIETEMP (0x8F) CHI ton tai tren TPS92664. Cac IC TPS92667 dung chan TS
       tuong tu thay the, doc thanh ghi nay se ra gia tri rac. */
    if (info->icid == TPS_ICID_92664)
    {
        uint8_t raw;
        if (TPS_ReadReg8(addr, TPS_REG_DIETEMP, &raw) == TPS_OK)
        {
            /* T[degC] = 0.9098 * DIETEMP - 50  (SLUSE18 bang 7-54) */
            info->dieTempC = (int16_t)(((int32_t)raw * 9098) / 10000 - 50);
        }
    }

    return TPS_OK;
}

/*  Doc bit STATUS.PWR cua IC301. Neu bang 0 nghia la IC da qua mot chu ky
 *  cup nguon va toan bo thanh ghi volatile da bi xoa -> can khoi tao lai. */
uint8_t TPS_CheckPowerCycle(uint8_t *needReinit)
{
    uint8_t status, err;

    if (needReinit == 0) { return TPS_ERR_PARAM; }
    *needReinit = 0;

    err = TPS_ReadReg8(TPS_ADDR_IC301, TPS_REG_STATUS, &status);
    if (err != TPS_OK) { *needReinit = 1; return err; }

    if ((status & TPS_STATUS_PWR) == 0) { *needReinit = 1; }
    return TPS_OK;
}
