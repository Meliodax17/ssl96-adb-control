/**
  ******************************************************************************
  * @file    main.c
  * @brief   Chuong trinh dieu khien module SSL 96-pixel ADB
  *          (TPS92664-Q1 MASTER + 5 x TPS92667-Q1 SLAVE)
  *          chay tren board Lumissil USB-Tool EB-V01C FOR CAN (STM32F103C8T6).
  *
  *  Ban nay thay the chuong trinh LT3365 cu. Ban goc duoc luu tai
  *  main_LT3365_backup.c.txt de doi chieu.
  *
  *  Duong tin hieu:
  *      STM32 USART1 (remap PB6/PB7, 1 Mbps 8N1)
  *        -> TJA1042 (U4) -> CAN_H/CAN_L -> CN100 chan 1/3 -> module SSL
  *
  *  Phim bam:
  *      KEY1 (PB0) : chuyen che do hien thi
  *      KEY2 (PB1) : tat toan bo LED va chay lai chan doan
  *
  *  Den bao PC13:
  *      sang lien tuc      = giao tiep binh thuong
  *      nhay nhanh (100ms) = loi giao tiep voi module
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "main.h"
#include "uart.h"
#include "delay.h"
#include "tps9266x.h"
#include "adb_dimming.h"
#include "tps_diag.h"
#include "usb_pwr.h"
#include "usb_lib.h"
#include "hw_config.h"
#include "usbio.h"
#include <string.h>

/* ========================================================================== */
/*  Cac bien bat buoc phai giu lai vi stm32f10x_it.c / uart.c / usb_endp.c /  */
/*  Command.c / I2C.c van tham chieu toi chung.                              */
/* ========================================================================== */
uint8_t  data[64] = {0};                 /* Command.c, I2C.c tham chieu       */
uint8_t  Pcdata[64] = {0};
uint8_t  Gu8data[64] = {0};
uint8_t  USART1_Rx_Buf[256] = {0};
uint8_t  USART2_Rx_Buf[256] = {0};
uint16_t USART1_RxCnt = 0, USART2_RxCnt = 0, Gu8ReadCnt = 10;
uint8_t  USART1_HaveMes = 0, USART2_HaveMes = 0;
uint8_t  Gu8ExtFlag = 0;
uint8_t  Gu8DmaComplete = 0, Gu8RealLen = 0;
uint32_t Gu8DMABufferLen = 15;
uint8_t  Gu8StartRw = 0;                 /* usb_endp.c tham chieu             */
uint8_t  Gu8BufCrc[9] = {0};             /* stm32f10x_it.c tham chieu         */
uint16_t Gu16Tim3Cnt = 0;
uint16_t LED_main_timer = 0;
uint32_t Modetime = 0;
int16_t  m = 0;
uint16_t j = 0;

uint8_t  B_Key_Up = 0, B_Key_Left = 0, B_Key_Enter = 0;
uint8_t  B_No_key = 0, B_Key_Combine = 0, B_respond = 0;
uint8_t  Keyin = 0, Keyvalue = 0, Key_In_Timer = 0;

extern volatile uint8_t USB_Received_Flag;

/* ========================================================================== */
/*  Trang thai ung dung                                                      */
/* ========================================================================== */
/*  Gioi han do rong xung toi da, theo ngan sach dien ap cua board nguon.
 *
 *  Board 32LT3365: boost ra 40V roi buck lai, nen dien ap chuoi LED luon
 *  duoi 40V, tuc khoang 13 LED sang dong thoi (LED ~3V).
 *  Chuoi ANODE#1 co 4 IC x 16 kenh = 64 kenh.
 *  64 / 13 ~ 5  ->  moi kenh chi duoc sang 1/5 chu ky  ->  1023 / 5 = 205.
 *
 *  Dat 0 de tat gioi han (khi da dung 6 bo nguon rieng, moi IC mot bo). */
#define ADB_WIDTH_LIMIT  205u

