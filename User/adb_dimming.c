/**
  ******************************************************************************
  * @file    adb_dimming.c
  * @brief   Ban do pixel va bang do sang cho module SSL 96-pixel ADB
  ******************************************************************************
  */

#include "adb_dimming.h"

#define A ADB_STR_A
#define B ADB_STR_B
#define C ADB_STR_C
#define D ADB_STR_D

/*  Thu tu kenh trong moi IC: kenh 1 = WIDTH01 (phia cathode), kenh 16 =
 *  WIDTH16 (phia anode).
 *
 *  IC301 (dia chi 0) : C14 D14 C15 D15 C16 D16 C17 D17 C18 D18 C19 D19 C20 D20 C21 D21
 *  IC302 (dia chi 1) : A22 B22 C22 D22 A23 B23 C23 D23 A24 B24 C24 A25 B25 C25 A26 B26
 *  IC303 (dia chi 2) : A14 B14 A15 B15 A16 B16 A17 B17 A18 B18 A19 B19 A20 B20 A21 B21
 *  IC304 (dia chi 3) : A6  B6  A7  B7  A8  B8  A9  B9  A10 B10 A11 B11 A12 B12 A13 B13
 *  IC305 (dia chi 4) : A1  B1  A2  B2  C2  A3  B3  C3  A4  B4  C4  D4  A5  B5  C5  D5
 *  IC306 (dia chi 5) : C6  D6  C7  D7  C8  D8  C9  D9  C10 D10 C11 D11 C12 D12 C13 D13
 */
const ADB_PixelLabel ADB_PIXEL_LABEL[TPS_PIXEL_COUNT] =
{
    /* IC301 - pixel 0..15 */
    {C,14},{D,14},{C,15},{D,15},{C,16},{D,16},{C,17},{D,17},
    {C,18},{D,18},{C,19},{D,19},{C,20},{D,20},{C,21},{D,21},
    /* IC302 - pixel 16..31 */
    {A,22},{B,22},{C,22},{D,22},{A,23},{B,23},{C,23},{D,23},
    {A,24},{B,24},{C,24},{A,25},{B,25},{C,25},{A,26},{B,26},
    /* IC303 - pixel 32..47 */
    {A,14},{B,14},{A,15},{B,15},{A,16},{B,16},{A,17},{B,17},
    {A,18},{B,18},{A,19},{B,19},{A,20},{B,20},{A,21},{B,21},
    /* IC304 - pixel 48..63 */
    {A, 6},{B, 6},{A, 7},{B, 7},{A, 8},{B, 8},{A, 9},{B, 9},
    {A,10},{B,10},{A,11},{B,11},{A,12},{B,12},{A,13},{B,13},
    /* IC305 - pixel 64..79 */
    {A, 1},{B, 1},{A, 2},{B, 2},{C, 2},{A, 3},{B, 3},{C, 3},
    {A, 4},{B, 4},{C, 4},{D, 4},{A, 5},{B, 5},{C, 5},{D, 5},
    /* IC306 - pixel 80..95 */
    {C, 6},{D, 6},{C, 7},{D, 7},{C, 8},{D, 8},{C, 9},{D, 9},
    {C,10},{D,10},{C,11},{D,11},{C,12},{D,12},{C,13},{D,13}
};

/*  Bang do sang mac dinh.
 *  Gia tri = lam tron(ty_le * 1023), voi ty le lay tu bang do sang cua module
 *  (26 hang LED x 4 cot chuoi A/B/C/D).
 *
 *  Nho: WIDTH = 1023 -> switch bypass MO -> LED sang toi da.
 *       WIDTH = 0    -> switch dong gan het chu ky -> LED tat.
 */
