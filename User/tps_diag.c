/**
  ******************************************************************************
  * @file    tps_diag.c
  * @brief   Bao cao chan doan chuoi TPS9266x, gui ve may tinh qua USB HID
  ******************************************************************************
  */
#include "tps_diag.h"
#include "tps9266x.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usbio.h"
#include <math.h>

/* ------------------------------------------------------------------------- */
/*  Cam bien nhiet NTC tren module                                           */
/*                                                                           */
/*  Mach (schematic SSL_96pixel_ADB MD_07012026_R00, trang 1 va 2):          */
/*                                                                           */
/*      VDK1 --[R303 6K81]--+--[R306 = NTC 10K]-- GND   -> ADC1 (chan 9)     */
/*                          |                              diem do TP376     */
/*                       [C400 10nF]                                         */
/*                                                                           */
/*      VDK1 --[R313 6K81]--+--[R314 6K81]------- GND   -> ADC2              */
/*                          |                                                */
/*                       [C319 10nF]                                         */
/*                                                                           */
/*  R306 la NTC, dat ngay canh chuoi LED nen no do nhiet VUNG DEN.           */
/*                                                                           */
/*  Diem mau chot: R313 = R314 nen ADC2 LUON bang dung mot nua VDK1. Lay ty  */
/*  so ADC1/ADC2 thi VDK1, dien ap tham chieu cua ADC va ca sai so khuech    */
/*  dai deu triet tieu. Khong can biet VDK1 bang bao nhieu.                  */
/*                                                                           */
/*      x     = adc1 / (2 * adc2)   = R_ntc / (R303 + R_ntc)                 */
/*      R_ntc = R303 * adc1 / (2 * adc2 - adc1)                              */
/*                                                                           */
/*  Quy doi ra nhiet do theo phuong trinh he so B:                           */
/*                                                                           */
/*      1/T = 1/T25 + ln(R_ntc / R25) / B                                    */
/*                                                                           */
/*  So lieu NTC lay tu trang san pham TDK cho dung ma B57332V5103F360:       */
/*      R25 = 10 kOhm +-1%,  B25/50 = 3380 K,  B25/85 = 3435 K,              */
/*      B25/100 = 3455 K,  dai lam viec -40..150 do C, AEC-Q200.             */
/*  Chon B25/100 vi vung nhiet dang quan tam cua tam den nam quanh 25..120.  */
/* ------------------------------------------------------------------------- */
#define NTC_R25      10000.0f   /* dien tro NTC o 25 do C, Ohm              */
#define NTC_BETA      3455.0f   /* he so B25/100, K                          */
#define NTC_R_TOP     6810.0f   /* R303, Ohm                                 */
#define NTC_T25        298.15f  /* 25 do C tinh ra Kelvin                    */

/*  Tra ve nhiet do theo don vi 0,1 do C. Tra ve NTC_TEMP_INVALID neu so doc
 *  khong hop le (NTC dut, chap, hoac ADC chua lay mau).                     */
#define NTC_TEMP_INVALID  (-32768)

static int16_t NtcTempDeci(uint8_t adc1, uint8_t adc2)
{
    float denom, rNtc, invT, tC;

    if (adc2 == 0u) { return NTC_TEMP_INVALID; }

    /*  Mau so 2*adc2 - adc1 tien toi 0 khi NTC hoa ra ho mach (dien tro vo
     *  cung lon). Chan lai de khong chia cho 0.                            */
    denom = 2.0f * (float)adc2 - (float)adc1;
    if (denom < 1.0f || adc1 == 0u) { return NTC_TEMP_INVALID; }

    rNtc = NTC_R_TOP * (float)adc1 / denom;
    if (rNtc < 1.0f) { return NTC_TEMP_INVALID; }

    invT = 1.0f / NTC_T25 + logf(rNtc / NTC_R25) / NTC_BETA;
    if (invT <= 0.0f) { return NTC_TEMP_INVALID; }

    tC = 1.0f / invT - 273.15f;
    if (tC < -60.0f || tC > 200.0f) { return NTC_TEMP_INVALID; }

    return (int16_t)(tC * 10.0f + (tC >= 0.0f ? 0.5f : -0.5f));
}

/*  In mot so co mot chu so thap phan, vi du 42.7 */
static void DiagDeci(int16_t deci)
{
    if (deci < 0) { Diag_Puts("-"); deci = (int16_t)(-deci); }
    Diag_Dec((uint32_t)(deci / 10));
    Diag_Puts(".");
    Diag_Dec((uint32_t)(deci % 10));
}

/* ------------------------------------------------------------------------- */
/*  Bo dem chua ban bao cao                                                  */
/* ------------------------------------------------------------------------- */
static char     s_text[DIAG_TEXT_MAX + 1];
static uint16_t s_len;

/*  Co bao goi IN truoc da duoc may tinh lay. usb_endp.c dat lai co nay
 *  trong EP2_IN_Callback. */
extern volatile uint8_t USB_TxDone;

void Diag_Init(void)
{
    s_len     = 0;
    s_text[0] = '\0';
}

/* ------------------------------------------------------------------------- */
/*  Ghi vao bo dem                                                           */
/* ------------------------------------------------------------------------- */
/*  Bang 1 khi bo dem da day va co ky tu bi bo di. Diag_RunFullReport doc
 *  co nay o cuoi de noi ro cho nguoi dung biet ban bao cao bi cat.       */
static uint8_t s_truncated;

static void DiagPutc(char c)
{
    if (s_len < DIAG_TEXT_MAX)
    {
        s_text[s_len] = c;
        s_len++;
        s_text[s_len] = '\0';
    }
    else
    {
        s_truncated = 1u;
    }
}

void Diag_Puts(const char *s)
{
    if (s == 0) { return; }
    while (*s) { DiagPutc(*s++); }
}

void Diag_Nl(void)
{
    DiagPutc('\r');
    DiagPutc('\n');
}

void Diag_Hex8(uint8_t v)
{
    static const char hexTab[] = "0123456789ABCDEF";
    DiagPutc(hexTab[(v >> 4) & 0x0F]);
    DiagPutc(hexTab[v & 0x0F]);
}