#define MODE_OFF        0   /* tat het                                       */
#define MODE_STATIC     1   /* bang do sang mac dinh                         */
#define MODE_LOWBEAM    2   /* chi chuoi A + B                               */
#define MODE_ADB_SWEEP  3   /* vung toi ADB chay ngang                       */
#define MODE_ALL_50     4   /* tat ca 50% - dung kiem tra phan cung          */
#define MODE_COUNT      5

static uint8_t  s_mode        = MODE_STATIC;
static uint8_t  s_modeApplied = 0xFF;
static uint8_t  s_commsOk     = 0;
static uint8_t  s_sweepPos    = 1;
static uint32_t s_lastSweepMs = 0;
static uint32_t s_lastPollMs  = 0;

/* Ket qua chan doan 6 IC, doc duoc qua debugger hoac gui len USB */
TPS_DevInfo  g_devInfo[TPS_DEV_COUNT];
uint8_t      g_initError = TPS_OK;

/* Bo dem thoi gian 1 ms do TIM3 tang (TIM3 dat chu ky 1 ms) */
volatile uint32_t g_msTick = 0;

/* ========================================================================== */
/*  Ham tuong thich nguoc                                                    */
/* ========================================================================== */

/* stm32f10x_it.c goi ham nay trong ngat TIM3 */
uint8_t Keyin_key(void)
{
    uint8_t key = 0;

    /* Phim tac dong muc thap (co dien tro keo len ben trong) */
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0) { key |= Key_Left;  }
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0) { key |= Key_Enter; }

    return key;
}

/* stm32f10x_it.c khai bao extern ham nay */
void c_fading_process(void)
{
}

/* Giu lai de tuong thich: cac module cu goi crc_16_ibm. Nay uy quyen cho
   ham CRC da duoc kiem chung trong driver. */
uint16_t crc_16_ibm(uint8_t *buf, uint8_t len)
{
    return TPS_Crc16Ibm(buf, (uint16_t)len);
}

void ClearRxBuf(void)
{
    USART1_HaveMes = 0;
    USART1_RxCnt   = 0;
    USART2_HaveMes = 0;
    USART2_RxCnt   = 0;
    Gu8DmaComplete = 0;

    memset(Pcdata,        0, sizeof(Pcdata));
    memset(USART1_Rx_Buf, 0, sizeof(USART1_Rx_Buf));
    memset(USART2_Rx_Buf, 0, sizeof(USART2_Rx_Buf));
}

uint8_t reverse_byte(uint8_t u8Data)
{
    u8Data = (uint8_t)(((u8Data & 0xf0) >> 4) | ((u8Data & 0x0f) << 4));
    u8Data = (uint8_t)(((u8Data & 0xcc) >> 2) | ((u8Data & 0x33) << 2));
    u8Data = (uint8_t)(((u8Data & 0xaa) >> 1) | ((u8Data & 0x55) << 1));
    return u8Data;
}

void Delay_us(uint16_t u16Data)
{
    TPS_DelayUs(u16Data);
}

/* ========================================================================== */
/*  Cau hinh ngoai vi cua board                                              */
/* ========================================================================== */

/*  LUU Y QUAN TRONG: KHONG duoc goi I2C_GPIO_Init() trong chuong trinh nay.
 *  Ham do cau hinh PB6 / PB7 thanh SCL / SDA, ma tren board Lumissil EB-V01C
 *  hai chan nay chinh la TXD1 / RXD1 di toi bo thu phat CAN TJA1042.
 */
static void Board_GpioInit(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    /* KEY1 = PB0, KEY2 = PB1, dau vao co dien tro keo len */
    gpio.GPIO_Pin  = GPIO_Pin_0 | GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio);

    /* Den bao trang thai PC13 */
    gpio.GPIO_Pin   = GPIO_Pin_13;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gpio);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/*  TIM3 chay chu ky 1 ms de lam moc thoi gian va quet phim.
 *  Trinh phuc vu ngat nam trong stm32f10x_it.c. */