const uint16_t ADB_WIDTH_DEFAULT[TPS_DEV_COUNT][TPS_CH_PER_DEV] =
{
    /* IC301 : C14  D14  C15  D15  C16  D16  C17  D17  C18  D18  C19  D19  C20  D20  C21  D21 */
    {          379, 123, 512, 205, 573, 205, 614, 164, 368, 164, 256,  82, 184,  82, 205,  82 },

    /* IC302 : A22  B22  C22  D22  A23  B23  C23  D23  A24  B24  C24  A25  B25  C25  A26  B26 */
    {          276, 276, 153,  82, 256, 256, 153,  82, 246, 246, 174, 286, 297, 143, 307, 307 },

    /* IC303 : A14  B14  A15  B15  A16  B16  A17  B17  A18  B18  A19  B19  A20  B20  A21  B21 */
    {          675, 675, 900, 900,1023,1023, 900, 900, 655, 655, 450, 450, 338, 338, 276, 276 },

    /* IC304 : A6   B6   A7   B7   A8   B8   A9   B9   A10  B10  A11  B11  A12  B12  A13  B13 */
    {          266, 266, 286, 286, 297, 297, 297, 307, 317, 317, 338, 338, 368, 368, 409, 471 },

    /* IC305 : A1   B1   A2   B2   C2   A3   B3   C3   A4   B4   C4   D4   A5   B5   C5   D5  */
    {          225, 225, 225, 235, 123, 235, 235, 123, 256, 256, 143,  41, 256, 256, 143,  41 },

    /* IC306 : C6   D6   C7   D7   C8   D8   C9   D9   C10  D10  C11  D11  C12  D12  C13  D13 */
    {          153,  41, 164,  61, 164,  61, 164,  61, 174,  61, 184,  72, 205,  82, 266, 123 }
};

#undef A
#undef B
#undef C
#undef D

/* ------------------------------------------------------------------------- */

void ADB_LoadDefault(void)
{
    uint8_t d, c;

    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        for (c = 0; c < TPS_CH_PER_DEV; c++)
        {
            TPS_SetWidth(d, c, ADB_WIDTH_DEFAULT[d][c]);
        }
    }
}

void ADB_LoadDefaultScaled(uint8_t percent)
{
    uint8_t  d, c;
    uint32_t v;

    if (percent > 100) { percent = 100; }

    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        for (c = 0; c < TPS_CH_PER_DEV; c++)
        {
            v = ((uint32_t)ADB_WIDTH_DEFAULT[d][c] * percent) / 100u;
            TPS_SetWidth(d, c, (uint16_t)v);
        }
    }
}

void ADB_ClampWidth(uint16_t maxWidth)
{
    uint8_t d, c;

    if (maxWidth > TPS_WIDTH_MAX) { maxWidth = TPS_WIDTH_MAX; }

    for (d = 0; d < TPS_DEV_COUNT; d++)
    {
        for (c = 0; c < TPS_CH_PER_DEV; c++)
        {
            if (TPS_GetWidth(d, c) > maxWidth)
            {
                TPS_SetWidth(d, c, maxWidth);
            }
        }
    }
}

void ADB_SetPixel(uint8_t pixel, uint16_t width)
{
    if (pixel >= TPS_PIXEL_COUNT) { return; }
    TPS_SetWidth(ADB_PIXEL_TO_DEV(pixel), ADB_PIXEL_TO_CH(pixel), width);
}

uint8_t ADB_FindPixel(uint8_t str, uint8_t led)
{
    uint8_t p;

    for (p = 0; p < TPS_PIXEL_COUNT; p++)
    {
        if (ADB_PIXEL_LABEL[p].str == str && ADB_PIXEL_LABEL[p].led == led)
        {
            return p;
        }
    }
    return 0xFF;
}

uint8_t ADB_SetLed(uint8_t str, uint8_t led, uint16_t width)
{
    uint8_t p = ADB_FindPixel(str, led);

    if (p == 0xFF) { return 0; }
    ADB_SetPixel(p, width);
    return 1;
}

void ADB_SetStrings(uint8_t strMask)
{
    uint8_t p;

    for (p = 0; p < TPS_PIXEL_COUNT; p++)
    {
        if ((strMask & (1u << ADB_PIXEL_LABEL[p].str)) == 0)
        {
            ADB_SetPixel(p, 0);
        }
    }
}

void ADB_SetDarkWindow(uint8_t ledFrom, uint8_t ledTo)
{
    uint8_t p, led;

    if (ledFrom > ledTo) { return; }

    for (p = 0; p < TPS_PIXEL_COUNT; p++)
    {
        led = ADB_PIXEL_LABEL[p].led;
        if (led >= ledFrom && led <= ledTo)
        {
            ADB_SetPixel(p, 0);
        }
    }
}