void Diag_Dec(uint32_t v)
{
    char    buf[11];
    uint8_t n = 0;

    if (v == 0u) { DiagPutc('0'); return; }
    while (v > 0u && n < sizeof(buf))
    {
        buf[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n > 0u) { DiagPutc(buf[--n]); }
}

/* ------------------------------------------------------------------------- */
/*  Gui qua USB HID                                                          */
/* ------------------------------------------------------------------------- */

/*  Gui mot goi 64 byte va cho may tinh lay xong.
 *  Diem quan trong: diem cuoi IN chi co MOT bo dem. Neu gui goi tiep theo
 *  khi may tinh chua kip lay goi truoc thi goi truoc bi ghi de va mat. Vi
 *  vay phai cho co USB_TxDone. */
static uint8_t DiagSendChunk(uint8_t *report)
{
    uint32_t guard = 2000000u;      /* khoang vai chuc ms */

    while (USB_TxDone == 0u)
    {
        if (guard-- == 0u) { return 0u; }   /* may tinh khong doc, bo cuoc */
    }

    USB_TxDone = 0u;
    USB_SendData(report, DIAG_REPORT_SIZE);
    return 1u;
}

uint8_t Diag_SendOverUsb(void)
{
    uint8_t  report[DIAG_REPORT_SIZE];
    uint16_t offset = 0;
    uint8_t  total, index, i, n;

    if (bDeviceState != CONFIGURED) { return 0u; }   /* chua cam vao may */

    /* So goi can gui, lam tron len. It nhat mot goi ke ca khi rong. */
    total = (uint8_t)((s_len + DIAG_CHUNK_TEXT - 1u) / DIAG_CHUNK_TEXT);
    if (total == 0u) { total = 1u; }

    for (index = 0; index < total; index++)
    {
        n = (uint8_t)((s_len - offset) > DIAG_CHUNK_TEXT
                      ? DIAG_CHUNK_TEXT
                      : (s_len - offset));

        report[0] = DIAG_TAG;
        report[1] = index;
        report[2] = total;
        report[3] = n;

        for (i = 0; i < DIAG_CHUNK_TEXT; i++)
        {
            report[DIAG_CHUNK_HDR + i] = (i < n)
                                       ? (uint8_t)s_text[offset + i]
                                       : 0x00u;
        }

        if (DiagSendChunk(report) == 0u) { return 0u; }
        offset = (uint16_t)(offset + n);
    }

    return 1u;
}

uint8_t Diag_HandleUsbCommand(const uint8_t *report)
{
    if (report == 0) { return 0u; }

    if (report[0] == DIAG_CMD_REQUEST)
    {
        (void)Diag_SendOverUsb();
        return 1u;
    }

    if (report[0] == DIAG_CMD_RERUN)
    {
        Diag_RunFullReport();
        (void)Diag_SendOverUsb();
        return 1u;
    }

    if (report[0] == DIAG_CMD_BCAST)
    {
        uint8_t err;

        /*  Bat sang moi kenh cua MOI IC bang mot lenh broadcast duy nhat.
         *  Khong IC nao phai tra loi, chi can nghe duoc la den sang. */
        s_len = 0; s_text[0] = '\0';

        Diag_Puts("=========================================================");
        Diag_Nl();
        Diag_Puts(" PHEP THU BROADCAST - HAY NHIN VAO DEN");
        Diag_Nl();
        Diag_Puts("=========================================================");
        Diag_Nl();

        TPS_CommsReset();
        TPS_DelayMs(2);
        err = TPS_BroadcastWidthAll(TPS_WIDTH_MAX / 2u);

        Diag_Puts(" Da gui lenh broadcast dat moi kenh ve 50%: ");
        Diag_Puts((err == TPS_OK) ? "gui xong" : "gui that bai");
        Diag_Nl();
        Diag_Nl();
        Diag_Puts(" Y NGHIA CUA DIEU BAN NHIN THAY:");
        Diag_Nl();
        Diag_Puts("  - Neu bay gio CA SAU nhom den deu sang, thi ba IC im lang");
        Diag_Nl();
        Diag_Puts("    van NGHE duoc lenh, chi khong NOI duoc. Loi nam o duong");
        Diag_Nl();
        Diag_Puts("    TX cua chung hoac o bit ACKEN, khong phai chip chet.");
        Diag_Nl();
        Diag_Puts("  - Neu van chi ba nhom sang nhu cu, thi ba IC kia that su");
        Diag_Nl();
        Diag_Puts("    khong nhan duoc gi: mat nguon VDD, hoac thieu clock LVDS,");
        Diag_Nl();
        Diag_Puts("    hoac duong RX toi chung bi dut. Phai do bang dong ho.");
        Diag_Nl();
        Diag_Nl();
        Diag_Puts(" Chay lai voi -Restore de tra ve bang do sang binh thuong.");
        Diag_Nl();

        (void)Diag_SendOverUsb();
        return 1u;
    }

    if (report[0] == DIAG_CMD_ALL_LEVEL)
    {
        /*  Dat MOI kenh cua MOI IC ve cung mot muc sang, roi bao cao xem
         *  muc do co nam trong ngan sach dien ap cua bo nguon hay khong.
         *
         *  Muc dich: cho phep do dan tim muc sang toi da dung duoc tren
         *  phan cung thuc te, thay vi tin vao mot con so tinh tren giay.
         *
         *  CO Y khong ap dung ADB_ClampWidth o day: nguoi dung can thu
         *  duoc ca nhung muc vuot ngan sach de thay gioi han that nam o
         *  dau. Vuot qua thi bo nguon roi khoi vong dieu chinh va den toi
         *  di, khong gay hong hoc. */
        uint16_t w;
        uint8_t  d, err, okMask = 0;
        uint32_t simA1, simA23, vA1, vA23;

        w = (uint16_t)(report[1] | ((uint16_t)report[2] << 8));
        if (w > TPS_WIDTH_MAX) { w = TPS_WIDTH_MAX; }

        s_len = 0; s_text[0] = '\0';

        TPS_SetWidthAll(w);

        for (d = 0; d < TPS_DEV_COUNT; d++)
        {
            TPS_CommsReset(); TPS_DelayMs(2);
            err = TPS_FlushWidth(d);
            if (err == TPS_OK) { okMask |= (uint8_t)(1u << d); }
        }

        /*  So LED sang DONG THOI = so kenh x ty le duty.
         *  ANODE#1 co 4 IC noi tiep = 64 kenh; ANODE#2 va #3 moi duong
         *  chi 1 IC = 16 kenh. Dien ap uoc tinh theo LED ~3V. */
        simA1  = (64u * (uint32_t)w) / 1024u;
        simA23 = (16u * (uint32_t)w) / 1024u;
        vA1    = simA1  * 3u;
        vA23   = simA23 * 3u;

        Diag_Puts("=========================================================");
        Diag_Nl();
        Diag_Puts(" DAT MOI KENH VE CUNG MOT MUC SANG");
        Diag_Nl();
        Diag_Puts("=========================================================");
        Diag_Nl();
        Diag_Puts(" WIDTH = "); Diag_Dec((uint32_t)w);
        Diag_Puts(" / 1023   (duty ");
        Diag_Dec(((uint32_t)w * 100u) / 1024u);
        Diag_Puts("%)");
        Diag_Nl();
        Diag_Puts(" Da gui thanh cong toi cac IC (mat na): 0x");
        Diag_Hex8(okMask);
        Diag_Nl();
        Diag_Nl();

        Diag_Puts(" UOC TINH TAI DIEN (LED ~3V, ngan sach ~40V):");
        Diag_Nl();
        Diag_Puts("   ANODE#1  64 kenh: ");
        Diag_Dec(simA1);  Diag_Puts(" LED dong thoi = ~");
        Diag_Dec(vA1);    Diag_Puts("V  ");
        Diag_Puts((vA1 <= 40u) ? "TRONG ngan sach" : "VUOT ngan sach!");
        Diag_Nl();
        Diag_Puts("   ANODE#2  16 kenh: ");
        Diag_Dec(simA23); Diag_Puts(" LED dong thoi = ~");
        Diag_Dec(vA23);   Diag_Puts("V  ");
        Diag_Puts((vA23 <= 40u) ? "TRONG ngan sach" : "VUOT ngan sach!");
        Diag_Nl();
        Diag_Puts("   ANODE#3  16 kenh: nhu ANODE#2");
        Diag_Nl();
        Diag_Nl();
        Diag_Puts(" Con so tren chi dung khi cac kenh DA duoc rai deu pha.");
        Diag_Nl();
        Diag_Puts(" Tang dan muc sang cho toi khi den bat dau toi di, do la");
        Diag_Nl();
        Diag_Puts(" gioi han that cua bo nguon tren phan cung nay.");
        Diag_Nl();

        (void)Diag_SendOverUsb();
        return 1u;
    }

    if (report[0] == DIAG_CMD_ONLY_IC)
    {
        /*  Chi bat sang MOT IC, tat het cac con khac.
         *
         *  Muc dich: xac dinh nhom den nao tren module thuoc ve IC nao. Moi
         *  phan tich truoc do deu dua tren mot anh xa chua ai kiem chung, va
         *  anh xa sai thi ket luan sai theo. */
        uint8_t  target = report[1];
        uint8_t  d, err, okMask = 0;
        uint16_t level;

        /*  report[2..3] = muc sang 0..1023 (byte thap truoc). Neu may tinh
         *  khong gui thi ca hai bang 0, khi do dung mac dinh 50%. */
        level = (uint16_t)(report[2] | ((uint16_t)report[3] << 8));
        if (level == 0u) { level = TPS_WIDTH_MAX / 2u; }
        if (level > TPS_WIDTH_MAX) { level = TPS_WIDTH_MAX; }

        s_len = 0; s_text[0] = '\0';

        if (target > 15u)
        {
            Diag_Puts(" Dia chi khong hop le, phai tu 0 den 15.");
            Diag_Nl();
            (void)Diag_SendOverUsb();
            return 1u;
        }

        /*  Dia chi ngoai dai 0..5: day la IC bi ho chan ADDRx. Bo dem WIDTH
         *  chi co 6 phan tu nen phai ghi thang bang duong rieng. */
        if (target >= TPS_DEV_COUNT)
        {
            uint8_t e2;

            Diag_Puts("=========================================================");
            Diag_Nl();
            Diag_Puts(" CHI BAT SANG DIA CHI "); Diag_Dec((uint32_t)target);
            Diag_Puts("  (ngoai dai binh thuong)");
            Diag_Nl();
            Diag_Puts("=========================================================");
            Diag_Nl();

            /* Tat het 6 dia chi thuong truoc */
            for (d = 0; d < TPS_DEV_COUNT; d++)
            {
                TPS_CommsReset(); TPS_DelayMs(2);
                (void)TPS_WriteWidthDirect(d, 0u);
            }

            TPS_CommsReset(); TPS_DelayMs(2);
            e2 = TPS_WriteWidthDirect(target, level);

            Diag_Puts(" Ghi WIDTH toi dia chi "); Diag_Dec((uint32_t)target);
            Diag_Puts(": "); Diag_Puts((e2 == TPS_OK) ? "THANH CONG" : "that bai");
            Diag_Nl();
            Diag_Nl();
            Diag_Puts(" HAY NHIN DEN. Nhom nao sang chinh la IC bi ho chan ADDRx.");
            Diag_Nl();
            (void)Diag_SendOverUsb();
            return 1u;
        }

        Diag_Puts("=========================================================");
        Diag_Nl();
        Diag_Puts(" CHI BAT SANG IC30"); Diag_Dec((uint32_t)(target + 1));
        Diag_Puts("   (dia chi "); Diag_Dec((uint32_t)target); Diag_Puts(")");
        Diag_Nl();
        Diag_Puts("=========================================================");
        Diag_Nl();

        /* Dat bo dem: con duoc chon sang 50%, cac con khac tat han */
        for (d = 0; d < TPS_DEV_COUNT; d++)
        {
            uint8_t c;
            for (c = 0; c < TPS_CH_PER_DEV; c++)
            {
                TPS_SetWidth(d, c, (d == target) ? level : 0u);
            }
        }

        for (d = 0; d < TPS_DEV_COUNT; d++)
        {
            TPS_CommsReset();
            TPS_DelayMs(2);
            err = TPS_FlushWidth(d);
            if (err == TPS_OK) { okMask |= (uint8_t)(1u << d); }
        }

        Diag_Puts(" Da gui du lieu thanh cong toi cac IC (mat na): 0x");
        Diag_Hex8(okMask);
        Diag_Nl();
        Diag_Puts(" IC30"); Diag_Dec((uint32_t)(target + 1));
        Diag_Puts(" nhan duoc lenh: ");
        Diag_Puts((okMask & (1u << target)) ? "CO" : "KHONG");
        Diag_Nl();
        Diag_Puts(" Muc sang dat: "); Diag_Dec((uint32_t)level);
        Diag_Puts(" / 1023"); Diag_Nl();
        Diag_Nl();
        /* ----------------------------------------------------------------
         *  Doc mach phat hien HO LED nam san trong chip.
         *
         *  Day la phep do DIEN thuc su, khong phai suy doan: chip tu kiem
         *  tra xem khi switch MO thi dong co chay qua LED hay khong.
         *
         *    - Bao HO LED   -> khong co dong. Duong anode chua cap nguon,
         *                      hoac bo nguon khong du dien ap.
         *    - Khong bao gi -> dong DANG chay, LED thuc su dang sang.
         * ---------------------------------------------------------------- */
        {
            uint8_t fl = 0, fh = 0, st2 = 0;

            TPS_DelayMs(50);            /* cho vai chu ky PWM de mach do kip */
            TPS_CommsReset(); TPS_DelayMs(2);

            Diag_Nl();
            Diag_Puts(" KIEM TRA DONG DIEN QUA LED (mach trong chip):");
            Diag_Nl();

            if (TPS_ReadReg8(target, TPS_REG_FAULT_OPEN_L, &fl) == TPS_OK &&
                TPS_ReadReg8(target, TPS_REG_FAULT_OPEN_H, &fh) == TPS_OK)
            {
                Diag_Puts("   HO LED kenh 1-8: 0x");  Diag_Hex8(fl);
                Diag_Puts("   kenh 9-16: 0x");        Diag_Hex8(fh);
                Diag_Nl();

                if (fl == 0u && fh == 0u)
                {
                    Diag_Puts("   => KHONG kenh nao ho: dong DANG chay qua LED,");
                    Diag_Nl();
                    Diag_Puts("      tuc den cua con nay thuc su dang sang.");
                }
                else
                {
                    Diag_Puts("   => CO kenh bao HO: KHONG co dong qua LED.");
                    Diag_Nl();
                    Diag_Puts("      Hoac duong anode cua con nay chua cap nguon,");
                    Diag_Nl();
                    Diag_Puts("      hoac bo nguon khong du dien ap de dan LED.");
                }
            }
            else { Diag_Puts("   khong doc duoc thanh ghi loi"); }
            Diag_Nl();

            if (TPS_ReadReg8(target, TPS_REG_STATUS, &st2) == TPS_OK)
            {
                Diag_Puts("   STATUS=0x"); Diag_Hex8(st2);
                if (st2 & TPS_STATUS_VLED_ERR) { Diag_Puts("  VLED_ERR: dien ap LED ngoai dai"); }
                Diag_Nl();
            }
        }

        Diag_Puts(" HAY NHIN DEN VA GHI LAI nhom nao dang sang.");
        Diag_Nl();
        Diag_Puts(" Chay lan luot voi -OnlyIc 0 den -OnlyIc 5 de lap bang");
        Diag_Nl();
        Diag_Puts(" doi chieu giua nhan tren module va so hieu IC.");
        Diag_Nl();

        (void)Diag_SendOverUsb();
        return 1u;
    }

    if (report[0] == DIAG_CMD_ONLY_STRING)
    {
        uint8_t  target = report[1];
        uint8_t  ch     = report[2];
        uint8_t  err;
        uint16_t level;

        level = (uint16_t)(report[3] | ((uint16_t)report[4] << 8));
        if (level > TPS_WIDTH_MAX) { level = TPS_WIDTH_MAX; }

        s_len = 0; s_text[0] = '\0';

        if (target >= TPS_DEV_COUNT)
        {
            Diag_Puts(" Dia chi IC khong hop le."); Diag_Nl();
            (void)Diag_SendOverUsb(); return 1u;
        }
        if (ch >= TPS_CH_PER_DEV)
        {
            Diag_Puts(" Dia chi kenh khong hop le."); Diag_Nl();
            (void)Diag_SendOverUsb(); return 1u;
        }

        /* Cap nhat duy nhat pixel muc tieu, giu nguyen cac pixel khac */
        TPS_SetWidth(target, ch, level);

        /* Xa du lieu xuong chip muc tieu */
        TPS_CommsReset();
        TPS_DelayMs(2);
        err = TPS_FlushWidth(target);

        Diag_Puts(" Update pixel IC30"); Diag_Dec((uint32_t)(target + 1));
        Diag_Puts(" CH"); Diag_Dec((uint32_t)(ch + 1));
        Diag_Puts(" = "); Diag_Dec((uint32_t)level);
        Diag_Puts(err == TPS_OK ? " OK" : " FAIL");
        Diag_Nl();

        (void)Diag_SendOverUsb();
    }

    if (report[0] == DIAG_CMD_IC_LEVEL)
    {
        /*  Dat ca 16 kenh cua MOT IC ve mot muc sang, GIU NGUYEN cac IC khac.
         *
         *  Khac voi ONLY_IC (0xD5) la lenh loai tru - no tat het nhung con
         *  con lai. Lenh nay chi ghi len dung con duoc chi dinh, nho vay may
         *  tinh dat duoc moi IC mot muc sang rieng va ca sau con cung sang
         *  theo muc cua minh.                                              */
        uint8_t  target = report[1];
        uint8_t  ch, err;
        uint16_t level  = (uint16_t)(report[2] | ((uint16_t)report[3] << 8));

        s_len = 0; s_text[0] = '\0';

        if (target >= TPS_DEV_COUNT)
        {
            Diag_Puts(" Dia chi IC khong hop le, phai tu 0 den 5."); Diag_Nl();
            (void)Diag_SendOverUsb();
            return 1u;
        }
        if (level > TPS_WIDTH_MAX) { level = TPS_WIDTH_MAX; }

        for (ch = 0; ch < TPS_CH_PER_DEV; ch++) { TPS_SetWidth(target, ch, level); }

        TPS_CommsReset();
        TPS_DelayMs(2);
        err = TPS_FlushWidth(target);

        Diag_Puts(" IC30"); Diag_Dec((uint32_t)(target + 1));
        Diag_Puts(" dat 16 kenh = "); Diag_Dec((uint32_t)level);
        Diag_Puts(err == TPS_OK ? "  OK" : "  THAT BAI");
        Diag_Nl();

        (void)Diag_SendOverUsb();
        return 1u;
    }

    if (report[0] == DIAG_CMD_ALL_LEVEL)
    {
        uint16_t w = (uint16_t)(report[1] | ((uint16_t)report[2] << 8));
        s_len = 0; s_text[0] = '\0';
        if (w > TPS_WIDTH_MAX) { w = TPS_WIDTH_MAX; }
        
        TPS_SetWidthAll(w);
        TPS_CommsReset();
        TPS_DelayMs(2);
        (void)TPS_FlushAll();
        
        Diag_Puts(" Set all channels to "); Diag_Dec((uint32_t)w); Diag_Nl();
        (void)Diag_SendOverUsb();
        return 1u;
    }

    if (report[0] == DIAG_CMD_RESTORE)
    {
        s_len = 0; s_text[0] = '\0';
        Diag_Puts(" Da yeu cau tra ve bang do sang binh thuong.");
        Diag_Nl();
        (void)Diag_SendOverUsb();
        return 1u;      /* main.c se goi Adb_ApplyMode() */
    }

    return 0u;
}

/* ------------------------------------------------------------------------- */
/*  Dien giai ket qua                                                        */
/* ------------------------------------------------------------------------- */

static const char *DiagErrName(uint8_t err)
{
    switch (err)
    {
    case TPS_OK:              return "OK        ";
    case TPS_ERR_PARAM:       return "THAM SO   ";
    case TPS_ERR_TX_TIMEOUT:  return "TX TIMEOUT";
    case TPS_ERR_ECHO:        return "MAT ECHO  ";
    case TPS_ERR_RX_TIMEOUT:  return "IM LANG   ";
    case TPS_ERR_CRC:         return "SAI CRC   ";
    case TPS_ERR_ACK:         return "KHONG ACK ";
    default:                  return "LA        ";
    }
}

static const char *DiagPartName(uint8_t icid)
{
    if (icid == TPS_ICID_92664) { return "TPS92664 (master)"; }
    if (icid == TPS_ICID_92667) { return "TPS92667 (slave)"; }
    return "khong nhan dang duoc";
}

/*  Giai thich thanh ghi STATUS theo bang 7-44 cua SLUSE18 */
static void DiagPrintStatus(uint8_t st)
{
    Diag_Puts("        STATUS=0x"); Diag_Hex8(st); Diag_Puts("  ->");
    if (st & 0x08) { Diag_Puts(" TRIM_ERR(IC hong, cam ghi)"); }
    if (st & 0x02) { Diag_Puts(" MTP_ERR(MTP trong, binh thuong voi thiet ke nay)"); }
    if (st & 0x10) { Diag_Puts(" LIMP_HOME"); }
    if (st & 0x20) { Diag_Puts(" VLED_ERR"); }
    if (st & 0x40) { Diag_Puts(" CHPMP_ERR(bom dien tich)"); }
    if (st & 0x80) { Diag_Puts(" PWM_ERR"); }
    if (st & 0x04) { Diag_Puts(" CANH BAO NHIET"); }
    if ((st & 0xFE) == 0) { Diag_Puts(" khong co bit loi nao"); }
    /*  Bit PWR nam rieng: no khong phai bit loi. PWR=0 nghia la IC da qua
     *  mot lan mat nguon KE TU lan firmware ghi PWR=1, tuc no chua duoc
     *  khoi tao lai. In sau cung de khong mau thuan voi cau tren. */
    if ((st & 0x01) == 0) { Diag_Puts(" | PWR=0: chua duoc khoi tao hoac vua mat nguon"); }
    else                  { Diag_Puts(" | PWR=1: da duoc khoi tao"); }
    Diag_Nl();
}

/* ------------------------------------------------------------------------- */

void Diag_RunFullReport(void)
{
    uint8_t d, err, icid, st, rd;
    uint8_t okRead, okWrite, okBack;
    const uint8_t testVal = 0x55;       /* gia tri thu, ghi vao WIDTH01H */

    /* Bat dau ban bao cao moi */
    s_len       = 0;
    s_text[0]   = '\0';
    s_truncated = 0u;

    Diag_Puts("=========================================================");
    Diag_Nl();
    Diag_Puts(" BAO CAO CHAN DOAN CHUOI TPS9266x");
    Diag_Nl();
    Diag_Puts(" FW v"); Diag_Dec((uint32_t)DIAG_FW_VERSION);
    Diag_Nl();
    Diag_Puts(" Tach bach duong GHI va duong DOC cua tung IC");
    Diag_Nl();
    Diag_Puts("=========================================================");
    Diag_Nl();

    Diag_Puts(" Mat na IC tra loi lenh doc: 0x");
    Diag_Hex8(TPS_DevOnline);
    Diag_Nl();
    Diag_Puts(" Mat na IC ghi WIDTH that bai: 0x");
    Diag_Hex8(TPS_FlushFailMask);
    Diag_Nl();

    Diag_Puts("---------------------------------------------------------");
    Diag_Nl();
    Diag_Puts(" NHIET DO");
    Diag_Nl();

    /*  Theo schematic SSL_96pixel_ADB MD_07012026_R00:
     *
     *      VDK1 --[R303]--+--[R306 = NTC]-- GND      -> IC301 chan 9  (ADC1)
     *                     |
     *                  [C400 loc]                       diem do: TP376
     *
     *      VDK1 --[R313]--+--[R314]-------- GND      -> IC301        (ADC2)
     *                     |
     *                  [C314 loc]
     *
     *  R306 la NTC va duoc dat ngay canh chuoi LED, nen day moi la nhiet do
     *  dang quan tam - nhiet vung den, khong phai nhiet trong long IC.
     *
     *  Hai chan ADC chi lay mau khi bit LEDADCEN trong SYSCFG duoc bat. Ham
     *  khoi tao binh thuong khong bat no vi viec dieu khien den khong can,
     *  nen o day bat tam roi tra lai nguyen trang, khong de lai anh huong
     *  gi len cau hinh dang chay.
     *
     *  Chi TPS92664 (IC301) co ADC. Cac con TPS92667 khong co.             */
    {
        uint8_t raw, sysOld = 0, sysSaved = 0;

        TPS_CommsReset();
        TPS_DelayMs(2);
        if (TPS_ReadReg8(TPS_ADDR_IC301, TPS_REG_SYSCFG, &sysOld) == TPS_OK)
        {
            sysSaved = 1;
            TPS_WriteReg8(TPS_ADDR_IC301, TPS_REG_SYSCFG,
                          (uint8_t)(sysOld | TPS_SYSCFG_LEDADCEN));
            TPS_DelayMs(5);          /* cho bo ADC lay xong mot vong mau */
        }
        else
        {
            Diag_Puts("  Khong doc duoc SYSCFG, ADC co the chua duoc bat.");
            Diag_Nl();
        }

        /*  Doc ca hai chan ADC roi quy doi. Phai doc ca hai vi phep tinh
         *  dua tren TY SO giua chung.                                      */
        {
            uint8_t a1 = 0, a2 = 0, ok1, ok2;

            TPS_CommsReset();
            TPS_DelayMs(2);
            ok1 = (TPS_ReadReg8(TPS_ADDR_IC301, TPS_REG_ADC1, &a1) == TPS_OK);

            TPS_CommsReset();
            TPS_DelayMs(2);
            ok2 = (TPS_ReadReg8(TPS_ADDR_IC301, TPS_REG_ADC2, &a2) == TPS_OK);

            Diag_Puts("  NTC R306 (canh chuoi LED): ");
            if (ok1 && ok2)
            {
                int16_t deci = NtcTempDeci(a1, a2);
                if (deci != NTC_TEMP_INVALID)
                {
                    float denom = 2.0f * (float)a2 - (float)a1;
                    uint32_t rNtc = (uint32_t)(NTC_R_TOP * (float)a1 / denom);
                    DiagDeci(deci); Diag_Puts(" do C   (R_ntc = ");
                    Diag_Dec(rNtc); Diag_Puts(" Ohm)");
                }
                else { Diag_Puts("so doc khong hop le - kiem tra NTC va cau phan ap"); }
            }
            else { Diag_Puts("khong doc duoc ADC"); }
            Diag_Nl();

            Diag_Puts("    ADC1=0x"); Diag_Hex8(a1);
            Diag_Puts("  ADC2=0x");   Diag_Hex8(a2);
            Diag_Puts("  (ADC2 la moc VDK1/2, dung de khu VDK1 khoi phep tinh)");
            Diag_Nl();
        }

        if (sysSaved)
        {
            TPS_CommsReset();
            TPS_DelayMs(2);
            TPS_WriteReg8(TPS_ADDR_IC301, TPS_REG_SYSCFG, sysOld);
        }

        /*  Nhiet do trong long IC301. Khac han NTC: day la nhiet cua ban
         *  than con chip, dung de biet chip co qua nong khong.             */
        TPS_CommsReset();
        TPS_DelayMs(2);
        Diag_Puts("  IC301 nhiet do trong IC (DIETEMP): ");
        if (TPS_ReadReg8(TPS_ADDR_IC301, TPS_REG_DIETEMP, &raw) == TPS_OK)
        {
            /*  T[degC] = 0.9098 * raw - 50  (SLUSE18 bang 7-54) */
            int32_t tC = ((int32_t)raw * 9098) / 10000 - 50;
            Diag_Puts("raw=0x"); Diag_Hex8(raw); Diag_Puts("  = ");
            if (tC < 0) { Diag_Puts("-"); tC = -tC; }
            Diag_Dec((uint32_t)tC); Diag_Puts(" do C");
        }
        else { Diag_Puts("khong doc duoc"); }
        Diag_Nl();
    }

    /*  Bit canh bao qua nhiet co tren ca 6 con, la canh bao nhiet duy nhat
     *  ma cac con slave TPS92667 cung cap.                                  */
    Diag_Puts("  Bit canh bao qua nhiet (STATUS bit 2) tung con:");
    Diag_Nl();
    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        uint8_t stT;
        TPS_CommsReset();
        TPS_DelayMs(2);
        if (TPS_ReadReg8(d, TPS_REG_STATUS, &stT) != TPS_OK)
        {
            Diag_Puts("    IC30"); Diag_Dec((uint32_t)(d + 1));
            Diag_Puts(" khong tra loi");
            Diag_Nl();
            continue;
        }
        Diag_Puts("    IC30"); Diag_Dec((uint32_t)(d + 1));
        Diag_Puts((stT & 0x04) ? " QUA NHIET" : " binh thuong");
        Diag_Nl();
    }


    Diag_Puts(" So lan tran bo dem nhan (ORE): "); Diag_Dec(TPS_OverrunCount);
    Diag_Nl();
    Diag_Puts(" So lan phai thu lai giao dich : "); Diag_Dec(TPS_RetryCount);
    Diag_Nl();

    okRead = 0; okWrite = 0; okBack = 0;

    /*  Tat thu lai trong luc do, de moi phep phan anh dung mot lan giao tiep
     *  chu khong phai ket qua sau khi da thu ba lan. */
    TPS_MaxTries = 1;

    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        Diag_Puts("---------------------------------------------------------");
        Diag_Nl();
        Diag_Puts(" IC30"); Diag_Dec((uint32_t)(d + 1));
        Diag_Puts("  dia chi="); Diag_Dec((uint32_t)d);
        Diag_Puts("  DEVID=0x"); Diag_Hex8(TPS_DevIdOf(d));
        Diag_Nl();

        /*  Dua bus ve trang thai sach truoc khi do con nay, de ket qua cua
         *  no khong bi con truoc lam hong. Khong co buoc nay thi mot IC im
         *  lang se keo theo IC ke tiep cung im lang, va bao cao do nham mot
         *  IC tot thanh IC hong. */
        TPS_CommsReset();
        TPS_DelayMs(2);

        /* --- Phep 1: doc ICID ------------------------------------------- */
        icid = 0;
        err  = TPS_ReadReg8(d, TPS_REG_ICID, &icid);
        Diag_Puts("  1. DOC  ICID   : "); Diag_Puts(DiagErrName(err));
        if (err == TPS_OK)
        {
            okRead++;
            Diag_Puts(" =0x"); Diag_Hex8(icid);
            Diag_Puts(" "); Diag_Puts(DiagPartName(icid));
        }
        Diag_Nl();

        /* --- Phep 2: ghi thu vao WIDTH01H -------------------------------- */
        err = TPS_WriteReg8(d, TPS_REG_WIDTH_BASE, testVal);
        Diag_Puts("  2. GHI  WIDTH01: "); Diag_Puts(DiagErrName(err));
        if (err == TPS_OK) { okWrite++; Diag_Puts(" (IC da tra ACK 0x7F)"); }
        Diag_Nl();

        /* --- Phep 3: doc lai chinh thanh ghi vua ghi --------------------- */
        rd  = 0;
        err = TPS_ReadReg8(d, TPS_REG_WIDTH_BASE, &rd);
        Diag_Puts("  3. DOC LAI     : "); Diag_Puts(DiagErrName(err));
        if (err == TPS_OK)
        {
            Diag_Puts(" =0x"); Diag_Hex8(rd);
            if (rd == testVal) { okBack++; Diag_Puts(" KHOP"); }
            else               { Diag_Puts(" KHONG KHOP, mong doi 0x"); Diag_Hex8(testVal); }
        }
        Diag_Nl();

        /* --- Phep 4: doc STATUS ----------------------------------------- */
        st  = 0;
        err = TPS_ReadReg8(d, TPS_REG_STATUS, &st);
        Diag_Puts("  4. DOC  STATUS : "); Diag_Puts(DiagErrName(err));
        Diag_Nl();
        if (err == TPS_OK) { DiagPrintStatus(st); }

        /* Tra WIDTH01H ve 0 de khong lam sai bang do sang sau nay */
        (void)TPS_WriteReg8(d, TPS_REG_WIDTH_BASE, 0x00);
    }

    /* ------------------------------------------------------------------- */
    /*  Quet CA 16 dia chi.
     *
     *  Ly do: neu mot IC khong tra loi o dia chi ta mong doi, van con hai
     *  kha nang rat khac nhau. Mot la no chet hoac mat nguon. Hai la no van
     *  song nhung chan ADDRx bi dat sai nen dang nam o dia chi khac. Phep
     *  quet nay phan biet duoc hai truong hop do, viec ma chi quet 6 dia chi
     *  khong the lam duoc.
     * ------------------------------------------------------------------- */
    Diag_Puts("---------------------------------------------------------");
    Diag_Nl();
    Diag_Puts(" QUET CA 16 DIA CHI (tim IC bi dat sai chan ADDRx)");
    Diag_Nl();

    {
        uint8_t a, found = 0;

        /*  Rieng phep quet nay BAT thu lai. Muc dich cua no la phat hien co
         *  IC nao ton tai hay khong, nen can do nhay cao nhat. Bus dang rat
         *  chap chon (so lan thu lai rat lon), mot lan thu de bo sot. */
        TPS_MaxTries = 3;

        for (a = 0; a < 16u; a++)
        {
            /*  Truoc MOI dia chi phai phat Communications Reset roi cho mot
             *  chut.
             *
             *  Ly do: o che do SEPTR = 0, mot khung het gio cho se lam may
             *  trang thai cua cac IC lech nhip, va chung lam ngo khung ke
             *  tiep. Vong quet chay sat nhau nen dia chi nao cung bi dia chi
             *  truoc do lam hong. Ban dau tien cua vong quet nay khong co
             *  buoc reset, va no bao 0 thiet bi trong khi phep do tung IC
             *  ngay truoc do van thay IC302 tra loi binh thuong. Ket qua mau
             *  thuan do la cach phat hien ra thieu sot nay. */
            TPS_CommsReset();
            TPS_DelayMs(2);

            icid = 0;
            if (TPS_ReadReg8(a, TPS_REG_ICID, &icid) == TPS_OK)
            {
                found++;
                Diag_Puts("   dia chi "); Diag_Dec((uint32_t)a);
                Diag_Puts("  DEVID=0x"); Diag_Hex8(TPS_DevIdOf(a));
                Diag_Puts("  ICID=0x");  Diag_Hex8(icid);
                Diag_Puts("  ");         Diag_Puts(DiagPartName(icid));
                if (a >= TPS_DEV_COUNT) { Diag_Puts("  <== NGOAI DAI 0..5"); }
                Diag_Nl();
            }
        }

        Diag_Puts("   Tong so IC tim thay tren toan bus: ");
        Diag_Dec((uint32_t)found);
        Diag_Puts(" / 6 mong doi");
        Diag_Nl();

        if (found == 0u)
        {
            Diag_Puts("   => Khong con nao tra loi o bat ky dia chi nao.");
            Diag_Nl();
        }
    }

    /* ------------------------------------------------------------------- */
    /*  KIEM TRA CLOCK LVDS
     *
     *  TPS92667 (cac con slave) KHONG co dao dong noi cho phan logic. Khi
     *  chua nhan duoc clock ngoai tren CLK_H/CLK_L thi chung nam trong
     *  Fail-Safe va KHONG giao tiep duoc, du VDD van du.
     *
     *  Dieu do giai thich cung luc CA HAI trieu chung bang MOT nguyen nhan:
     *  vua cam lang tren UART, vua sang den thuong truc neu chan FS dat o
     *  muc lam switch mo.
     *
     *  Hai phep doc sau kiem tra truc tiep gia thuyet do.
     * ------------------------------------------------------------------- */
    /* ------------------------------------------------------------------- */
    /*  DO NHIEU NEN CUA DUONG NHAN
     *
     *  Dem so byte toi trong 200 ms ma vi dieu khien KHONG phat gi ca.
     *  Bus lanh manh phai cho ket qua 0: khi khong ai duoc hoi thi khong ai
     *  duoc phep noi.
     *
     *  Khac gia tri 0 nghia la co thu gi do dang lai duong TX lien tuc, va
     *  do chinh la thu lam moi cau tra loi bi de len thanh rac.
     * ------------------------------------------------------------------- */
    Diag_Puts("---------------------------------------------------------");
    Diag_Nl();
    Diag_Puts(" DO NHIEU NEN CUA DUONG NHAN (khong phat gi trong 200 ms)");
    Diag_Nl();
    {
        uint32_t noise = TPS_MeasureBusNoise(200u, 0u);

        Diag_Puts("   So byte nhan duoc khi im lang: ");
        Diag_Dec(noise);
        Diag_Nl();

        if (noise == 0u)
        {
            Diag_Puts("   => Bus YEN TINH. Khong ai lai duong TX bat thuong.");
            Diag_Nl();
            Diag_Puts("      Loi nam o cho khac, khong phai nghen bus.");
        }
        else
        {
            Diag_Puts("   => CO THU GI DO DANG LAI DUONG TX LIEN TUC.");
            Diag_Nl();
            Diag_Puts("      Hay RUT CAP CN100 ra roi chay lai phep do nay:");
            Diag_Nl();
            Diag_Puts("        con so ve 0  -> nhieu tu phia MODULE (mot IC hong)");
            Diag_Nl();
            Diag_Puts("        con so van cao -> nhieu tu phia BOARD hoac CAP");
        }
        Diag_Nl();
    }

    Diag_Puts("---------------------------------------------------------");
    Diag_Nl();
    Diag_Puts(" KIEM TRA CLOCK LVDS");
    Diag_Nl();

    {
        uint8_t v;

        /* 1. Master IC301 co thuc su DANG PHAT clock khong? */
        TPS_CommsReset();
        TPS_DelayMs(2);
        Diag_Puts("  IC301 OUTCTRL (bit LVDS_TX): ");
        if (TPS_ReadReg8(TPS_ADDR_IC301, TPS_REG_OUTCTRL, &v) == TPS_OK)
        {
            Diag_Puts("0x"); Diag_Hex8(v);
            Diag_Puts((v & TPS_OUTCTRL_LVDS_TX) ? "  -> DANG PHAT clock"
                                                : "  -> KHONG phat clock!");
        }
        else { Diag_Puts("khong doc duoc"); }
        Diag_Nl();

        /* 2. Cac con slave co THUC SU nhan duoc clock ngoai khong? */
        for (d = 0; d < TPS_DEV_COUNT; d++)
        {
            TPS_CommsReset();
            TPS_DelayMs(2);
            if (TPS_ReadReg8(d, TPS_REG_CLK_SYNC, &v) != TPS_OK) { continue; }

            Diag_Puts("  IC30"); Diag_Dec((uint32_t)(d + 1));
            Diag_Puts(" CLK_SYNC=0x"); Diag_Hex8(v);
            Diag_Puts((v & TPS_CLKSYNC_CLK_IS_EXT) ? "  clock NGOAI"
                                                   : "  clock NOI");
            if (v & TPS_CLKSYNC_VDD_OV_FLT) { Diag_Puts("  DA TUNG QUA AP VDD"); }
            Diag_Nl();
        }

        Diag_Puts("  Con nao khong hien o tren la con khong tra loi duoc.");
        Diag_Nl();
    }

    TPS_MaxTries = 3;          /* bat lai che do thu lai cho van hanh binh thuong */

    /* --- Ket luan ------------------------------------------------------- */
    Diag_Puts("=========================================================");
    Diag_Nl();
    if (s_truncated)
    {
        Diag_Puts(" (ban bao cao da cham gioi han bo dem, phan cuoi bi cat)");
        Diag_Nl();
    }
    Diag_Puts(" TONG KET: doc "); Diag_Dec(okRead);
    Diag_Puts("/6   ghi ");       Diag_Dec(okWrite);
    Diag_Puts("/6   doc lai khop "); Diag_Dec(okBack);
    Diag_Puts("/6");
    Diag_Nl();

    if (okRead == 0u && okWrite == 0u)
    {
        /*  Truong hop dac biet: KHONG con nao tra loi, ca doc lan ghi.
         *  Neu day that su la loi cua tung IC thi rat kho de ca sau con cung
         *  hong mot luc. Nhieu kha nang la khong co IC nao tren bus. */
        Diag_Puts(" => KHONG CO IC NAO TREN BUS.");
        Diag_Nl();
        Diag_Puts("    Ca 6 con deu im lang o ca hai chieu. Sau IC cung hong");
        Diag_Nl();
        Diag_Puts("    mot luc la rat kho xay ra, nen hay kiem tra ket noi:");
        Diag_Nl();
        Diag_Puts("      1. Cap 12 chan toi module da cam chua (CN100)");
        Diag_Nl();
        Diag_Puts("      2. Module da co nguon 5V chua (CN100 chan 5, GND chan 6)");
        Diag_Nl();
        Diag_Puts("      3. Hai day CAN_H (chan 1) va CAN_L (chan 3) co dut khong");
        Diag_Nl();
        Diag_Puts("    Luu y: bao cao nay KHONG bao loi MAT ECHO, tuc la bo thu");
        Diag_Nl();
        Diag_Puts("    phat CAN tren board dieu khien van hoat dong binh thuong.");
        Diag_Nl();
        Diag_Puts("    Vay loi nam o phia module hoac o duong day, khong phai");
        Diag_Nl();
        Diag_Puts("    o vi dieu khien.");
    }
    else if (okWrite == TPS_DEV_COUNT && okRead < TPS_DEV_COUNT)
    {
        Diag_Puts(" => Duong GHI tot ca 6 con, duong DOC hong.");
        Diag_Nl();
        Diag_Puts("    Dung nguyen nhan lam chi mot chuoi sang o ban cu,");
        Diag_Nl();
        Diag_Puts("    vi firmware cu chan lenh ghi theo ket qua lenh doc.");
        Diag_Nl();
        Diag_Puts("    Huong xu ly: tang thoi gian quay bus sau moi goi.");
    }
    else if (okWrite < TPS_DEV_COUNT && okRead < TPS_DEV_COUNT)
    {
        Diag_Puts(" => Mot so IC cam lang ca hai chieu, so con lai binh thuong.");
        Diag_Nl();
        Diag_Puts("    Neu da do duoc VDD du 5V tai chan cua chung, thi nguyen");
        Diag_Nl();
        Diag_Puts("    nhan con lai kha di nhat la THIEU CLOCK LVDS: TPS92667");
        Diag_Nl();
        Diag_Puts("    khong co dao dong noi, thieu clock la nam trong Fail-Safe");
        Diag_Nl();
        Diag_Puts("    va khong noi chuyen duoc, dong thoi den co the sang");
        Diag_Nl();
        Diag_Puts("    thuong truc neu chan FS dat o muc lam switch mo.");
        Diag_Nl();
        Diag_Puts("    Xem muc KIEM TRA CLOCK LVDS o tren, va do cap CLK_H/CLK_L");
        Diag_Nl();
        Diag_Puts("    tai chan 13/14 va 23/24 cua tung IC bang may hien song.");
    }
    else if (okRead == TPS_DEV_COUNT)
    {
        Diag_Puts(" => Ca 6 IC giao tiep tot ca hai chieu.");
        Diag_Nl();
        Diag_Puts("    Neu van chi mot chuoi sang thi loi nam o duong dien");
        Diag_Nl();
        Diag_Puts("    ANODE/CATHODE hoac o bo nguon, khong phai o firmware.");
    }
    Diag_Nl();
    Diag_Puts("=========================================================");
    Diag_Nl();
}