static void Tim3_Init_1ms(void)
{
    TIM_TimeBaseInitTypeDef tb;
    NVIC_InitTypeDef        nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    TIM_DeInit(TIM3);

    /* 72 MHz / 72 / 1000 = 1 kHz  ->  1 ms */
    tb.TIM_Period        = 1000 - 1;
    tb.TIM_Prescaler     = 72 - 1;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &tb);

    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel                   = TIM3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 1;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);

    TIM_Cmd(TIM3, ENABLE);
}

/* ========================================================================== */
/*  Logic hien thi                                                           */
/* ========================================================================== */

static void Adb_ApplyMode(uint8_t mode)
{
    switch (mode)
    {
    case MODE_OFF:
        TPS_SetWidthAll(0);
        break;

    case MODE_STATIC:
        ADB_LoadDefault();
        break;

    case MODE_LOWBEAM:
        ADB_LoadDefault();
        ADB_SetStrings(ADB_MASK_AB);      /* chi giu chuoi A va B            */
        break;

    case MODE_ADB_SWEEP:
        ADB_LoadDefault();
        /* Vung toi rong 4 vi tri LED, chay tu LED 1 den LED 26 */
        ADB_SetDarkWindow(s_sweepPos, (uint8_t)(s_sweepPos + 3));
        break;

    case MODE_ALL_50:
        TPS_SetWidthAll(TPS_WIDTH_MAX / 2);
        break;

    default:
        TPS_SetWidthAll(0);
        break;
    }

    /*  Gioi han do rong xung theo ngan sach dien ap cua bo nguon.
     *
     *  Dat o day, SAU phan switch, de ap dung cho MOI che do. Ban truoc chi
     *  gioi han rieng MODE_STATIC, nen chuyen sang MODE_ALL_50 la dat 50%
     *  cho ca 64 kenh cua ANODE#1, tuc 32 LED sang dong thoi, khoang 96V,
     *  vuot xa muc duoi 40V ma board nguon cho duoc. Khi do bo nguon roi
     *  khoi vong dieu chinh va den tat het thay vi sang hon. */
    if (ADB_WIDTH_LIMIT > 0u) { ADB_ClampWidth(ADB_WIDTH_LIMIT); }

    (void)TPS_FlushAll();
    /* Chi bao tot khi ca 6 IC deu nhan duoc du lieu, xem TPS_FlushFailMask */
    s_commsOk = (TPS_FlushFailMask == 0) ? 1 : 0;
}

/*  Doc trang thai va ma loi cua ca 6 IC. Ket qua nam trong g_devInfo[] de
 *  xem bang debugger. Cach nay thay the viec do dien tro tinh: dien tro
 *  ANODE-CATHODE phu thuoc hoan toan vao trang thai switch nen khong the
 *  ket luan hong hoc tu phep do tinh. */
static void Adb_RunDiagnostics(void)
{
    uint8_t d;

    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        TPS_ReadDevInfo(d, &g_devInfo[d]);
    }
}

static uint8_t Adb_Startup(void)
{
    g_initError = TPS_Init();

    /*  LUON nap bang do sang, ke ca khi TPS_Init() bao loi.
     *
     *  Ban truoc bo qua buoc nay khi co loi, nen chi mot IC tra loi cham la
     *  toan bo den tat. Trong khi do lenh ghi WIDTH van co the toi duoc cac
     *  IC. Nap bang do sang la viec khong gay hai: con nao khong nhan duoc
     *  thi giu nguyen trang thai cu, con nao nhan duoc thi sang dung.
     *
     *  Bang do sang phai nap SAU TPS_Init() vi ham do xoa trang bo dem. */
    Adb_RunDiagnostics();
    Adb_ApplyMode(s_mode);
    s_modeApplied = s_mode;

    /*  Den bao chi xanh khi CA 6 IC deu nhan duoc du lieu WIDTH. Truoc day
     *  den bao xanh ngay ca khi 5 trong 6 con dang toi, lam nguoi dung tuong
     *  moi thu binh thuong. */
    s_commsOk = (TPS_FlushFailMask == 0) ? 1 : 0;

    return g_initError;
}

/* ========================================================================== */
/*  Chuong trinh chinh                                                       */
/* ========================================================================== */
int main(void)
{
    uint32_t now;
    uint32_t ledToggleMs = 0;
    uint8_t  needReinit  = 0;

    Set_System();                 /* clock he thong + chan ngat USB           */
    USB_Interrupts_Config();
    Set_USBClock();
    USB_Init();

    Board_GpioInit();
    Tim3_Init_1ms();

    Diag_Init();                  /* bo dem bao cao, gui ve may qua USB HID   */

    TPS_UartInit();
    TPS_DelayMs(100);             /* cho module len nguon va chot dia chi     */

    Adb_Startup();

    /*  Bao cao chan doan ngay sau khi khoi tao. In ra USART2 nen khong anh
     *  huong gi toi bus noi voi module. Chay mot lan luc khoi dong, va chay
     *  lai moi khi bam KEY2. */
    Diag_RunFullReport();
    /* Bao cao co ghi thu vao WIDTH01H nen phai nap lai bang do sang */
    Adb_ApplyMode(s_mode);

    while (1)
    {
        now = g_msTick;

        /* ---- Phim KEY1: doi che do ---------------------------------- */
        if (B_Key_Left)
        {
            B_Key_Left = 0;
            s_mode = (uint8_t)((s_mode + 1) % MODE_COUNT);
            s_modeApplied = 0xFF;
            s_sweepPos = 1;
        }

        /* ---- Phim KEY2: tat het va chan doan lai --------------------- */
        if (B_Key_Enter)
        {
            B_Key_Enter = 0;
            Adb_RunDiagnostics();
            /* In lai bao cao day du ra USART2 roi nap lai che do hien tai */
            Diag_RunFullReport();
            s_modeApplied = 0xFF;
        }

        /* ---- Doi che do --------------------------------------------- */
        if (s_modeApplied != s_mode)
        {
            Adb_ApplyMode(s_mode);
            s_modeApplied = s_mode;
        }

        /* ---- Hieu ung ADB chay ngang, cap nhat moi 80 ms ------------- */
        if (s_mode == MODE_ADB_SWEEP && (now - s_lastSweepMs) >= 80u)
        {
            s_lastSweepMs = now;
            s_sweepPos++;
            if (s_sweepPos > ADB_LED_COUNT) { s_sweepPos = 1; }
            Adb_ApplyMode(MODE_ADB_SWEEP);
        }

        /* ---- Giam sat module moi 500 ms ------------------------------ */
        if ((now - s_lastPollMs) >= 500u)
        {
            s_lastPollMs = now;

            if (TPS_CheckPowerCycle(&needReinit) != TPS_OK) { s_commsOk = 0; }
            else                                            { s_commsOk = 1; }

            if (needReinit)
            {
                /* Module vua bi cup nguon: toan bo thanh ghi volatile da mat,
                   cac IC dang o Fail-Safe. Phai khoi tao lai tu dau. */
                if (Adb_Startup() == TPS_OK) { s_commsOk = 1; }
            }
        }

        /* ---- Den bao trang thai -------------------------------------- */
        if (s_commsOk)
        {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13);        /* sang lien tuc      */
        }
        else if ((now - ledToggleMs) >= 100u)
        {
            ledToggleMs = now;
            GPIO_WriteBit(GPIOC, GPIO_Pin_13,
                (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13)));
        }

        /* ---- Goi tin USB HID tu phan mem tren may tinh ---------------- */
        if (USB_Received_Flag)
        {
            USB_Received_Flag = 0;
            USB_GetData(data, 64);

            /*  Lenh cua phan chan doan: may tinh xin ban bao cao, hoac yeu
             *  cau chay lai roi gui. Xem tps_diag.h. */
            if (Diag_HandleUsbCommand(data))
            {
                /*  Phep thu co ghi vao thanh ghi WIDTH nen phai nap lai bang
                 *  do sang. Rieng lenh BCAST thi KHONG nap lai, vi muc dich
                 *  cua no la giu den sang de nguoi dung quan sat. */
                if (data[0] == DIAG_CMD_RERUN ||
                    data[0] == DIAG_CMD_RESTORE)
                {
                    Adb_ApplyMode(s_mode);
                }
            }
        }
    }
}
