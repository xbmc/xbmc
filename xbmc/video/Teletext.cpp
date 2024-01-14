/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
 * Most Codeparts are taken from the TuxBox Teletext plugin which is based
 * upon videotext-0.6.19991029 and written by Thomas Loewe (LazyT),
 * Roland Meier and DBLuelle. See http://www.tuxtxt.net/ for more information.
 * Many thanks to the TuxBox Teletext Team for this great work.
 */

#include "Teletext.h"

#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "filesystem/SpecialProtocol.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/keyboard/KeyIDs.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <harfbuzz/hb-ft.h>

using namespace std::chrono_literals;

static inline void SDL_memset4(uint32_t* dst, uint32_t val, size_t len)
{
  for (; len > 0; --len)
    *dst++ = val;
}
#define SDL_memcpy4(dst, src, len) memcpy(dst, src, (len) << 2)

static const char *TeletextFont = "special://xbmc/media/Fonts/teletext.ttf";

/* spacing attributes */
#define alpha_black         0x00
#define alpha_red           0x01
#define alpha_green         0x02
#define alpha_yellow        0x03
#define alpha_blue          0x04
#define alpha_magenta       0x05
#define alpha_cyan          0x06
#define alpha_white         0x07
#define flash               0x08
#define steady              0x09
#define end_box             0x0A
#define start_box           0x0B
#define normal_size         0x0C
#define double_height       0x0D
#define double_width        0x0E
#define double_size         0x0F
#define mosaic_black        0x10
#define mosaic_red          0x11
#define mosaic_green        0x12
#define mosaic_yellow       0x13
#define mosaic_blue         0x14
#define mosaic_magenta      0x15
#define mosaic_cyan         0x16
#define mosaic_white        0x17
#define conceal             0x18
#define contiguous_mosaic   0x19
#define separated_mosaic    0x1A
#define esc                 0x1B
#define black_background    0x1C
#define new_background      0x1D
#define hold_mosaic         0x1E
#define release_mosaic      0x1F

#define RowAddress2Row(row) ((row == 40) ? 24 : (row - 40))

// G2 Set as defined in ETS 300 706
const unsigned short int G2table[5][6*16] =
{
  // Latin G2 Supplementary Set
  { 0x0020, 0x00A1, 0x00A2, 0x00A3, 0x0024, 0x00A5, 0x0023, 0x00A7, 0x00A4, 0x2018, 0x201C, 0x00AB, 0x2190, 0x2191, 0x2192, 0x2193,
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00D7, 0x00B5, 0x00B6, 0x00B7, 0x00F7, 0x2019, 0x201D, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
    0x0020, 0x0300, 0x0301, 0x02C6, 0x0303, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x00B8, 0x005F, 0x02DD, 0x02DB, 0x02C7,
    0x2014, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A, 0x20AC, 0x2030, 0x03B1, 0x0020, 0x0020, 0x0020, 0x215B, 0x215C, 0x215D, 0x215E,
    0x2126, 0x00C6, 0x00D0, 0x00AA, 0x0126, 0x0020, 0x0132, 0x013F, 0x0141, 0x00D8, 0x0152, 0x00BA, 0x00DE, 0x0166, 0x014A, 0x0149,
    0x0138, 0x00E6, 0x0111, 0x00F0, 0x0127, 0x0131, 0x0133, 0x0140, 0x0142, 0x00F8, 0x0153, 0x00DF, 0x00FE, 0x0167, 0x014B, 0x25A0},
  // Cyrillic G2 Supplementary Set
  { 0x0020, 0x00A1, 0x00A2, 0x00A3, 0x0024, 0x00A5, 0x0020, 0x00A7, 0x0020, 0x2018, 0x201C, 0x00AB, 0x2190, 0x2191, 0x2192, 0x2193,
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00D7, 0x00B5, 0x00B6, 0x00B7, 0x00F7, 0x2019, 0x201D, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
    0x0020, 0x0300, 0x0301, 0x02C6, 0x02DC, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x00B8, 0x005F, 0x02DD, 0x02DB, 0x02C7,
    0x2014, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A, 0x20AC, 0x2030, 0x03B1, 0x0141, 0x0142, 0x00DF, 0x215B, 0x215C, 0x215D, 0x215E,
    0x0044, 0x0045, 0x0046, 0x0047, 0x0049, 0x004A, 0x004B, 0x004C, 0x004E, 0x0051, 0x0052, 0x0053, 0x0055, 0x0056, 0x0057, 0x005A,
    0x0064, 0x0065, 0x0066, 0x0067, 0x0069, 0x006A, 0x006B, 0x006C, 0x006E, 0x0071, 0x0072, 0x0073, 0x0075, 0x0076, 0x0077, 0x007A},
  // Greek G2 Supplementary Set
  { 0x0020, 0x0061, 0x0062, 0x00A3, 0x0065, 0x0068, 0x0069, 0x00A7, 0x003A, 0x2018, 0x201C, 0x006B, 0x2190, 0x2191, 0x2192, 0x2193,
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00D7, 0x006D, 0x006E, 0x0070, 0x00F7, 0x2019, 0x201D, 0x0074, 0x00BC, 0x00BD, 0x00BE, 0x0078,
    0x0020, 0x0300, 0x0301, 0x02C6, 0x02DC, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x00B8, 0x005F, 0x02DD, 0x02DB, 0x02C7,
    0x003F, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A, 0x20AC, 0x2030, 0x03B1, 0x038A, 0x038E, 0x038F, 0x215B, 0x215C, 0x215D, 0x215E,
    0x0043, 0x0044, 0x0046, 0x0047, 0x004A, 0x004C, 0x0051, 0x0052, 0x0053, 0x0055, 0x0056, 0x0057, 0x0059, 0x005A, 0x0386, 0x0389,
    0x0063, 0x0064, 0x0066, 0x0067, 0x006A, 0x006C, 0x0071, 0x0072, 0x0073, 0x0075, 0x0076, 0x0077, 0x0079, 0x007A, 0x0388, 0x25A0},
  // Arabic G2 Set
  { 0x0020, 0x0639, 0xFEC9, 0xFE83, 0xFE85, 0xFE87, 0xFE8B, 0xFE89, 0xFB7C, 0xFB7D, 0xFB7A, 0xFB58, 0xFB59, 0xFB56, 0xFB6D, 0xFB8E,
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 0x0668, 0x0669, 0xFECE, 0xFECD, 0xFEFC, 0xFEEC, 0xFEEA, 0xFEE9,
    0x00E0, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x00EB, 0x00EA, 0x00F9, 0x00EE, 0xFECA,
    0x00E9, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x00E2, 0x00F4, 0x00FB, 0x00E7, 0x25A0}
};

//const (avoid warnings :<)
TextPageAttr_t Text_AtrTable[] =
{
  { TXT_ColorWhite  , TXT_ColorBlack , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_WB */
  { TXT_ColorWhite  , TXT_ColorBlack , C_G0P, 0, 0, 1 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_PassiveDefault */
  { TXT_ColorWhite  , TXT_ColorRed   , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L250 */
  { TXT_ColorBlack  , TXT_ColorGreen , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L251 */
  { TXT_ColorBlack  , TXT_ColorYellow, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L252 */
  { TXT_ColorWhite  , TXT_ColorBlue  , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_L253 */
  { TXT_ColorMagenta, TXT_ColorBlack , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU0 */
  { TXT_ColorGreen  , TXT_ColorBlack , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU1 */
  { TXT_ColorYellow , TXT_ColorBlack , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU2 */
  { TXT_ColorCyan   , TXT_ColorBlack , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_TOPMENU3 */
  { TXT_ColorMenu2  , TXT_ColorMenu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG0 */
  { TXT_ColorYellow , TXT_ColorMenu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG1 */
  { TXT_ColorMenu2  , TXT_ColorTransp, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG2 */
  { TXT_ColorWhite  , TXT_ColorMenu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSG3 */
  { TXT_ColorMenu2  , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM0 */
  { TXT_ColorYellow , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM1 */
  { TXT_ColorMenu2  , TXT_ColorBlack , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM2 */
  { TXT_ColorWhite  , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MSGDRM3 */
  { TXT_ColorMenu1  , TXT_ColorBlue  , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENUHIL0 5a Z */
  { TXT_ColorWhite  , TXT_ColorBlue  , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENUHIL1 58 X */
  { TXT_ColorMenu2  , TXT_ColorTransp, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENUHIL2 9b õ */
  { TXT_ColorMenu2  , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU0 ab ´ */
  { TXT_ColorYellow , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU1 a4 § */
  { TXT_ColorMenu2  , TXT_ColorTransp, C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU2 9b õ */
  { TXT_ColorMenu2  , TXT_ColorMenu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU3 cb À */
  { TXT_ColorCyan   , TXT_ColorMenu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU4 c7 « */
  { TXT_ColorWhite  , TXT_ColorMenu3 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU5 c8 » */
  { TXT_ColorWhite  , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_MENU6 a8 ® */
  { TXT_ColorYellow , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}, /* ATR_CATCHMENU0 a4 § */
  { TXT_ColorWhite  , TXT_ColorMenu1 , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f}  /* ATR_CATCHMENU1 a8 ® */
};

/* shapes */
enum
{
  S_END = 0,
  S_FHL, /* full horizontal line: y-offset */
  S_FVL, /* full vertical line: x-offset */
  S_BOX, /* rectangle: x-offset, y-offset, width, height */
  S_TRA, /* trapez: x0, y0, l0, x1, y1, l1 */
  S_BTR, /* trapez in bgcolor: x0, y0, l0, x1, y1, l1 */
  S_INV, /* invert */
  S_LNK, /* call other shape: shapenumber */
  S_CHR, /* Character from freetype hibyte, lowbyte */
  S_ADT, /* Character 2F alternating raster */
  S_FLH, /* flip horizontal */
  S_FLV  /* flip vertical */
};

/* shape coordinates */
enum
{
  S_W13 = 5, /* width*1/3 */
  S_W12, /* width*1/2 */
  S_W23, /* width*2/3 */
  S_W11, /* width */
  S_WM3, /* width-3 */
  S_H13, /* height*1/3 */
  S_H12, /* height*1/2 */
  S_H23, /* height*2/3 */
  S_H11, /* height */
  S_NrShCoord
};

/* G3 characters */
unsigned char aG3_20[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_21[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_22[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_23[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_24[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_25[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_26[] = { S_INV, S_LNK, 0x66, S_END };
unsigned char aG3_27[] = { S_INV, S_LNK, 0x67, S_END };
unsigned char aG3_28[] = { S_INV, S_LNK, 0x68, S_END };
unsigned char aG3_29[] = { S_INV, S_LNK, 0x69, S_END };
unsigned char aG3_2a[] = { S_INV, S_LNK, 0x6a, S_END };
unsigned char aG3_2b[] = { S_INV, S_LNK, 0x6b, S_END };
unsigned char aG3_2c[] = { S_INV, S_LNK, 0x6c, S_END };
unsigned char aG3_2d[] = { S_INV, S_LNK, 0x6d, S_END };
unsigned char aG3_2e[] = { S_BOX, 2, 0, 3, S_H11, S_END };
unsigned char aG3_2f[] = { S_ADT };
unsigned char aG3_30[] = { S_LNK, 0x20, S_FLH, S_END };
unsigned char aG3_31[] = { S_LNK, 0x21, S_FLH, S_END };
unsigned char aG3_32[] = { S_LNK, 0x22, S_FLH, S_END };
unsigned char aG3_33[] = { S_LNK, 0x23, S_FLH, S_END };
unsigned char aG3_34[] = { S_LNK, 0x24, S_FLH, S_END };
unsigned char aG3_35[] = { S_LNK, 0x25, S_FLH, S_END };
unsigned char aG3_36[] = { S_INV, S_LNK, 0x76, S_END };
unsigned char aG3_37[] = { S_INV, S_LNK, 0x77, S_END };
unsigned char aG3_38[] = { S_INV, S_LNK, 0x78, S_END };
unsigned char aG3_39[] = { S_INV, S_LNK, 0x79, S_END };
unsigned char aG3_3a[] = { S_INV, S_LNK, 0x7a, S_END };
unsigned char aG3_3b[] = { S_INV, S_LNK, 0x7b, S_END };
unsigned char aG3_3c[] = { S_INV, S_LNK, 0x7c, S_END };
unsigned char aG3_3d[] = { S_INV, S_LNK, 0x7d, S_END };
unsigned char aG3_3e[] = { S_LNK, 0x2e, S_FLH, S_END };
unsigned char aG3_3f[] = { S_BOX, 0, 0, S_W11, S_H11, S_END };
unsigned char aG3_40[] = { S_BOX, 0, S_H13, S_W11, S_H13, S_LNK, 0x7e, S_END };
unsigned char aG3_41[] = { S_BOX, 0, S_H13, S_W11, S_H13, S_LNK, 0x7e, S_FLV, S_END };
unsigned char aG3_42[] = { S_LNK, 0x50, S_BOX, S_W12, S_H13, S_W12, S_H13, S_END };
unsigned char aG3_43[] = { S_LNK, 0x50, S_BOX, 0, S_H13, S_W12, S_H13, S_END };
unsigned char aG3_44[] = { S_LNK, 0x48, S_FLV, S_LNK, 0x48, S_END };
unsigned char aG3_45[] = { S_LNK, 0x44, S_FLH, S_END };
unsigned char aG3_46[] = { S_LNK, 0x47, S_FLV, S_END };
unsigned char aG3_47[] = { S_LNK, 0x48, S_FLH, S_LNK, 0x48, S_END };
unsigned char aG3_48[] = { S_TRA, 0, 0, S_W23, 0, S_H23, 0, S_BTR, 0, 0, S_W13, 0, S_H13, 0, S_END };
unsigned char aG3_49[] = { S_LNK, 0x48, S_FLH, S_END };
unsigned char aG3_4a[] = { S_LNK, 0x48, S_FLV, S_END };
unsigned char aG3_4b[] = { S_LNK, 0x48, S_FLH, S_FLV, S_END };
unsigned char aG3_4c[] = { S_LNK, 0x50, S_BOX, 0, S_H13, S_W11, S_H13, S_END };
unsigned char aG3_4d[] = { S_CHR, 0x25, 0xE6 };
unsigned char aG3_4e[] = { S_CHR, 0x25, 0xCF };
unsigned char aG3_4f[] = { S_CHR, 0x25, 0xCB };
unsigned char aG3_50[] = { S_BOX, S_W12, 0, 2, S_H11, S_FLH, S_BOX, S_W12, 0, 2, S_H11,S_END };
unsigned char aG3_51[] = { S_BOX, 0, S_H12, S_W11, 2, S_FLV, S_BOX, 0, S_H12, S_W11, 2,S_END };
unsigned char aG3_52[] = { S_LNK, 0x55, S_FLH, S_FLV, S_END };
unsigned char aG3_53[] = { S_LNK, 0x55, S_FLV, S_END };
unsigned char aG3_54[] = { S_LNK, 0x55, S_FLH, S_END };
unsigned char aG3_55[] = { S_LNK, 0x7e, S_FLV, S_BOX, 0, S_H12, S_W12, 2, S_FLV, S_BOX, 0, S_H12, S_W12, 2, S_END };
unsigned char aG3_56[] = { S_LNK, 0x57, S_FLH, S_END};
unsigned char aG3_57[] = { S_LNK, 0x55, S_LNK, 0x50 , S_END};
unsigned char aG3_58[] = { S_LNK, 0x59, S_FLV, S_END};
unsigned char aG3_59[] = { S_LNK, 0x7e, S_LNK, 0x51 , S_END};
unsigned char aG3_5a[] = { S_LNK, 0x50, S_LNK, 0x51 , S_END};
unsigned char aG3_5b[] = { S_CHR, 0x21, 0x92};
unsigned char aG3_5c[] = { S_CHR, 0x21, 0x90};
unsigned char aG3_5d[] = { S_CHR, 0x21, 0x91};
unsigned char aG3_5e[] = { S_CHR, 0x21, 0x93};
unsigned char aG3_5f[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_60[] = { S_INV, S_LNK, 0x20, S_END };
unsigned char aG3_61[] = { S_INV, S_LNK, 0x21, S_END };
unsigned char aG3_62[] = { S_INV, S_LNK, 0x22, S_END };
unsigned char aG3_63[] = { S_INV, S_LNK, 0x23, S_END };
unsigned char aG3_64[] = { S_INV, S_LNK, 0x24, S_END };
unsigned char aG3_65[] = { S_INV, S_LNK, 0x25, S_END };
unsigned char aG3_66[] = { S_LNK, 0x20, S_FLV, S_END };
unsigned char aG3_67[] = { S_LNK, 0x21, S_FLV, S_END };
unsigned char aG3_68[] = { S_LNK, 0x22, S_FLV, S_END };
unsigned char aG3_69[] = { S_LNK, 0x23, S_FLV, S_END };
unsigned char aG3_6a[] = { S_LNK, 0x24, S_FLV, S_END };
unsigned char aG3_6b[] = { S_BOX, 0, 0, S_W11, S_H13, S_TRA, 0, S_H13, S_W11, 0, S_H23, 1, S_END };
unsigned char aG3_6c[] = { S_TRA, 0, 0, 1, 0, S_H12, S_W12, S_FLV, S_TRA, 0, 0, 1, 0, S_H12, S_W12, S_BOX, 0, S_H12, S_W12,1, S_END };
unsigned char aG3_6d[] = { S_TRA, 0, 0, S_W12, S_W12, S_H12, 0, S_FLH, S_TRA, 0, 0, S_W12, S_W12, S_H12, 0, S_END };
unsigned char aG3_6e[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_6f[] = { S_CHR, 0x00, 0x20};
unsigned char aG3_70[] = { S_INV, S_LNK, 0x30, S_END };
unsigned char aG3_71[] = { S_INV, S_LNK, 0x31, S_END };
unsigned char aG3_72[] = { S_INV, S_LNK, 0x32, S_END };
unsigned char aG3_73[] = { S_INV, S_LNK, 0x33, S_END };
unsigned char aG3_74[] = { S_INV, S_LNK, 0x34, S_END };
unsigned char aG3_75[] = { S_INV, S_LNK, 0x35, S_END };
unsigned char aG3_76[] = { S_LNK, 0x66, S_FLH, S_END };
unsigned char aG3_77[] = { S_LNK, 0x67, S_FLH, S_END };
unsigned char aG3_78[] = { S_LNK, 0x68, S_FLH, S_END };
unsigned char aG3_79[] = { S_LNK, 0x69, S_FLH, S_END };
unsigned char aG3_7a[] = { S_LNK, 0x6a, S_FLH, S_END };
unsigned char aG3_7b[] = { S_LNK, 0x6b, S_FLH, S_END };
unsigned char aG3_7c[] = { S_LNK, 0x6c, S_FLH, S_END };
unsigned char aG3_7d[] = { S_LNK, 0x6d, S_FLV, S_END };
unsigned char aG3_7e[] = { S_BOX, S_W12, 0, 2, S_H12, S_FLH, S_BOX, S_W12, 0, 2, S_H12, S_END };// help char, not printed directly (only by S_LNK)

unsigned char *aShapes[] =
{
  aG3_20, aG3_21, aG3_22, aG3_23, aG3_24, aG3_25, aG3_26, aG3_27, aG3_28, aG3_29, aG3_2a, aG3_2b, aG3_2c, aG3_2d, aG3_2e, aG3_2f,
  aG3_30, aG3_31, aG3_32, aG3_33, aG3_34, aG3_35, aG3_36, aG3_37, aG3_38, aG3_39, aG3_3a, aG3_3b, aG3_3c, aG3_3d, aG3_3e, aG3_3f,
  aG3_40, aG3_41, aG3_42, aG3_43, aG3_44, aG3_45, aG3_46, aG3_47, aG3_48, aG3_49, aG3_4a, aG3_4b, aG3_4c, aG3_4d, aG3_4e, aG3_4f,
  aG3_50, aG3_51, aG3_52, aG3_53, aG3_54, aG3_55, aG3_56, aG3_57, aG3_58, aG3_59, aG3_5a, aG3_5b, aG3_5c, aG3_5d, aG3_5e, aG3_5f,
  aG3_60, aG3_61, aG3_62, aG3_63, aG3_64, aG3_65, aG3_66, aG3_67, aG3_68, aG3_69, aG3_6a, aG3_6b, aG3_6c, aG3_6d, aG3_6e, aG3_6f,
  aG3_70, aG3_71, aG3_72, aG3_73, aG3_74, aG3_75, aG3_76, aG3_77, aG3_78, aG3_79, aG3_7a, aG3_7b, aG3_7c, aG3_7d, aG3_7e
};

// G0 Table as defined in ETS 300 706
// cyrillic G0 Charset (0 = Serbian/Croatian, 1 = Russian/Bulgarian, 2 = Ukrainian)
const unsigned short int G0table[6][6*16] =
{
  // Cyrillic G0 Set - Option 1 - Serbian/Croatian
  { ' ', '!', '\"', '#', '$', '%', '&', '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    0x0427, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0408, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
    0x041F, 0x040C, 0x0420, 0x0421, 0x0422, 0x0423, 0x0412, 0x0403, 0x0409, 0x040A, 0x0417, 0x040B, 0x0416, 0x0402, 0x0428, 0x040F,
    0x0447, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0458, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
    0x043F, 0x045C, 0x0440, 0x0441, 0x0442, 0x0443, 0x0432, 0x0453, 0x0459, 0x045A, 0x0437, 0x045B, 0x0436, 0x0452, 0x0448, 0x25A0},
  // Cyrillic G0 Set - Option 2 - Russian/Bulgarian
  { ' ', '!', '\"', '#', '$', '%', 0x044B, '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
    0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042A, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042B,
    0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
    0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044C, 0x044A, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x25A0},
  // Cyrillic G0 Set - Option 3 - Ukrainian
  { ' ', '!', '\"', '#', '$', '%', 0x0457, '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
    0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x0406, 0x0417, 0x0428, 0x0404, 0x0429, 0x0427, 0x0407,
    0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
    0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044C, 0x0456, 0x0437, 0x0448, 0x0454, 0x0449, 0x0447, 0x25A0},
  // Greek G0 Set
  { ' ', '!', '\"', '#', '$', '%', '&', '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', 0x00AB, '=', 0x00BB, '?',
    0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
    0x03A0, 0x03A1, 0x0384, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
    0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7, 0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
    0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7, 0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x25A0},
  // Hebrew G0 Set
  { ' ', '!', 0x05F2, 0x00A3, '$', '%', '&', '\'', '(' , ')' , '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0x2190, 0x00BD, 0x2192, 0x2191, '#',
    0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
    0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x20AA, 0x2551, 0x00BE, 0x00F7, 0x25A0},
  // Arabic G0 Set - Thanks to Habib2006(fannansat)
  { ' ', '!', 0x05F2, 0x00A3, '$', 0x066A, 0xFEF0, 0xFEF2, 0xFD3F, 0xFD3E, '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', 0x061B, '>', '=', '<', 0x061F,
    0xFE94, 0x0621, 0xFE92, 0x0628, 0xFE98, 0x062A, 0xFE8E, 0xFE8D, 0xFE91, 0xFE93, 0xFE97, 0xFE9B, 0xFE9F, 0xFEA3, 0xFEA7, 0xFEA9,
    0x0630, 0xFEAD, 0xFEAF, 0xFEB3, 0xFEB7, 0xFEBB, 0xFEBF, 0xFEC1, 0xFEC5, 0xFECB, 0xFECF, 0xFE9C, 0xFEA0, 0xFEA4, 0xFEA8, 0x0023,
    0x0640, 0xFED3, 0xFED7, 0xFEDB, 0xFEDF, 0xFEE3, 0xFEE7, 0xFEEB, 0xFEED, 0xFEEF, 0xFEF3, 0xFE99, 0xFE9D, 0xFEA1, 0xFEA5, 0xFEF4,
    0xFEF0, 0xFECC, 0xFED0, 0xFED4, 0xFED1, 0xFED8, 0xFED5, 0xFED9, 0xFEE0, 0xFEDD, 0xFEE4, 0xFEE1, 0xFEE8, 0xFEE5, 0xFEFB, 0x25A0}
};

const unsigned short int nationaltable23[14][2] =
{
  { '#',    0x00A4 }, /* 0          */
  { '#',    0x016F }, /* 1  CS/SK   */
  { 0x00A3,    '$' }, /* 2    EN    */
  { '#',    0x00F5 }, /* 3    ET    */
  { 0x00E9, 0x0457 }, /* 4    FR    */
  { '#',       '$' }, /* 5    DE    */
  { 0x00A3,    '$' }, /* 6    IT    */
  { '#',       '$' }, /* 7  LV/LT   */
  { '#',    0x0144 }, /* 8    PL    */
  { 0x00E7,    '$' }, /* 9  PT/ES   */
  { '#',    0x00A4 }, /* A    RO    */
  { '#',    0x00CB }, /* B SR/HR/SL */
  { '#',    0x00A4 }, /* C SV/FI/HU */
  { 0x20A4, 0x011F }, /* D    TR    */
};
const unsigned short int nationaltable40[14] =
{
  '@',    /* 0          */
  0x010D, /* 1  CS/SK   */
  '@',    /* 2    EN    */
  0x0161, /* 3    ET    */
  0x00E0, /* 4    FR    */
  0x00A7, /* 5    DE    */
  0x00E9, /* 6    IT    */
  0x0161, /* 7  LV/LT   */
  0x0105, /* 8    PL    */
  0x00A1, /* 9  PT/ES   */
  0x0162, /* A    RO    */
  0x010C, /* B SR/HR/SL */
  0x00C9, /* C SV/FI/HU */
  0x0130, /* D    TR    */
};
const unsigned short int nationaltable5b[14][6] =
{
  {    '[',   '\\',    ']',    '^',    '_',    '`' }, /* 0          */
  { 0x0165, 0x017E, 0x00FD, 0x00ED, 0x0159, 0x00E9 }, /* 1  CS/SK   */
  { 0x2190, 0x00BD, 0x2192, 0x2191,    '#', 0x00AD }, /* 2    EN    */
  { 0x00C4, 0x00D6, 0x017D, 0x00DC, 0x00D5, 0x0161 }, /* 3    ET    */
  { 0x0451, 0x00EA, 0x00F9, 0x00EE,    '#', 0x00E8 }, /* 4    FR    */
  { 0x00C4, 0x00D6, 0x00DC,    '^',    '_', 0x00B0 }, /* 5    DE    */
  { 0x00B0, 0x00E7, 0x2192, 0x2191,    '#', 0x00F9 }, /* 6    IT    */
  { 0x0117, 0x0119, 0x017D, 0x010D, 0x016B, 0x0161 }, /* 7  LV/LT   */
  { 0x017B, 0x015A, 0x0141, 0x0107, 0x00F3, 0x0119 }, /* 8    PL    */
  { 0x00E1, 0x00E9, 0x00ED, 0x00F3, 0x00FA, 0x00BF }, /* 9  PT/ES   */
  { 0x00C2, 0x015E, 0x01CD, 0x01CF, 0x0131, 0x0163 }, /* A    RO    */
  { 0x0106, 0x017D, 0x00D0, 0x0160, 0x0451, 0x010D }, /* B SR/HR/SL */
  { 0x00C4, 0x00D6, 0x00C5, 0x00DC,    '_', 0x00E9 }, /* C SV/FI/HU */
  { 0x015E, 0x00D6, 0x00C7, 0x00DC, 0x011E, 0x0131 }, /* D    TR    */
};
const unsigned short int nationaltable7b[14][4] =
{
  { '{',       '|',    '}',    '~' }, /* 0          */
  { 0x00E1, 0x011B, 0x00FA, 0x0161 }, /* 1  CS/SK   */
  { 0x00BC, 0x2551, 0x00BE, 0x00F7 }, /* 2    EN    */
  { 0x00E4, 0x00F6, 0x017E, 0x00FC }, /* 3    ET    */
  { 0x00E2, 0x00F4, 0x00FB, 0x00E7 }, /* 4    FR    */
  { 0x00E4, 0x00F6, 0x00FC, 0x00DF }, /* 5    DE    */
  { 0x00E0, 0x00F3, 0x00E8, 0x00EC }, /* 6    IT    */
  { 0x0105, 0x0173, 0x017E, 0x012F }, /* 7  LV/LT   */
  { 0x017C, 0x015B, 0x0142, 0x017A }, /* 8    PL    */
  { 0x00FC, 0x00F1, 0x00E8, 0x00E0 }, /* 9  PT/ES   */
  { 0x00E2, 0x015F, 0x01CE, 0x00EE }, /* A    RO    */
  { 0x0107, 0x017E, 0x0111, 0x0161 }, /* B SR/HR/SL */
  { 0x00E4, 0x00F6, 0x00E5, 0x00FC }, /* C SV/FI/HU */
  { 0x015F, 0x00F6, 0x00E7, 0x00FC }, /* D    TR    */
};
const unsigned short int arrowtable[] =
{
  8592, 8594, 8593, 8595, 'O', 'K', 8592, 8592
};

CTeletextDecoder::CTeletextDecoder()
{
  memset(&m_RenderInfo, 0, sizeof(TextRenderInfo_t));

  m_teletextFont                 = CSpecialProtocol::TranslatePath(TeletextFont);
  m_TextureBuffer                = NULL;
  m_txtCache                     = NULL;
  m_Manager                      = NULL;
  m_Library                      = NULL;
  m_RenderInfo.ShowFlof          = true;
  m_RenderInfo.Show39            = false;
  m_RenderInfo.Showl25           = true;
  m_RenderInfo.Prev_100          = 0x100;
  m_RenderInfo.Prev_10           = 0x100;
  m_RenderInfo.Next_100          = 0x100;
  m_RenderInfo.Next_10           = 0x100;
  m_RenderInfo.InputCounter      = 2;

  unsigned short rd0[] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0x00<<8, 0x00<<8, 0x00<<8, 0,      0      };
  unsigned short gn0[] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0x20<<8, 0x10<<8, 0x20<<8, 0,      0      };
  unsigned short bl0[] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0x40<<8, 0x20<<8, 0x40<<8, 0,      0      };
  unsigned short tr0[] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                          0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                          0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                          0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
                          0x0000 , 0x0000 , 0x0A0A , 0xFFFF, 0x3030 };

  memcpy(m_RenderInfo.rd0,rd0,TXT_Color_SIZECOLTABLE*sizeof(unsigned short));
  memcpy(m_RenderInfo.gn0,gn0,TXT_Color_SIZECOLTABLE*sizeof(unsigned short));
  memcpy(m_RenderInfo.bl0,bl0,TXT_Color_SIZECOLTABLE*sizeof(unsigned short));
  memcpy(m_RenderInfo.tr0,tr0,TXT_Color_SIZECOLTABLE*sizeof(unsigned short));

  m_LastPage = 0;
  m_TempPage = 0;
  m_Ascender = 0;
  m_PCOldCol = 0;
  m_PCOldRow = 0;
  m_CatchedPage = 0;
  m_CatchCol = 0;
  m_CatchRow = 0;
  prevTimeSec = 0;
  prevHeaderPage = 0;
  m_updateTexture = false;
  m_YOffset = 0;
}

CTeletextDecoder::~CTeletextDecoder() = default;

bool CTeletextDecoder::Changed()
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);
  if (IsSubtitlePage(m_txtCache->Page))
  {
    m_updateTexture = true;
    return true;
  }

  /* Update on every changed second */
  if (m_txtCache->TimeString[7] != prevTimeSec)
  {
    prevTimeSec = m_txtCache->TimeString[7];
    m_updateTexture = true;
    return true;
  }
  return false;
}

bool CTeletextDecoder::HandleAction(const CAction &action)
{
  if (m_txtCache == NULL)
  {
    CLog::Log(LOGERROR, "CTeletextDecoder::HandleAction called without teletext cache");
    return false;
  }

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  if (action.GetID() == ACTION_MOVE_UP)
  {
    if (m_RenderInfo.PageCatching)
      CatchNextPage(-1, -1);
    else
      GetNextPageOne(true);
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_DOWN)
  {
    if (m_RenderInfo.PageCatching)
      CatchNextPage(1, 1);
    else
      GetNextPageOne(false);
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_RIGHT)
  {
    if (m_RenderInfo.PageCatching)
      CatchNextPage(0, 1);
    else if (m_RenderInfo.Boxed)
    {
      m_RenderInfo.SubtitleDelay++;
          // display SubtitleDelay
      m_RenderInfo.PosY = 0;
      char ns[10];
      SetPosX(1);
      snprintf(ns, sizeof(ns), "+%d    ", m_RenderInfo.SubtitleDelay);
      RenderCharFB(ns[0], &Text_AtrTable[ATR_WB]);
      RenderCharFB(ns[1], &Text_AtrTable[ATR_WB]);
      RenderCharFB(ns[2], &Text_AtrTable[ATR_WB]);
      RenderCharFB(ns[4], &Text_AtrTable[ATR_WB]);
    }
    else
    {
      GetNextSubPage(1);
    }
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_LEFT)
  {
    if (m_RenderInfo.PageCatching)
      CatchNextPage(0, -1);
    else if (m_RenderInfo.Boxed)
    {
        m_RenderInfo.SubtitleDelay--;

        // display subtitledelay
        m_RenderInfo.PosY = 0;
        char ns[10];
        SetPosX(1);
        snprintf(ns, sizeof(ns), "+%d    ", m_RenderInfo.SubtitleDelay);
        RenderCharFB(ns[0], &Text_AtrTable[ATR_WB]);
        RenderCharFB(ns[1], &Text_AtrTable[ATR_WB]);
        RenderCharFB(ns[2], &Text_AtrTable[ATR_WB]);
        RenderCharFB(ns[4], &Text_AtrTable[ATR_WB]);
    }
    else
    {
      GetNextSubPage(-1);
    }
    return true;
  }
  else if (action.GetID() >= REMOTE_0 && action.GetID() <= REMOTE_9)
  {
    PageInput(action.GetID() - REMOTE_0);
    return true;
  }
  else if (action.GetID() == KEY_UNICODE)
  { // input from the keyboard
    if (action.GetUnicode() >= 48 && action.GetUnicode() < 58)
    {
      PageInput(action.GetUnicode() - 48);
      return true;
    }
    return false;
  }
  else if (action.GetID() == ACTION_PAGE_UP)
  {
    SwitchZoomMode();
    return true;
  }
  else if (action.GetID() == ACTION_PAGE_DOWN)
  {
    SwitchTranspMode();
    return true;
  }
  else if (action.GetID() == ACTION_SELECT_ITEM)
  {
    if (m_txtCache->SubPageTable[m_txtCache->Page] == 0xFF)
      return false;

    if (!m_RenderInfo.PageCatching)
      StartPageCatching();
    else
      StopPageCatching();

    return true;
  }

  if (m_RenderInfo.PageCatching)
  {
    m_txtCache->PageUpdate    = true;
    m_RenderInfo.PageCatching = false;
    return true;
  }

  if (action.GetID() == ACTION_SHOW_INFO)
  {
    SwitchHintMode();
    return true;
  }
  else if (action.GetID() == ACTION_TELETEXT_RED)
  {
    ColorKey(m_RenderInfo.Prev_100);
    return true;
  }
  else if (action.GetID() == ACTION_TELETEXT_GREEN)
  {
    ColorKey(m_RenderInfo.Prev_10);
    return true;
  }
  else if (action.GetID() == ACTION_TELETEXT_YELLOW)
  {
    ColorKey(m_RenderInfo.Next_10);
    return true;
  }
  else if (action.GetID() == ACTION_TELETEXT_BLUE)
  {
    ColorKey(m_RenderInfo.Next_100);
    return true;
  }

  return false;
}

bool CTeletextDecoder::InitDecoder()
{
  int error;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  m_txtCache = appPlayer->GetTeletextCache();
  if (m_txtCache == nullptr)
  {
    CLog::Log(LOGERROR, "{}: called without teletext cache", __FUNCTION__);
    return false;
  }

  /* init fontlibrary */
  if ((error = FT_Init_FreeType(&m_Library)))
  {
    CLog::Log(LOGERROR, "{}: <FT_Init_FreeType: {:#2X}>", __FUNCTION__, error);
    m_Library = NULL;
    return false;
  }

  if ((error = FTC_Manager_New(m_Library, 7, 2, 0, &MyFaceRequester, NULL, &m_Manager)))
  {
    FT_Done_FreeType(m_Library);
    m_Library = NULL;
    m_Manager = NULL;
    CLog::Log(LOGERROR, "{}: <FTC_Manager_New: {:#2X}>", __FUNCTION__, error);
    return false;
  }

  if ((error = FTC_SBitCache_New(m_Manager, &m_Cache)))
  {
    FTC_Manager_Done(m_Manager);
    FT_Done_FreeType(m_Library);
    m_Manager = NULL;
    m_Library = NULL;
    CLog::Log(LOGERROR, "{}: <FTC_SBit_Cache_New: {:#2X}>", __FUNCTION__, error);
    return false;
  }

  /* calculate font dimensions */
  m_RenderInfo.Width            = (int)(CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth()*CServiceBroker::GetWinSystem()->GetGfxContext().GetGUIScaleX());
  m_RenderInfo.Height           = (int)(CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight()*CServiceBroker::GetWinSystem()->GetGfxContext().GetGUIScaleY());
  m_RenderInfo.FontHeight       = m_RenderInfo.Height / 25;
  m_RenderInfo.FontWidth_Normal = m_RenderInfo.Width  / (m_RenderInfo.Show39 ? 39 : 40);
  SetFontWidth(m_RenderInfo.FontWidth_Normal);
  for (int i = 0; i <= 10; i++)
    m_RenderInfo.axdrcs[i+12+1] = (m_RenderInfo.FontHeight * i + 6) / 10;

  /* center screen */
  m_TypeTTF.face_id   = (FTC_FaceID) const_cast<char*>(m_teletextFont.c_str());
  m_TypeTTF.height    = (FT_UShort) m_RenderInfo.FontHeight;
  m_TypeTTF.flags     = FT_LOAD_MONOCHROME;
  if (FTC_Manager_LookupFace(m_Manager, m_TypeTTF.face_id, &m_Face))
  {
    m_TypeTTF.face_id = (FTC_FaceID) const_cast<char*>(m_teletextFont.c_str());
    if ((error = FTC_Manager_LookupFace(m_Manager, m_TypeTTF.face_id, &m_Face)))
    {
      CLog::Log(LOGERROR, "{}: <FTC_Manager_Lookup_Face failed with Errorcode {:#2X}>",
                __FUNCTION__, error);
      FTC_Manager_Done(m_Manager);
      FT_Done_FreeType(m_Library);
      m_Manager = NULL;
      m_Library = NULL;
      return false;
    }
  }
  m_Ascender = m_RenderInfo.FontHeight * m_Face->ascender / m_Face->units_per_EM;

  /* set variable screeninfo for double buffering */
  m_YOffset       = 0;
  m_TextureBuffer = new UTILS::COLOR::Color[4 * m_RenderInfo.Height * m_RenderInfo.Width];

  ClearFB(GetColorRGB(TXT_ColorTransp));
  ClearBB(GetColorRGB(TXT_ColorTransp)); /* initialize backbuffer */
  /* set new colormap */
  SetColors(DefaultColors, 0, TXT_Color_SIZECOLTABLE);

  for (int i = 0; i < 40 * 25; i++)
  {
    m_RenderInfo.PageChar[i]         = ' ';
    m_RenderInfo.PageAtrb[i].fg      = TXT_ColorTransp;
    m_RenderInfo.PageAtrb[i].bg      = TXT_ColorTransp;
    m_RenderInfo.PageAtrb[i].charset = C_G0P;
    m_RenderInfo.PageAtrb[i].doubleh = 0;
    m_RenderInfo.PageAtrb[i].doublew = 0;
    m_RenderInfo.PageAtrb[i].IgnoreAtBlackBgSubst = 0;
  }

  m_RenderInfo.TranspMode = false;
  m_LastPage              = 0x100;

  return true;
}

void CTeletextDecoder::EndDecoder()
{
  /* clear SubtitleCache */
  for (TextSubtitleCache_t*& subtitleCache : m_RenderInfo.SubtitleCache)
  {
    if (subtitleCache != NULL)
    {
      delete subtitleCache;
      subtitleCache = NULL;
    }
  }

  if (m_TextureBuffer)
  {
    delete[] m_TextureBuffer;
    m_TextureBuffer = NULL;
  }

  /* close freetype */
  if (m_Manager)
  {
    FTC_Node_Unref(m_anode, m_Manager);
    FTC_Manager_Done(m_Manager);
  }
  if (m_Library)
  {
    FT_Done_FreeType(m_Library);
  }

  m_Manager               = NULL;
  m_Library               = NULL;

  if (!m_txtCache)
  {
    CLog::Log(LOGINFO, "{}: called without cache", __FUNCTION__);
  }
  else
  {
    std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);
    m_txtCache->PageUpdate = true;
    CLog::Log(LOGDEBUG, "Teletext: Rendering ended");
  }
}

void CTeletextDecoder::PageInput(int Number)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  m_updateTexture = true;

  /* clear m_TempPage */
  if (m_RenderInfo.InputCounter == 2)
    m_TempPage = 0;

  /* check for 0 & 9 on first position */
  if (Number == 0 && m_RenderInfo.InputCounter == 2)
  {
    /* set page */
    m_TempPage = m_LastPage; /* 0 toggles to last page as in program switching */
    m_RenderInfo.InputCounter = -1;
  }
  else if (Number == 9 && m_RenderInfo.InputCounter == 2)
  {
    return;
  }

  /* show pageinput */
  if (m_RenderInfo.ZoomMode == 2)
  {
    m_RenderInfo.ZoomMode = 1;
    CopyBB2FB();
  }

  m_RenderInfo.PosY = 0;

  switch (m_RenderInfo.InputCounter)
  {
  case 2:
    SetPosX(1);
    RenderCharFB(Number | '0', &Text_AtrTable[ATR_WB]);
    RenderCharFB('-',          &Text_AtrTable[ATR_WB]);
    RenderCharFB('-',          &Text_AtrTable[ATR_WB]);
    break;

  case 1:
    SetPosX(2);
    RenderCharFB(Number | '0', &Text_AtrTable[ATR_WB]);
    break;

  case 0:
    SetPosX(3);
    RenderCharFB(Number | '0', &Text_AtrTable[ATR_WB]);
    break;
  }

  /* generate pagenumber */
  m_TempPage |= Number << (m_RenderInfo.InputCounter*4);

  m_RenderInfo.InputCounter--;

  if (m_RenderInfo.InputCounter < 0)
  {
    /* disable SubPage zapping */
    m_txtCache->ZapSubpageManual = false;

    /* reset input */
    m_RenderInfo.InputCounter = 2;

    /* set new page */
    m_LastPage = m_txtCache->Page;

    m_txtCache->Page      = m_TempPage;
    m_RenderInfo.HintMode = false;

    /* check cache */
    int subp = m_txtCache->SubPageTable[m_txtCache->Page];
    if (subp != 0xFF)
    {
      m_txtCache->SubPage     = subp;
      m_txtCache->PageUpdate  = true;
    }
    else
    {
      m_txtCache->SubPage = 0;
//      RenderMessage(PageNotFound);
    }
  }
}

void CTeletextDecoder::GetNextPageOne(bool up)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* disable subpage zapping */
  m_txtCache->ZapSubpageManual = false;

  /* abort pageinput */
  m_RenderInfo.InputCounter = 2;

  /* find next cached page */
  m_LastPage = m_txtCache->Page;

  int subp;
  do {
    if (up)
      CDVDTeletextTools::NextDec(&m_txtCache->Page);
    else
      CDVDTeletextTools::PrevDec(&m_txtCache->Page);
    subp = m_txtCache->SubPageTable[m_txtCache->Page];
  } while (subp == 0xFF && m_txtCache->Page != m_LastPage);

  /* update Page */
  if (m_txtCache->Page != m_LastPage)
  {
    if (m_RenderInfo.ZoomMode == 2)
      m_RenderInfo.ZoomMode = 1;

    m_txtCache->SubPage     = subp;
    m_RenderInfo.HintMode   = false;
    m_txtCache->PageUpdate  = true;
  }
}

void CTeletextDecoder::GetNextSubPage(int offset)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* abort pageinput */
  m_RenderInfo.InputCounter = 2;

  for (int loop = m_txtCache->SubPage + offset; loop != m_txtCache->SubPage; loop += offset)
  {
    if (loop < 0)
      loop = 0x79;
    else if (loop > 0x79)
      loop = 0;
    if (loop == m_txtCache->SubPage)
      break;

    if (m_txtCache->astCachetable[m_txtCache->Page][loop])
    {
      /* enable manual SubPage zapping */
      m_txtCache->ZapSubpageManual = true;

      /* update page */
      if (m_RenderInfo.ZoomMode == 2) /* if zoomed to lower half */
        m_RenderInfo.ZoomMode = 1; /* activate upper half */

      m_txtCache->SubPage     = loop;
      m_RenderInfo.HintMode   = false;
      m_txtCache->PageUpdate  = true;

      return;
    }
  }
}

void CTeletextDecoder::SwitchZoomMode()
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  if (m_txtCache->SubPageTable[m_txtCache->Page] != 0xFF)
  {
    /* toggle mode */
    m_RenderInfo.ZoomMode++;

    if (m_RenderInfo.ZoomMode == 3)
      m_RenderInfo.ZoomMode = 0;

    /* update page */
    m_txtCache->PageUpdate = true;
  }
}

void CTeletextDecoder::SwitchTranspMode()
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* toggle mode */
  if (!m_RenderInfo.TranspMode)
    m_RenderInfo.TranspMode = true;
  else
    m_RenderInfo.TranspMode = false; /* backward to immediately switch to TV-screen */

  /* set mode */
  if (!m_RenderInfo.TranspMode) /* normal text-only */
  {
    ClearBB(m_txtCache->FullScrColor);
    m_txtCache->PageUpdate = true;
  }
  else /* semi-transparent BG with FG text */
  {
    ClearBB(TXT_ColorTransp);
    m_txtCache->PageUpdate = true;
  }
}

void CTeletextDecoder::SwitchHintMode()
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* toggle mode */
  m_RenderInfo.HintMode ^= true;

  if (!m_RenderInfo.HintMode)  /* toggle evaluation of level 2.5 information by explicitly switching off HintMode */
  {
    m_RenderInfo.Showl25 ^= true;
  }
  /* update page */
  m_txtCache->PageUpdate = true;
}

void CTeletextDecoder::ColorKey(int target)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  if (!target)
    return;

  if (m_RenderInfo.ZoomMode == 2)
    m_RenderInfo.ZoomMode = 1;

  m_LastPage                = m_txtCache->Page;
  m_txtCache->Page          = target;
  m_txtCache->SubPage       = m_txtCache->SubPageTable[m_txtCache->Page];
  m_RenderInfo.InputCounter = 2;
  m_RenderInfo.HintMode     = false;
  m_txtCache->PageUpdate    = true;
}

void CTeletextDecoder::StartPageCatching()
{
  m_RenderInfo.PageCatching = true;

  /* abort pageinput */
  m_RenderInfo.InputCounter = 2;

  /* show info line */
  m_RenderInfo.ZoomMode = 0;
  m_RenderInfo.PosX     = 0;
  m_RenderInfo.PosY     = 24*m_RenderInfo.FontHeight;

  /* check for pagenumber(s) */
  m_CatchRow            = 1;
  m_CatchCol            = 0;
  m_CatchedPage         = 0;
  m_PCOldRow            = 0;
  m_PCOldCol            = 0; /* no inverted page number to restore yet */
  CatchNextPage(0, 1);

  if (!m_CatchedPage)
  {
    std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

    m_RenderInfo.PageCatching = false;
    m_txtCache->PageUpdate    = true;
    return;
  }
}

void CTeletextDecoder::StopPageCatching()
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* set new page */
  if (m_RenderInfo.ZoomMode == 2)
    m_RenderInfo.ZoomMode = 1;

  m_LastPage                = m_txtCache->Page;
  m_txtCache->Page          = m_CatchedPage;
  m_RenderInfo.HintMode     = false;
  m_txtCache->PageUpdate    = true;
  m_RenderInfo.PageCatching = false;

  int subp = m_txtCache->SubPageTable[m_txtCache->Page];
  if (subp != 0xFF)
    m_txtCache->SubPage = subp;
  else
    m_txtCache->SubPage = 0;
}

void CTeletextDecoder::CatchNextPage(int firstlineinc, int inc)
{
  int tmp_page, allowwrap = 1; /* allow first wrap around */

  /* catch next page */
  for(;;)
  {
    unsigned char *p = &(m_RenderInfo.PageChar[m_CatchRow*40 + m_CatchCol]);
    TextPageAttr_t a = m_RenderInfo.PageAtrb[m_CatchRow*40 + m_CatchCol];

    if (!(a.charset == C_G1C || a.charset == C_G1S) && /* no mosaic */
       (a.fg != a.bg) && /* not hidden */
       (*p >= '1' && *p <= '8' && /* valid page number */
        *(p+1) >= '0' && *(p+1) <= '9' &&
        *(p+2) >= '0' && *(p+2) <= '9') &&
       (m_CatchRow == 0 || (*(p-1) < '0' || *(p-1) > '9')) && /* non-numeric char before and behind */
       (m_CatchRow == 37 || (*(p+3) < '0' || *(p+3) > '9')))
    {
      tmp_page = ((*p - '0')<<8) | ((*(p+1) - '0')<<4) | (*(p+2) - '0');

#if 0
      if (tmp_page != m_CatchedPage)  /* confusing to skip identical page numbers - I want to reach what I aim to */
#endif
      {
        m_CatchedPage = tmp_page;
        RenderCatchedPage();
        m_CatchCol += inc;  /* FIXME: limit */
        return;
      }
    }

    if (firstlineinc > 0)
    {
      m_CatchRow++;
      m_CatchCol = 0;
      firstlineinc = 0;
    }
    else if (firstlineinc < 0)
    {
      m_CatchRow--;
      m_CatchCol = 37;
      firstlineinc = 0;
    }
    else
      m_CatchCol += inc;

    if (m_CatchCol > 37)
    {
      m_CatchRow++;
      m_CatchCol = 0;
    }
    else if (m_CatchCol < 0)
    {
      m_CatchRow--;
      m_CatchCol = 37;
    }

    if (m_CatchRow > 23)
    {
      if (allowwrap)
      {
        allowwrap = 0;
        m_CatchRow = 1;
        m_CatchCol = 0;
      }
      else
      {
        return;
      }
    }
    else if (m_CatchRow < 1)
    {
      if (allowwrap)
      {
        allowwrap = 0;
        m_CatchRow = 23;
        m_CatchCol =37;
      }
      else
      {
        return;
      }
    }
  }
}

void CTeletextDecoder::RenderCatchedPage()
{
  int zoom = 0;
  m_updateTexture = true;

  /* handle zoom */
  if (m_RenderInfo.ZoomMode)
    zoom = 1<<10;

  if (m_PCOldRow || m_PCOldCol) /* not at first call */
  {
    /* restore pagenumber */
    SetPosX(m_PCOldCol);

    if (m_RenderInfo.ZoomMode == 2)
      m_RenderInfo.PosY = (m_PCOldRow-12)*m_RenderInfo.FontHeight*((zoom>>10)+1);
    else
      m_RenderInfo.PosY = m_PCOldRow*m_RenderInfo.FontHeight*((zoom>>10)+1);

    RenderCharFB(m_RenderInfo.PageChar[m_PCOldRow*40 + m_PCOldCol    ], &m_RenderInfo.PageAtrb[m_PCOldRow*40 + m_PCOldCol    ]);
    RenderCharFB(m_RenderInfo.PageChar[m_PCOldRow*40 + m_PCOldCol + 1], &m_RenderInfo.PageAtrb[m_PCOldRow*40 + m_PCOldCol + 1]);
    RenderCharFB(m_RenderInfo.PageChar[m_PCOldRow*40 + m_PCOldCol + 2], &m_RenderInfo.PageAtrb[m_PCOldRow*40 + m_PCOldCol + 2]);
  }

  m_PCOldRow = m_CatchRow;
  m_PCOldCol = m_CatchCol;

  /* mark pagenumber */
  if (m_RenderInfo.ZoomMode == 1 && m_CatchRow > 11)
  {
    m_RenderInfo.ZoomMode = 2;
    CopyBB2FB();
  }
  else if (m_RenderInfo.ZoomMode == 2 && m_CatchRow < 12)
  {
    m_RenderInfo.ZoomMode = 1;
    CopyBB2FB();
  }
  SetPosX(m_CatchCol);

  if (m_RenderInfo.ZoomMode == 2)
    m_RenderInfo.PosY = (m_CatchRow-12)*m_RenderInfo.FontHeight*((zoom>>10)+1);
  else
    m_RenderInfo.PosY = m_CatchRow*m_RenderInfo.FontHeight*((zoom>>10)+1);

  TextPageAttr_t a0 = m_RenderInfo.PageAtrb[m_CatchRow*40 + m_CatchCol    ];
  TextPageAttr_t a1 = m_RenderInfo.PageAtrb[m_CatchRow*40 + m_CatchCol + 1];
  TextPageAttr_t a2 = m_RenderInfo.PageAtrb[m_CatchRow*40 + m_CatchCol + 2];
  int t;

  /* exchange colors */
  t = a0.fg; a0.fg = a0.bg; a0.bg = t;
  t = a1.fg; a1.fg = a1.bg; a1.bg = t;
  t = a2.fg; a2.fg = a2.bg; a2.bg = t;

  RenderCharFB(m_RenderInfo.PageChar[m_CatchRow*40 + m_CatchCol    ], &a0);
  RenderCharFB(m_RenderInfo.PageChar[m_CatchRow*40 + m_CatchCol + 1], &a1);
  RenderCharFB(m_RenderInfo.PageChar[m_CatchRow*40 + m_CatchCol + 2], &a2);
}

void CTeletextDecoder::RenderPage()
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  int StartRow = 0;
  int national_subset_bak = m_txtCache->NationalSubset;

  if (m_txtCache->PageUpdate)
    m_updateTexture = true;

  /* update page or timestring */
  if (m_txtCache->PageUpdate && m_txtCache->PageReceiving != m_txtCache->Page && m_RenderInfo.InputCounter == 2)
  {
    /* reset update flag */
    m_txtCache->PageUpdate = false;
    if (m_RenderInfo.Boxed && m_RenderInfo.SubtitleDelay)
    {
      TextSubtitleCache_t* c = NULL;
      int j = -1;
      for (int i = 0; i < SUBTITLE_CACHESIZE; i++)
      {
        if (j == -1 && !m_RenderInfo.SubtitleCache[i])
          j = i;
        if (m_RenderInfo.SubtitleCache[i] && !m_RenderInfo.SubtitleCache[i]->Valid)
        {
          c = m_RenderInfo.SubtitleCache[i];
          break;
        }
      }
      if (c == NULL)
      {
        if (j == -1) // no more space in SubtitleCache
          return;

        c = new TextSubtitleCache_t;
        if (c == NULL)
          return;

        *c = {};
        m_RenderInfo.SubtitleCache[j] = c;
      }
      c->Valid = true;
      c->Timestamp = std::chrono::steady_clock::now();

      if (m_txtCache->SubPageTable[m_txtCache->Page] != 0xFF)
      {
        TextPageinfo_t * p = DecodePage(m_RenderInfo.Showl25, c->PageChar, c->PageAtrb, m_RenderInfo.HintMode, m_RenderInfo.ShowFlof);
        if (p)
        {
          m_RenderInfo.Boxed = p->boxed;
        }
      }
      m_RenderInfo.DelayStarted = true;
      return;
    }
    m_RenderInfo.DelayStarted = false;
    /* decode page */
    if (m_txtCache->SubPageTable[m_txtCache->Page] != 0xFF)
    {
      TextPageinfo_t * p = DecodePage(m_RenderInfo.Showl25, m_RenderInfo.PageChar, m_RenderInfo.PageAtrb, m_RenderInfo.HintMode, m_RenderInfo.ShowFlof);
      if (p)
      {
        m_RenderInfo.PageInfo = p;
        m_RenderInfo.Boxed = p->boxed;
      }
      if (m_RenderInfo.Boxed || m_RenderInfo.TranspMode)
        FillBorder(GetColorRGB(TXT_ColorTransp));
      else
        FillBorder(GetColorRGB((enumTeletextColor)m_txtCache->FullScrColor));

      if (m_txtCache->ColorTable)                   /* as late as possible to shorten the time the old page is displayed with the new colors */
        SetColors(m_txtCache->ColorTable, 16, 16);  /* set colors for CLUTs 2+3 */
    }
    else
      StartRow = 1;

    DoRenderPage(StartRow, national_subset_bak);
  }
  else
  {
    if (m_RenderInfo.DelayStarted)
    {
      auto now = std::chrono::steady_clock::now();
      for (TextSubtitleCache_t* const subtitleCache : m_RenderInfo.SubtitleCache)
      {
        if (subtitleCache && subtitleCache->Valid &&
            std::chrono::duration_cast<std::chrono::seconds>(now - subtitleCache->Timestamp)
                    .count() >= m_RenderInfo.SubtitleDelay)
        {
          memcpy(m_RenderInfo.PageChar, subtitleCache->PageChar, 40 * 25);
          memcpy(m_RenderInfo.PageAtrb, subtitleCache->PageAtrb, 40 * 25 * sizeof(TextPageAttr_t));
          DoRenderPage(StartRow, national_subset_bak);
          subtitleCache->Valid = false;
          return;
        }
      }
    }
    if (m_RenderInfo.ZoomMode != 2)
    {
      m_RenderInfo.PosY = 0;
      if (m_txtCache->SubPageTable[m_txtCache->Page] == 0xff)
      {
        m_RenderInfo.PageAtrb[32].fg = TXT_ColorYellow;
        m_RenderInfo.PageAtrb[32].bg = TXT_ColorMenu1;
        int showpage    = m_txtCache->PageReceiving;
        int showsubpage;

        // Verify that showpage is positive before any access to the array
        if (showpage >= 0 && (showsubpage = m_txtCache->SubPageTable[showpage]) != 0xff)
        {
          TextCachedPage_t *pCachedPage;
          pCachedPage = m_txtCache->astCachetable[showpage][showsubpage];
          if (pCachedPage && IsDec(showpage))
          {
            m_RenderInfo.PosX = 0;
            if (m_RenderInfo.InputCounter == 2)
            {
              if (m_txtCache->BTTok && !m_txtCache->BasicTop[m_txtCache->Page]) /* page non-existent according to TOP (continue search anyway) */
              {
                m_RenderInfo.PageAtrb[0].fg = TXT_ColorWhite;
                m_RenderInfo.PageAtrb[0].bg = TXT_ColorRed;
              }
              else
              {
                m_RenderInfo.PageAtrb[0].fg = TXT_ColorYellow;
                m_RenderInfo.PageAtrb[0].bg = TXT_ColorMenu1;
              }
              CDVDTeletextTools::Hex2Str((char*)m_RenderInfo.PageChar+3, m_txtCache->Page);

              int col;
              for (col = m_RenderInfo.nofirst; col < 7; col++) // selected page
              {
                RenderCharFB(m_RenderInfo.PageChar[col], &m_RenderInfo.PageAtrb[0]);
              }
              RenderCharFB(m_RenderInfo.PageChar[col], &m_RenderInfo.PageAtrb[32]);
            }
            else
              SetPosX(8);

            memcpy(&m_RenderInfo.PageChar[8], pCachedPage->p0, 24); /* header line without timestring */
            for (unsigned char i : pCachedPage->p0)
            {
              RenderCharFB(i, &m_RenderInfo.PageAtrb[32]);
            }

            /* Update on every Header number change */
            if (pCachedPage->p0[2] != prevHeaderPage)
            {
              prevHeaderPage = pCachedPage->p0[2];
              m_updateTexture = true;
            }
          }
        }
      }

      /* update timestring */
      SetPosX(32);
      for (int i = 0; i < 8; i++)
      {
        if (!m_RenderInfo.PageAtrb[32+i].flashing)
          RenderCharFB(m_txtCache->TimeString[i], &m_RenderInfo.PageAtrb[32]);
        else
        {
          SetPosX(33+i);
        }
      }
    }
    DoFlashing(StartRow);
    m_txtCache->NationalSubset = national_subset_bak;
  }
}

bool CTeletextDecoder::IsSubtitlePage(int pageNumber) const
{
  if (!m_txtCache)
    return false;

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  for (const auto subPage : m_txtCache->SubtitlePages)
  {
    if (subPage.page == pageNumber)
      return true;
  }

  return false;
}

void CTeletextDecoder::DoFlashing(int startrow)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  TextCachedPage_t* textCachepage =
      m_txtCache->astCachetable[m_txtCache->Page][m_txtCache->SubPage];

  // Verify that the page is not deleted by the other thread: CDVDTeletextData::ResetTeletextCache()
  if (!textCachepage || m_RenderInfo.PageInfo != &textCachepage->pageinfo)
    m_RenderInfo.PageInfo = nullptr;

  /* get national subset */
  if (m_txtCache->NationalSubset <= NAT_MAX_FROM_HEADER && /* not for GR/RU as long as line28 is not evaluated */
     m_RenderInfo.PageInfo && m_RenderInfo.PageInfo->nationalvalid) /* individual subset according to page header */
  {
    m_txtCache->NationalSubset = CountryConversionTable[m_RenderInfo.PageInfo->national];
  }

  /* Flashing */
  TextPageAttr_t flashattr;
  char flashchar;
  std::chrono::milliseconds flashphase = std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::steady_clock::now().time_since_epoch()) %
                                         1000;

  int srow = startrow;
  int erow = 24;
  int factor=1;

  switch (m_RenderInfo.ZoomMode)
  {
    case 1: erow = 12; factor=2;break;
    case 2: srow = 12; factor=2;break;
  }

  m_RenderInfo.PosY = startrow*m_RenderInfo.FontHeight*factor;
  for (int row = srow; row < erow; row++)
  {
    int index = row * 40;
    int dhset = 0;
    int incflash = 3;
    int decflash = 2;

    m_RenderInfo.PosX = 0;
    for (int col = m_RenderInfo.nofirst; col < 40; col++)
    {
      if (m_RenderInfo.PageAtrb[index + col].flashing && m_RenderInfo.PageChar[index + col] > 0x20 && m_RenderInfo.PageChar[index + col] != 0xff )
      {
        SetPosX(col);
        flashchar = m_RenderInfo.PageChar[index + col];
        bool doflash = false;
        memcpy(&flashattr, &m_RenderInfo.PageAtrb[index + col], sizeof(TextPageAttr_t));
        switch (flashattr.flashing &0x1c) // Flash Rate
        {
          case 0x00 :  // 1 Hz
            if (flashphase > 500ms)
              doflash = true;
            break;
          case 0x04 :  // 2 Hz  Phase 1
            if (flashphase < 250ms)
              doflash = true;
            break;
          case 0x08 :  // 2 Hz  Phase 2
            if (flashphase >= 250ms && flashphase < 500ms)
              doflash = true;
            break;
          case 0x0c :  // 2 Hz  Phase 3
            if (flashphase >= 500ms && flashphase < 750ms)
              doflash = true;
            break;
          case 0x10 :  // incremental flash
            incflash++;
            if (incflash>3) incflash = 1;
            switch (incflash)
            {
              case 1:
                if (flashphase < 250ms)
                  doflash = true;
                break;
              case 2:
                if (flashphase >= 250ms && flashphase < 500ms)
                  doflash = true;
                break;
              case 3:
                if (flashphase >= 500ms && flashphase < 750ms)
                  doflash = true;
                break;
            }
            break;
          case 0x14 :  // decremental flash
            decflash--;
            if (decflash<1) decflash = 3;
            switch (decflash)
            {
              case 1:
                if (flashphase < 250ms)
                  doflash = true;
                break;
              case 2:
                if (flashphase >= 250ms && flashphase < 500ms)
                  doflash = true;
                break;
              case 3:
                if (flashphase >= 500ms && flashphase < 750ms)
                  doflash = true;
                break;
            }
            break;

        }

        switch (flashattr.flashing &0x03) // Flash Mode
        {
          case 0x01 :  // normal Flashing
            if (doflash) flashattr.fg = flashattr.bg;
            break;
          case 0x02 :  // inverted Flashing
            doflash = !doflash;
            if (doflash) flashattr.fg = flashattr.bg;
            break;
          case 0x03 :  // color Flashing
            if (doflash) flashattr.fg = flashattr.fg + (flashattr.fg > 7 ? (-8) : 8);
            break;

        }
        RenderCharFB(flashchar, &flashattr);
        if (flashattr.doublew) col++;
        if (flashattr.doubleh) dhset = 1;

        m_updateTexture = true;
      }
    }
    if (dhset)
    {
      row++;
      m_RenderInfo.PosY += m_RenderInfo.FontHeight*factor;
    }
    m_RenderInfo.PosY += m_RenderInfo.FontHeight*factor;
  }
}

void CTeletextDecoder::DoRenderPage(int startrow, int national_subset_bak)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* display first column?  */
  m_RenderInfo.nofirst = m_RenderInfo.Show39;
  for (int row = 1; row < 24; row++)
  {
    int Byte = m_RenderInfo.PageChar[row*40];
    if (Byte != ' '  && Byte != 0x00 && Byte != 0xFF && m_RenderInfo.PageAtrb[row*40].fg != m_RenderInfo.PageAtrb[row*40].bg)
    {
      m_RenderInfo.nofirst = 0;
      break;
    }
  }
  m_RenderInfo.FontWidth_Normal = m_RenderInfo.Width / (m_RenderInfo.nofirst ? 39 : 40);
  SetFontWidth(m_RenderInfo.FontWidth_Normal);

  if (m_RenderInfo.TranspMode || m_RenderInfo.Boxed)
  {
    FillBorder(GetColorRGB(TXT_ColorTransp));//ClearBB(transp);
    m_RenderInfo.ClearBBColor = TXT_ColorTransp;
  }

  /* get national subset */
  if (m_txtCache->NationalSubset <= NAT_MAX_FROM_HEADER && /* not for GR/RU as long as line28 is not evaluated */
    m_RenderInfo.PageInfo && m_RenderInfo.PageInfo->nationalvalid) /* individual subset according to page header */
  {
    m_txtCache->NationalSubset = CountryConversionTable[m_RenderInfo.PageInfo->national];
  }
  /* render page */
  if (m_RenderInfo.PageInfo && (m_RenderInfo.PageInfo->function == FUNC_GDRCS || m_RenderInfo.PageInfo->function == FUNC_DRCS)) /* character definitions */
  {
    #define DRCSROWS 8
    #define DRCSCOLS (48/DRCSROWS)
    #define DRCSZOOMX 3
    #define DRCSZOOMY 5
    #define DRCSXSPC (12*DRCSZOOMX + 2)
    #define DRCSYSPC (10*DRCSZOOMY + 2)

    unsigned char ax[] = { /* array[0..12] of x-offsets, array[0..10] of y-offsets for each pixel */
      DRCSZOOMX * 0,
      DRCSZOOMX * 1,
      DRCSZOOMX * 2,
      DRCSZOOMX * 3,
      DRCSZOOMX * 4,
      DRCSZOOMX * 5,
      DRCSZOOMX * 6,
      DRCSZOOMX * 7,
      DRCSZOOMX * 8,
      DRCSZOOMX * 9,
      DRCSZOOMX * 10,
      DRCSZOOMX * 11,
      DRCSZOOMX * 12,
      DRCSZOOMY * 0,
      DRCSZOOMY * 1,
      DRCSZOOMY * 2,
      DRCSZOOMY * 3,
      DRCSZOOMY * 4,
      DRCSZOOMY * 5,
      DRCSZOOMY * 6,
      DRCSZOOMY * 7,
      DRCSZOOMY * 8,
      DRCSZOOMY * 9,
      DRCSZOOMY * 10
    };

    ClearBB(TXT_ColorBlack);
    for (int col = 0; col < 24*40; col++)
      m_RenderInfo.PageAtrb[col] = Text_AtrTable[ATR_WB];

    for (int row = 0; row < DRCSROWS; row++)
    {
      for (int col = 0; col < DRCSCOLS; col++)
      {
        RenderDRCS(m_RenderInfo.Width,
          m_RenderInfo.PageChar + 20 * (DRCSCOLS * row + col + 2),
          m_TextureBuffer
          + (m_RenderInfo.FontHeight + DRCSYSPC * row + m_RenderInfo.Height) * m_RenderInfo.Width
          + DRCSXSPC * col,
          ax, GetColorRGB(TXT_ColorWhite), GetColorRGB(TXT_ColorBlack));
      }
    }
    memset(m_RenderInfo.PageChar + 40, 0xff, 24*40); /* don't render any char below row 0 */
  }
  m_RenderInfo.PosY = startrow*m_RenderInfo.FontHeight;
  for (int row = startrow; row < 24; row++)
  {
    int index = row * 40;

    m_RenderInfo.PosX = 0;
    for (int col = m_RenderInfo.nofirst; col < 40; col++)
    {
      RenderCharBB(m_RenderInfo.PageChar[index + col], &m_RenderInfo.PageAtrb[index + col]);

      if (m_RenderInfo.PageAtrb[index + col].doubleh && m_RenderInfo.PageChar[index + col] != 0xff && row < 24-1)  /* disable lower char in case of doubleh setting in l25 objects */
        m_RenderInfo.PageChar[index + col + 40] = 0xff;
      if (m_RenderInfo.PageAtrb[index + col].doublew && col < 40-1)  /* skip next column if double width */
      {
        col++;
        if (m_RenderInfo.PageAtrb[index + col - 1].doubleh && m_RenderInfo.PageChar[index + col] != 0xff && row < 24-1)  /* disable lower char in case of doubleh setting in l25 objects */
          m_RenderInfo.PageChar[index + col + 40] = 0xff;
      }
    }
    m_RenderInfo.PosY += m_RenderInfo.FontHeight;
  }
  DoFlashing(startrow);

  /* update framebuffer */
  CopyBB2FB();
  m_txtCache->NationalSubset = national_subset_bak;
}

void CTeletextDecoder::Decode_BTT()
{
  /* basic top table */
  int current, b1, b2, b3, b4;
  unsigned char btt[23*40];

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  if (m_txtCache->SubPageTable[0x1f0] == 0xff || 0 == m_txtCache->astCachetable[0x1f0][m_txtCache->SubPageTable[0x1f0]]) /* not yet received */
    return;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  appPlayer->LoadPage(0x1f0, m_txtCache->SubPageTable[0x1f0], btt);
  if (btt[799] == ' ') /* not completely received or error */
    return;

  current = 0x100;
  for (int i = 0; i < 800; i++)
  {
    b1 = btt[i];
    if (b1 == ' ')
      b1 = 0;
    else
    {
      b1 = dehamming[b1];
      if (b1 == 0xFF) /* hamming error in btt */
      {
        btt[799] = ' '; /* mark btt as not received */
        return;
      }
    }
    m_txtCache->BasicTop[current] = b1;
    CDVDTeletextTools::NextDec(&current);
  }
  /* page linking table */
  m_txtCache->ADIP_PgMax = -1; /* rebuild table of adip pages */
  for (int i = 0; i < 10; i++)
  {
    b1 = dehamming[btt[800 + 8*i +0]];

    if (b1 == 0xE)
      continue; /* unused */
    else if (b1 == 0xF)
      break; /* end */

    b4 = dehamming[btt[800 + 8*i +7]];

    if (b4 != 2) /* only adip, ignore multipage (1) */
      continue;

    b2 = dehamming[btt[800 + 8*i +1]];
    b3 = dehamming[btt[800 + 8*i +2]];

    if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
    {
      CLog::Log(LOGERROR, "CTeletextDecoder::Decode_BTT <Biterror in btt/plt index {}>", i);
      btt[799] = ' '; /* mark btt as not received */
      return;
    }

    b1 = b1<<8 | b2<<4 | b3; /* page number */
    m_txtCache->ADIP_Pg[++m_txtCache->ADIP_PgMax] = b1;
  }

  m_txtCache->BTTok = true;
}

void CTeletextDecoder::Decode_ADIP() /* additional information table */
{
  int i, p, j, b1, b2, b3, charfound;
  unsigned char padip[23*40];

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  for (i = 0; i <= m_txtCache->ADIP_PgMax; i++)
  {
    p = m_txtCache->ADIP_Pg[i];
    if (!p || m_txtCache->SubPageTable[p] == 0xff || 0 == m_txtCache->astCachetable[p][m_txtCache->SubPageTable[p]]) /* not cached (avoid segfault) */
      continue;

    appPlayer->LoadPage(p, m_txtCache->SubPageTable[p], padip);
    for (j = 0; j < 44; j++)
    {
      b1 = dehamming[padip[20*j+0]];
      if (b1 == 0xE)
        continue; /* unused */

      if (b1 == 0xF)
        break; /* end */

      b2 = dehamming[padip[20*j+1]];
      b3 = dehamming[padip[20*j+2]];

      if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF)
      {
        CLog::Log(LOGERROR,
                  "CTeletextDecoder::Decode_BTT <Biterror in ait {:03x} {} {:02x} {:02x} {:02x} "
                  "{:02x} {:02x} {:02x}>",
                  p, j, padip[20 * j + 0], padip[20 * j + 1], padip[20 * j + 2], b1, b2, b3);
        return;
      }

      if (b1>8 || b2>9 || b3>9) /* ignore entries with invalid or hex page numbers */
      {
        continue;
      }

      b1 = b1<<8 | b2<<4 | b3; /* page number */
      charfound = 0; /* flag: no printable char found */

      for (b2 = 11; b2 >= 0; b2--)
      {
        b3 = deparity[padip[20*j + 8 + b2]];
        if (b3 < ' ')
          b3 = ' ';

        if (b3 == ' ' && !charfound)
          m_txtCache->ADIPTable[b1][b2] = '\0';
        else
        {
          m_txtCache->ADIPTable[b1][b2] = b3;
          charfound = 1;
        }
      }
    } /* next link j */

    m_txtCache->ADIP_Pg[i] = 0; /* completely decoded: clear entry */
  } /* next adip page i */

  while ((m_txtCache->ADIP_PgMax >= 0) && !m_txtCache->ADIP_Pg[m_txtCache->ADIP_PgMax]) /* and shrink table */
    m_txtCache->ADIP_PgMax--;
}

int CTeletextDecoder::TopText_GetNext(int startpage, int up, int findgroup)
{
  int current, nextgrp, nextblk;

  int stoppage =  (IsDec(startpage) ? startpage : startpage & 0xF00); // avoid endless loop in hexmode
  nextgrp = nextblk = 0;
  current = startpage;

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  do {
    if (up)
      CDVDTeletextTools::NextDec(&current);
    else
      CDVDTeletextTools::PrevDec(&current);

    if (!m_txtCache->BTTok || m_txtCache->BasicTop[current]) /* only if existent */
    {
      if (findgroup)
      {
        if (m_txtCache->BasicTop[current] >= 6 && m_txtCache->BasicTop[current] <= 7)
          return current;
        if (!nextgrp && (current&0x00F) == 0)
          nextgrp = current;
      }
      if (m_txtCache->BasicTop[current] >= 2 && m_txtCache->BasicTop[current] <= 5) /* always find block */
        return current;

      if (!nextblk && (current&0x0FF) == 0)
        nextblk = current;
    }
  } while (current != stoppage);

  if (nextgrp)
    return nextgrp;
  else if (nextblk)
    return nextblk;
  else
    return current;
}

void CTeletextDecoder::Showlink(int column, int linkpage)
{
  unsigned char line[] = "   >???   ";
  int oldfontwidth = m_RenderInfo.FontWidth;
  int yoffset;

  if (m_YOffset)
    yoffset = 0;
  else
    yoffset = m_RenderInfo.Height;

  int abx = ((m_RenderInfo.Width)%(40-m_RenderInfo.nofirst) == 0 ? m_RenderInfo.Width+1 : (m_RenderInfo.Width)/(((m_RenderInfo.Width)%(40-m_RenderInfo.nofirst)))+1);// distance between 'inserted' pixels
  int width = m_RenderInfo.Width /4;

  m_RenderInfo.PosY = 24*m_RenderInfo.FontHeight;

  if (m_RenderInfo.Boxed)
  {
    m_RenderInfo.PosX = column*width;
    FillRect(m_TextureBuffer, m_RenderInfo.Width, m_RenderInfo.PosX, m_RenderInfo.PosY+yoffset, m_RenderInfo.Width, m_RenderInfo.FontHeight, GetColorRGB(TXT_ColorTransp));
    return;
  }

  if (m_txtCache->ADIPTable[linkpage][0])
  {
    m_RenderInfo.PosX = column*width;
    int l = strlen(m_txtCache->ADIPTable[linkpage]);

    if (l > 9) /* smaller font, if no space for one half space at front and end */
      SetFontWidth(oldfontwidth * 10 / (l+1));

    FillRect(m_TextureBuffer, m_RenderInfo.Width, m_RenderInfo.PosX, m_RenderInfo.PosY+yoffset, width+(m_RenderInfo.Width%4), m_RenderInfo.FontHeight, GetColorRGB((enumTeletextColor)Text_AtrTable[ATR_L250 + column].bg));
    m_RenderInfo.PosX += ((width) - (l*m_RenderInfo.FontWidth+l*m_RenderInfo.FontWidth/abx))/2; /* center */

    for (char *p = m_txtCache->ADIPTable[linkpage]; *p; p++)
      RenderCharBB(*p, &Text_AtrTable[ATR_L250 + column]);

    SetFontWidth(oldfontwidth);
  }
  else /* display number */
  {
    m_RenderInfo.PosX = column*width;
    FillRect(m_TextureBuffer, m_RenderInfo.Width, m_RenderInfo.PosX, m_RenderInfo.PosY+yoffset, m_RenderInfo.Width-m_RenderInfo.PosX, m_RenderInfo.FontHeight, GetColorRGB((enumTeletextColor)Text_AtrTable[ATR_L250 + column].bg));
    if (linkpage < m_txtCache->Page)
    {
      line[6] = '<';
      CDVDTeletextTools::Hex2Str((char*)line + 5, linkpage);
    }
    else
      CDVDTeletextTools::Hex2Str((char*)line + 6, linkpage);

    for (unsigned char *p = line; p < line+9; p++)
      RenderCharBB(*p, &Text_AtrTable[ATR_L250 + column]);
  }
}

void CTeletextDecoder::CreateLine25()
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* btt completely received and not yet decoded */
  if (!m_txtCache->BTTok)
    Decode_BTT();

  if (m_txtCache->ADIP_PgMax >= 0)
    Decode_ADIP();

  if (!m_RenderInfo.ShowHex && m_RenderInfo.ShowFlof &&
     (m_txtCache->FlofPages[m_txtCache->Page][0] || m_txtCache->FlofPages[m_txtCache->Page][1] || m_txtCache->FlofPages[m_txtCache->Page][2] || m_txtCache->FlofPages[m_txtCache->Page][3])) // FLOF-Navigation present
  {
    m_RenderInfo.Prev_100 = m_txtCache->FlofPages[m_txtCache->Page][0];
    m_RenderInfo.Prev_10  = m_txtCache->FlofPages[m_txtCache->Page][1];
    m_RenderInfo.Next_10  = m_txtCache->FlofPages[m_txtCache->Page][2];
    m_RenderInfo.Next_100 = m_txtCache->FlofPages[m_txtCache->Page][3];

    m_RenderInfo.PosY = 24*m_RenderInfo.FontHeight;
    m_RenderInfo.PosX = 0;
    for (int i=m_RenderInfo.nofirst; i<40; i++)
      RenderCharBB(m_RenderInfo.PageChar[24*40 + i], &m_RenderInfo.PageAtrb[24*40 + i]);
  }
  else
  {
    /*  normal: blk-1, grp+1, grp+2, blk+1 */
    /*  hex:    hex+1, blk-1, grp+1, blk+1 */
    if (m_RenderInfo.ShowHex)
    {
      /* arguments: startpage, up, findgroup */
      m_RenderInfo.Prev_100 = NextHex(m_txtCache->Page);
      m_RenderInfo.Prev_10  = TopText_GetNext(m_txtCache->Page, 0, 0);
      m_RenderInfo.Next_10  = TopText_GetNext(m_txtCache->Page, 1, 1);
    }
    else
    {
      m_RenderInfo.Prev_100 = TopText_GetNext(m_txtCache->Page, 0, 0);
      m_RenderInfo.Prev_10  = TopText_GetNext(m_txtCache->Page, 1, 1);
      m_RenderInfo.Next_10  = TopText_GetNext(m_RenderInfo.Prev_10, 1, 1);
    }
    m_RenderInfo.Next_100 = TopText_GetNext(m_RenderInfo.Next_10, 1, 0);
    Showlink(0, m_RenderInfo.Prev_100);
    Showlink(1, m_RenderInfo.Prev_10);
    Showlink(2, m_RenderInfo.Next_10);
    Showlink(3, m_RenderInfo.Next_100);
  }
}

void CTeletextDecoder::RenderCharFB(int Char, TextPageAttr_t *Attribute)
{
  RenderCharIntern(&m_RenderInfo, Char, Attribute, m_RenderInfo.ZoomMode, m_YOffset);
}

void CTeletextDecoder::RenderCharBB(int Char, TextPageAttr_t *Attribute)
{
  RenderCharIntern(&m_RenderInfo, Char, Attribute, 0, m_RenderInfo.Height-m_YOffset);
}

void CTeletextDecoder::CopyBB2FB()
{
  UTILS::COLOR::Color *src, *dst, *topsrc;
  int screenwidth;
  UTILS::COLOR::Color fillcolor;

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* line 25 */
  if (!m_RenderInfo.PageCatching)
    CreateLine25();

  /* copy backbuffer to framebuffer */
  if (!m_RenderInfo.ZoomMode)
  {
    if (m_YOffset)
      m_YOffset = 0;
    else
      m_YOffset = m_RenderInfo.Height;

    if (m_RenderInfo.ClearBBColor >= 0)
    {
      m_RenderInfo.ClearBBColor = -1;
    }
    return;
  }

  src = dst = topsrc = m_TextureBuffer + m_RenderInfo.Width;

  if (m_YOffset)
  {
    dst += m_RenderInfo.Width * m_RenderInfo.Height;
  }
  else
  {
    src    += m_RenderInfo.Width * m_RenderInfo.Height;
    topsrc += m_RenderInfo.Width * m_RenderInfo.Height;
  }

  if (!m_RenderInfo.PageCatching)
    SDL_memcpy4(dst+(24*m_RenderInfo.FontHeight)*m_RenderInfo.Width, src + (24*m_RenderInfo.FontHeight)*m_RenderInfo.Width, m_RenderInfo.Width*m_RenderInfo.FontHeight); /* copy line25 in normal height */

  if (m_RenderInfo.TranspMode)
    fillcolor = GetColorRGB(TXT_ColorTransp);
  else
    fillcolor = GetColorRGB((enumTeletextColor)m_txtCache->FullScrColor);

  if (m_RenderInfo.ZoomMode == 2)
    src += 12*m_RenderInfo.FontHeight*m_RenderInfo.Width;

  screenwidth = m_RenderInfo.Width;

  for (int i = 12*m_RenderInfo.FontHeight; i; i--)
  {
    SDL_memcpy4(dst, src, screenwidth);
    dst += m_RenderInfo.Width;
    SDL_memcpy4(dst, src, screenwidth);
    dst += m_RenderInfo.Width;
    src += m_RenderInfo.Width;
  }

  for (int i = m_RenderInfo.Height - 25*m_RenderInfo.FontHeight; i >= 0;i--)
  {
    SDL_memset4(dst + m_RenderInfo.Width*(m_RenderInfo.FontHeight+i), fillcolor, screenwidth);
  }
}

FT_Error CTeletextDecoder::MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
  FT_Error result = FT_New_Face(library, (const char*)face_id, 0, aface);

  if (!result)
    CLog::Log(LOGINFO, "Teletext font {} loaded", (char*)face_id);
  else
    CLog::Log(LOGERROR, "Opening of Teletext font {} failed", (char*)face_id);

  return result;
}

void CTeletextDecoder::SetFontWidth(int newWidth)
{
  if (m_RenderInfo.FontWidth != newWidth)
  {
    m_RenderInfo.FontWidth = newWidth;
    m_TypeTTF.width       = (FT_UShort) m_RenderInfo.FontWidth;

    for (int i = 0; i <= 12; i++)
      m_RenderInfo.axdrcs[i] = (m_RenderInfo.FontWidth * i + 6) / 12;
  }
}

int CTeletextDecoder::GetCurFontWidth()
{
  int mx  = (m_RenderInfo.Width)%(40-m_RenderInfo.nofirst);                 // # of unused pixels
  int abx = (mx == 0 ? m_RenderInfo.Width+1 : (m_RenderInfo.Width)/(mx+1)); // distance between 'inserted' pixels
  int nx  = abx+1-(m_RenderInfo.PosX % (abx+1));                            // # of pixels to next insert
  return m_RenderInfo.FontWidth+(((m_RenderInfo.PosX+m_RenderInfo.FontWidth+1) <= m_RenderInfo.Width && nx <= m_RenderInfo.FontWidth+1) ? 1 : 0);
}

void CTeletextDecoder::SetPosX(int column)
{
  m_RenderInfo.PosX = 0;

  for (int i = 0; i < column-m_RenderInfo.nofirst; i++)
    m_RenderInfo.PosX += GetCurFontWidth();
}

void CTeletextDecoder::ClearBB(UTILS::COLOR::Color Color)
{
  SDL_memset4(m_TextureBuffer + (m_RenderInfo.Height-m_YOffset)*m_RenderInfo.Width, Color, m_RenderInfo.Width*m_RenderInfo.Height);
}

void CTeletextDecoder::ClearFB(UTILS::COLOR::Color Color)
{
  SDL_memset4(m_TextureBuffer + m_RenderInfo.Width*m_YOffset, Color, m_RenderInfo.Width*m_RenderInfo.Height);
}

void CTeletextDecoder::FillBorder(UTILS::COLOR::Color Color)
{
  FillRect(m_TextureBuffer + (m_RenderInfo.Height-m_YOffset)*m_RenderInfo.Width, m_RenderInfo.Width, 0, 25*m_RenderInfo.FontHeight, m_RenderInfo.Width, m_RenderInfo.Height-(25*m_RenderInfo.FontHeight), Color);
  FillRect(m_TextureBuffer + m_RenderInfo.Width*m_YOffset, m_RenderInfo.Width, 0, 25*m_RenderInfo.FontHeight, m_RenderInfo.Width, m_RenderInfo.Height-(25*m_RenderInfo.FontHeight), Color);
}

void CTeletextDecoder::FillRect(
    UTILS::COLOR::Color* buffer, int xres, int x, int y, int w, int h, UTILS::COLOR::Color Color)
{
  if (!buffer) return;

  UTILS::COLOR::Color* p = buffer + x + y * xres;

  if (w > 0)
  {
    for ( ; h > 0 ; h--)
    {
      SDL_memset4(p, Color, w);
      p += xres;
    }
  }
}

void CTeletextDecoder::DrawVLine(
    UTILS::COLOR::Color* lfb, int xres, int x, int y, int l, UTILS::COLOR::Color color)
{
  if (!lfb) return;
  UTILS::COLOR::Color* p = lfb + x + y * xres;

  for ( ; l > 0 ; l--)
  {
    *p = color;
    p += xres;
  }
}

void CTeletextDecoder::DrawHLine(
    UTILS::COLOR::Color* lfb, int xres, int x, int y, int l, UTILS::COLOR::Color color)
{
  if (!lfb) return;
  if (l > 0)
    SDL_memset4(lfb + x + y * xres, color, l);
}

void CTeletextDecoder::RenderDRCS(
    int xres,
    unsigned char* s, /* pointer to char data, parity undecoded */
    UTILS::COLOR::Color* d, /* pointer to frame buffer of top left pixel */
    unsigned char* ax, /* array[0..12] of x-offsets, array[0..10] of y-offsets for each pixel */
    UTILS::COLOR::Color fgcolor,
    UTILS::COLOR::Color bgcolor)
{
  if (d == NULL) return;

  unsigned char *ay = ax + 13; /* array[0..10] of y-offsets for each pixel */

  for (int y = 0; y < 10; y++) /* 10*2 bytes a 6 pixels per char definition */
  {
    unsigned char c1 = deparity[*s++];
    unsigned char c2 = deparity[*s++];
    int h = ay[y+1] - ay[y];

    if (!h)
      continue;
    if (((c1 == ' ') && (*(s-2) != ' ')) || ((c2 == ' ') && (*(s-1) != ' '))) /* parity error: stop decoding FIXME */
      return;
    for (int bit = 0x20, x = 0;
        bit;
        bit >>= 1, x++)  /* bit mask (MSB left), column counter */
    {
      UTILS::COLOR::Color f1 = (c1 & bit) ? fgcolor : bgcolor;
      UTILS::COLOR::Color f2 = (c2 & bit) ? fgcolor : bgcolor;
      for (int i = 0; i < h; i++)
      {
        if (ax[x+1] > ax[x])
          SDL_memset4(d + ax[x], f1, ax[x+1] - ax[x]);
        if (ax[x+7] > ax[x+6])
          SDL_memset4(d + ax[x+6], f2, ax[x+7] - ax[x+6]); /* 2nd byte 6 pixels to the right */
        d += xres;
      }
      d -= h * xres;
    }
    d += h * xres;
  }
}

void CTeletextDecoder::FillRectMosaicSeparated(UTILS::COLOR::Color* lfb,
                                               int xres,
                                               int x,
                                               int y,
                                               int w,
                                               int h,
                                               UTILS::COLOR::Color fgcolor,
                                               UTILS::COLOR::Color bgcolor,
                                               int set)
{
  if (!lfb) return;
  FillRect(lfb,xres,x, y, w, h, bgcolor);
  if (set)
  {
    FillRect(lfb,xres,x+1, y+1, w-2, h-2, fgcolor);
  }
}

void CTeletextDecoder::FillTrapez(UTILS::COLOR::Color* lfb,
                                  int xres,
                                  int x0,
                                  int y0,
                                  int l0,
                                  int xoffset1,
                                  int h,
                                  int l1,
                                  UTILS::COLOR::Color color)
{
  UTILS::COLOR::Color* p = lfb + x0 + y0 * xres;
  int xoffset, l;

  for (int yoffset = 0; yoffset < h; yoffset++)
  {
    l = l0 + ((l1-l0) * yoffset + h/2) / h;
    xoffset = (xoffset1 * yoffset + h/2) / h;
    if (l > 0)
      SDL_memset4(p + xoffset, color, l);
    p += xres;
  }
}

void CTeletextDecoder::FlipHorz(UTILS::COLOR::Color* lfb, int xres, int x, int y, int w, int h)
{
  UTILS::COLOR::Color buf[2048];
  UTILS::COLOR::Color* p = lfb + x + y * xres;
  int w1,h1;

  for (h1 = 0 ; h1 < h ; h1++)
  {
    SDL_memcpy4(buf,p,w);
    for (w1 = 0 ; w1 < w ; w1++)
    {
      *(p+w1) = buf[w-(w1+1)];
    }
    p += xres;
  }
}

void CTeletextDecoder::FlipVert(UTILS::COLOR::Color* lfb, int xres, int x, int y, int w, int h)
{
  UTILS::COLOR::Color buf[2048];
  UTILS::COLOR::Color *p = lfb + x + y * xres, *p1, *p2;
  int h1;

  for (h1 = 0 ; h1 < h/2 ; h1++)
  {
    p1 = (p+(h1*xres));
    p2 = (p+(h-(h1+1))*xres);
    SDL_memcpy4(buf, p1, w);
    SDL_memcpy4(p1, p2, w);
    SDL_memcpy4(p2, buf, w);
  }
}

int CTeletextDecoder::ShapeCoord(int param, int curfontwidth, int curFontHeight)
{
  switch (param)
  {
  case S_W13:
    return curfontwidth/3;
  case S_W12:
    return curfontwidth/2;
  case S_W23:
    return curfontwidth*2/3;
  case S_W11:
    return curfontwidth;
  case S_WM3:
    return curfontwidth-3;
  case S_H13:
    return curFontHeight/3;
  case S_H12:
    return curFontHeight/2;
  case S_H23:
    return curFontHeight*2/3;
  case S_H11:
    return curFontHeight;
  default:
    return param;
  }
}

void CTeletextDecoder::DrawShape(UTILS::COLOR::Color* lfb,
                                 int xres,
                                 int x,
                                 int y,
                                 int shapenumber,
                                 int curfontwidth,
                                 int FontHeight,
                                 int curFontHeight,
                                 UTILS::COLOR::Color fgcolor,
                                 UTILS::COLOR::Color bgcolor,
                                 bool clear)
{
  if (!lfb || shapenumber < 0x20 || shapenumber > 0x7e || (shapenumber == 0x7e && clear))
    return;

  unsigned char *p = aShapes[shapenumber - 0x20];

  if (*p == S_INV)
  {
    int t = fgcolor;
    fgcolor = bgcolor;
    bgcolor = t;
    p++;
  }

  if (clear)
    FillRect(lfb, xres, x, y, curfontwidth, FontHeight, bgcolor);

  while (*p != S_END)
  {
    switch (*p++)
    {
    case S_FHL:
    {
      int offset = ShapeCoord(*p++, curfontwidth, curFontHeight);
      DrawHLine(lfb, xres, x, y + offset, curfontwidth, fgcolor);
      break;
    }
    case S_FVL:
    {
      int offset = ShapeCoord(*p++, curfontwidth, curFontHeight);
      DrawVLine(lfb,xres,x + offset, y, FontHeight, fgcolor);
      break;
    }
    case S_FLH:
      FlipHorz(lfb,xres,x,y,curfontwidth, FontHeight);
      break;
    case S_FLV:
      FlipVert(lfb,xres,x,y,curfontwidth, FontHeight);
      break;
    case S_BOX:
    {
      int xo = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int yo = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int w = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int h = ShapeCoord(*p++, curfontwidth, curFontHeight);
      FillRect(lfb,xres,x + xo, y + yo, w, h, fgcolor);
      break;
    }
    case S_TRA:
    {
      int x0 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int y0 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int l0 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int x1 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int y1 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int l1 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      FillTrapez(lfb, xres,x + x0, y + y0, l0, x1-x0, y1-y0, l1, fgcolor);
      break;
    }
    case S_BTR:
    {
      int x0 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int y0 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int l0 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int x1 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int y1 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      int l1 = ShapeCoord(*p++, curfontwidth, curFontHeight);
      FillTrapez(lfb, xres, x + x0, y + y0, l0, x1-x0, y1-y0, l1, bgcolor);
      break;
    }
    case S_LNK:
    {
      DrawShape(lfb,xres,x, y, ShapeCoord(*p, curfontwidth, curFontHeight), curfontwidth, FontHeight, curFontHeight, fgcolor, bgcolor, false);
      break;
    }
    default:
      break;
    }
  }
}

void CTeletextDecoder::RenderCharIntern(TextRenderInfo_t* RenderInfo, int Char, TextPageAttr_t *Attribute, int zoom, int yoffset)
{
  int Row, Pitch;
  int glyph;
  UTILS::COLOR::Color bgcolor, fgcolor;
  int factor, xfactor;
  unsigned char *sbitbuffer;

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  int national_subset_local = m_txtCache->NationalSubset;
  int curfontwidth          = GetCurFontWidth();
  int t                     = curfontwidth;
  m_RenderInfo.PosX        += t;
  int curfontwidth2         = GetCurFontWidth();
  m_RenderInfo.PosX        -= t;
  int alphachar             = RenderChar(m_TextureBuffer+(yoffset)*m_RenderInfo.Width, m_RenderInfo.Width, Char, &m_RenderInfo.PosX, m_RenderInfo.PosY, Attribute, zoom > 0, curfontwidth, curfontwidth2, m_RenderInfo.FontHeight, m_RenderInfo.TranspMode, m_RenderInfo.axdrcs, m_Ascender);
  if (alphachar <= 0) return;

  if (zoom && Attribute->doubleh)
    factor = 4;
  else if (zoom || Attribute->doubleh)
    factor = 2;
  else
    factor = 1;

  fgcolor = GetColorRGB((enumTeletextColor)Attribute->fg);
  if (m_RenderInfo.TranspMode && m_RenderInfo.PosY < 24*m_RenderInfo.FontHeight)
  {
    bgcolor = GetColorRGB(TXT_ColorTransp);
  }
  else
  {
    bgcolor = GetColorRGB((enumTeletextColor)Attribute->bg);
  }

  if (Attribute->doublew)
  {
    curfontwidth += curfontwidth2;
    xfactor = 2;
  }
  else
    xfactor = 1;

  // Check if the alphanumeric char has diacritical marks (or results from composing chars) or
  // on the other hand it is just a simple alphanumeric char
  if (!Attribute->diacrit)
  {
    Char = alphachar;
  }
  else
  {
    if ((national_subset_local == NAT_SC) || (national_subset_local == NAT_RB) ||
        (national_subset_local == NAT_UA))
      Char = G2table[1][0x20 + Attribute->diacrit];
    else if (national_subset_local == NAT_GR)
      Char = G2table[2][0x20 + Attribute->diacrit];
    else if (national_subset_local == NAT_HB)
      Char = G2table[3][0x20 + Attribute->diacrit];
    else if (national_subset_local == NAT_AR)
      Char = G2table[4][0x20 + Attribute->diacrit];
    else
      Char = G2table[0][0x20 + Attribute->diacrit];

    // use harfbuzz to combine the diacritical mark with the alphanumeric char
    // fallback to the alphanumeric char if composition fails
    hb_unicode_funcs_t* ufuncs = hb_unicode_funcs_get_default();
    hb_codepoint_t composedChar;
    const hb_bool_t isComposed = hb_unicode_compose(ufuncs, alphachar, Char, &composedChar);
    Char = isComposed ? composedChar : alphachar;
  }

  /* render char */
  if (!(glyph = FT_Get_Char_Index(m_Face, Char)))
  {
    CLog::Log(LOGERROR, "{}:  <FT_Get_Char_Index for Char {:x} \"{}\" failed", __FUNCTION__,
              alphachar, alphachar);

    FillRect(m_TextureBuffer, m_RenderInfo.Width, m_RenderInfo.PosX, m_RenderInfo.PosY + yoffset, curfontwidth, factor*m_RenderInfo.FontHeight, bgcolor);
    m_RenderInfo.PosX += curfontwidth;
    return;
  }

  if (FTC_SBitCache_Lookup(m_Cache, &m_TypeTTF, glyph, &m_sBit, &m_anode) != 0)
  {
    FillRect(m_TextureBuffer, m_RenderInfo.Width, m_RenderInfo.PosX, m_RenderInfo.PosY + yoffset, curfontwidth, m_RenderInfo.FontHeight, bgcolor);
    m_RenderInfo.PosX += curfontwidth;
    return;
  }

  sbitbuffer = m_sBit->buffer;

  int backupTTFshiftY = m_RenderInfo.TTFShiftY;
  if (national_subset_local == NAT_AR)
      m_RenderInfo.TTFShiftY = backupTTFshiftY - 2; // for arabic TTF font should be shifted up slightly

  UTILS::COLOR::Color* p;
  int f; /* running counter for zoom factor */
  int he = m_sBit->height; // sbit->height should not be altered, I guess
  Row = factor * (m_Ascender - m_sBit->top + m_RenderInfo.TTFShiftY);
  if (Row < 0)
  {
    sbitbuffer  -= m_sBit->pitch*Row;
    he += Row;
    Row = 0;
  }
  else
  {
    FillRect(m_TextureBuffer, m_RenderInfo.Width, m_RenderInfo.PosX, m_RenderInfo.PosY + yoffset, curfontwidth, Row, bgcolor); /* fill upper margin */
  }

  if (m_Ascender - m_sBit->top + m_RenderInfo.TTFShiftY + he > m_RenderInfo.FontHeight)
    he = m_RenderInfo.FontHeight - m_Ascender + m_sBit->top - m_RenderInfo.TTFShiftY; /* limit char height to defined/calculated FontHeight */
  if (he < 0) he = m_RenderInfo.FontHeight;

  p = m_TextureBuffer + m_RenderInfo.PosX + (yoffset + m_RenderInfo.PosY + Row) * m_RenderInfo.Width; /* running pointer into framebuffer */
  for (Row = he; Row; Row--) /* row counts up, but down may be a little faster :) */
  {
    int pixtodo = m_sBit->width;
    UTILS::COLOR::Color* pstart = p;

    for (int Bit = xfactor * (m_sBit->left + m_RenderInfo.TTFShiftX); Bit > 0; Bit--) /* fill left margin */
    {
      for (f = factor-1; f >= 0; f--)
        *(p + f*m_RenderInfo.Width) = bgcolor;
      p++;
    }

    for (Pitch = m_sBit->pitch; Pitch; Pitch--)
    {
      for (int Bit = 0x80; Bit; Bit >>= 1)
      {
        UTILS::COLOR::Color color;

        if (--pixtodo < 0)
          break;

        if (*sbitbuffer & Bit) /* bit set -> foreground */
          color = fgcolor;
        else /* bit not set -> background */
          color = bgcolor;

        for (f = factor-1; f >= 0; f--)
          *(p + f*m_RenderInfo.Width) = color;
        p++;

        if (xfactor > 1) /* double width */
        {
          for (f = factor-1; f >= 0; f--)
            *(p + f*m_RenderInfo.Width) = color;
          p++;
        }
      }
      sbitbuffer++;
    }
    for (int Bit = (curfontwidth - xfactor*(m_sBit->width + m_sBit->left + m_RenderInfo.TTFShiftX));
        Bit > 0; Bit--) /* fill rest of char width */
    {
      for (f = factor-1; f >= 0; f--)
        *(p + f*m_RenderInfo.Width) = bgcolor;
      p++;
    }

    p = pstart + factor*m_RenderInfo.Width;
  }

  Row = m_Ascender - m_sBit->top + he + m_RenderInfo.TTFShiftY;
  FillRect(m_TextureBuffer,
           m_RenderInfo.Width,
           m_RenderInfo.PosX,
           m_RenderInfo.PosY + yoffset + Row * factor,
           curfontwidth,
           (m_RenderInfo.FontHeight - Row) * factor,
           bgcolor); /* fill lower margin */

  if (Attribute->underline)
    FillRect(m_TextureBuffer,
            m_RenderInfo.Width,
            m_RenderInfo.PosX,
            m_RenderInfo.PosY + yoffset + (m_RenderInfo.FontHeight-2)* factor,
            curfontwidth,
            2*factor,
            fgcolor); /* underline char */

  m_RenderInfo.PosX      += curfontwidth;
  m_RenderInfo.TTFShiftY  = backupTTFshiftY; // restore TTFShiftY
}

int CTeletextDecoder::RenderChar(
    UTILS::COLOR::Color* buffer, // pointer to render buffer, min. FontHeight*2*xres
    int xres, // length of 1 line in render buffer
    int Char, // character to render
    int*
        pPosX, // left border for rendering relative to *buffer, will be set to right border after rendering
    int PosY, // vertical position of char in *buffer
    TextPageAttr_t* Attribute, // Attributes of Char
    bool zoom, // 1= character will be rendered in double height
    int curfontwidth, // rendering width of character
    int curfontwidth2, // rendering width of next character (needed for doublewidth)
    int FontHeight, // height of character
    bool transpmode, // 1= transparent display
    unsigned char* axdrcs, // width and height of DRCS-chars
    int Ascender) // Ascender of font
{
  UTILS::COLOR::Color bgcolor, fgcolor;
  int factor, xfactor;

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  int national_subset_local = m_txtCache->NationalSubset;
  int ymosaic[4];
  ymosaic[0] = 0; /* y-offsets for 2*3 mosaic */
  ymosaic[1] = (FontHeight + 1) / 3;
  ymosaic[2] = (FontHeight * 2 + 1) / 3;
  ymosaic[3] = FontHeight;

  if (Attribute->setX26)
  {
    national_subset_local = 0; // no national subset
  }

  // G0+G2 set designation
  if (Attribute->setG0G2 != 0x3f)
  {
    switch (Attribute->setG0G2)
    {
      case 0x20 :
        national_subset_local = NAT_SC;
        break;
      case 0x24 :
        national_subset_local = NAT_RB;
        break;
      case 0x25 :
        national_subset_local = NAT_UA;
        break;
      case 0x37:
        national_subset_local = NAT_GR;
        break;
      case 0x55:
        national_subset_local = NAT_HB;
        break;
      case 0x47:
      case 0x57:
        national_subset_local = NAT_AR;
        break;
      default:
        national_subset_local = CountryConversionTable[Attribute->setG0G2 & 0x07];
        break;
    }
  }

  if (Attribute->charset == C_G0S) // use secondary charset
    national_subset_local = m_txtCache->NationalSubsetSecondary;
  if (zoom && Attribute->doubleh)
    factor = 4;
  else if (zoom || Attribute->doubleh)
    factor = 2;
  else
    factor = 1;

  if (Attribute->doublew)
  {
    curfontwidth += curfontwidth2;
    xfactor = 2;
  }
  else
    xfactor = 1;

  if (Char == 0xFF)  /* skip doubleheight chars in lower line */
  {
    *pPosX += curfontwidth;
    return -1;
  }

  /* get colors */
  if (Attribute->inverted)
  {
    int t = Attribute->fg;
    Attribute->fg = Attribute->bg;
    Attribute->bg = t;
  }
  fgcolor = GetColorRGB((enumTeletextColor)Attribute->fg);
  if (transpmode == true && PosY < 24*FontHeight)
  {
    bgcolor = GetColorRGB(TXT_ColorTransp);
  }
  else
  {
    bgcolor = GetColorRGB((enumTeletextColor)Attribute->bg);
  }

  /* handle mosaic */
  if ((Attribute->charset == C_G1C || Attribute->charset == C_G1S) &&
     ((Char&0xA0) == 0x20))
  {
    int w1 = (curfontwidth / 2 ) *xfactor;
    int w2 = (curfontwidth - w1) *xfactor;

    Char = (Char & 0x1f) | ((Char & 0x40) >> 1);
    if (Attribute->charset == C_G1S) /* separated mosaic */
    {
      for (int y = 0; y < 3; y++)
      {
        FillRectMosaicSeparated(buffer, xres,*pPosX,      PosY +  ymosaic[y]*factor, w1, (ymosaic[y+1] - ymosaic[y])*factor, fgcolor, bgcolor, Char & 0x01);
        FillRectMosaicSeparated(buffer, xres,*pPosX + w1, PosY +  ymosaic[y]*factor, w2, (ymosaic[y+1] - ymosaic[y])*factor, fgcolor, bgcolor, Char & 0x02);
        Char >>= 2;
      }
    }
    else
    {
      for (int y = 0; y < 3; y++)
      {
        FillRect(buffer, xres, *pPosX,      PosY + ymosaic[y]*factor, w1, (ymosaic[y+1] - ymosaic[y])*factor, (Char & 0x01) ? fgcolor : bgcolor);
        FillRect(buffer, xres, *pPosX + w1, PosY + ymosaic[y]*factor, w2, (ymosaic[y+1] - ymosaic[y])*factor, (Char & 0x02) ? fgcolor : bgcolor);
        Char >>= 2;
      }
    }

    *pPosX += curfontwidth;
    return 0;
  }

  if (Attribute->charset == C_G3)
  {
    if (Char < 0x20 || Char > 0x7d)
    {
      Char = 0x20;
    }
    else
    {
      if (*aShapes[Char - 0x20] == S_CHR)
      {
        unsigned char *p = aShapes[Char - 0x20];
        Char = (*(p+1) <<8) + (*(p+2));
      }
      else if (*aShapes[Char - 0x20] == S_ADT)
      {
        if (buffer)
        {
          int x,y,f,c;
          UTILS::COLOR::Color* p = buffer + *pPosX + PosY * xres;
          for (y=0; y<FontHeight;y++)
          {
            for (f=0; f<factor; f++)
            {
              for (x=0; x<curfontwidth*xfactor;x++)
              {
                c = (y&4 ? (x/3)&1 :((x+3)/3)&1);
                *(p+x) = (c ? fgcolor : bgcolor);
              }
              p += xres;
            }
          }
        }
        *pPosX += curfontwidth;
        return 0;
      }
      else
      {
        DrawShape(buffer, xres,*pPosX, PosY, Char, curfontwidth, FontHeight, factor*FontHeight, fgcolor, bgcolor, true);
        *pPosX += curfontwidth;
        return 0;
      }
    }
  }
  else if (Attribute->charset >= C_OFFSET_DRCS)
  {
    TextCachedPage_t *pcache = m_txtCache->astCachetable[(Attribute->charset & 0x10) ? m_txtCache->drcs : m_txtCache->gdrcs][Attribute->charset & 0x0f];
    if (pcache)
    {
      unsigned char drcs_data[23*40];
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      appPlayer->LoadPage((Attribute->charset & 0x10) ? m_txtCache->drcs : m_txtCache->gdrcs,
                          Attribute->charset & 0x0f, drcs_data);
      unsigned char *p;
      if (Char < 23*2)
        p = drcs_data + 20*Char;
      else if (pcache->pageinfo.p24)
        p = pcache->pageinfo.p24 + 20*(Char - 23*2);
      else
      {
        FillRect(buffer, xres,*pPosX, PosY, curfontwidth, factor*FontHeight, bgcolor);
        *pPosX += curfontwidth;
        return 0;
      }
      axdrcs[12] = curfontwidth; /* adjust last x-offset according to position, FIXME: double width */
      RenderDRCS(xres, p, buffer + *pPosX + PosY * xres, axdrcs, fgcolor, bgcolor);
    }
    else
    {
      FillRect(buffer,xres,*pPosX, PosY, curfontwidth, factor*FontHeight, bgcolor);
    }
    *pPosX += curfontwidth;
    return 0;
  }
  else if (Attribute->charset == C_G2 && Char >= 0x20 && Char <= 0x7F)
  {
    if ((national_subset_local == NAT_SC) || (national_subset_local == NAT_RB) || (national_subset_local == NAT_UA))
      Char = G2table[1][Char-0x20];
    else if (national_subset_local == NAT_GR)
      Char = G2table[2][Char-0x20];
    else if (national_subset_local == NAT_AR)
      Char = G2table[3][Char-0x20];
    else
      Char = G2table[0][Char-0x20];

    //if (Char == 0x7F)
    //{
    //  FillRect(buffer,xres,*pPosX, PosY, curfontwidth, factor*Ascender, fgcolor);
    //  FillRect(buffer,xres,*pPosX, PosY + factor*Ascender, curfontwidth, factor*(FontHeight-Ascender), bgcolor);
    //  *pPosX += curfontwidth;
    //  return 0;
    //}
  }
  else if (national_subset_local == NAT_SC && Char >= 0x20 && Char <= 0x7F) /* remap complete areas for serbian/croatian */
    Char = G0table[0][Char-0x20];
  else if (national_subset_local == NAT_RB && Char >= 0x20 && Char <= 0x7F) /* remap complete areas for russian/bulgarian */
    Char = G0table[1][Char-0x20];
  else if (national_subset_local == NAT_UA && Char >= 0x20 && Char <= 0x7F) /* remap complete areas for ukrainian */
    Char = G0table[2][Char-0x20];
  else if (national_subset_local == NAT_GR && Char >= 0x20 && Char <= 0x7F) /* remap complete areas for greek */
    Char = G0table[3][Char-0x20];
  else if (national_subset_local == NAT_HB && Char >= 0x20 && Char <= 0x7F) /* remap complete areas for hebrew */
    Char = G0table[4][Char-0x20];
  else if (national_subset_local == NAT_AR && Char >= 0x20 && Char <= 0x7F) /* remap complete areas for arabic */
    Char = G0table[5][Char-0x20];
  else
  {
    /* load char */
    switch (Char)
    {
    case 0x00:
    case 0x20:
      FillRect(buffer, xres, *pPosX, PosY, curfontwidth, factor*FontHeight, bgcolor);
      *pPosX += curfontwidth;
      return -3;
    case 0x23:
    case 0x24:
      Char = nationaltable23[national_subset_local][Char-0x23];
      break;
    case 0x40:
      Char = nationaltable40[national_subset_local];
      break;
    case 0x5B:
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
    case 0x60:
      Char = nationaltable5b[national_subset_local][Char-0x5B];
      break;
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
      Char = nationaltable7b[national_subset_local][Char-0x7B];
      break;
    case 0x7F:
      FillRect(buffer,xres,*pPosX, PosY , curfontwidth, factor*Ascender, fgcolor);
      FillRect(buffer,xres,*pPosX, PosY + factor*Ascender, curfontwidth, factor*(FontHeight-Ascender), bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE0: /* |- */
      DrawHLine(buffer,xres,*pPosX, PosY, curfontwidth, fgcolor);
      DrawVLine(buffer,xres,*pPosX, PosY +1, FontHeight -1, fgcolor);
      FillRect(buffer,xres,*pPosX +1, PosY +1, curfontwidth-1, FontHeight-1, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE1: /* - */
      DrawHLine(buffer,xres,*pPosX, PosY, curfontwidth, fgcolor);
      FillRect(buffer,xres,*pPosX, PosY +1, curfontwidth, FontHeight-1, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE2: /* -| */
      DrawHLine(buffer,xres,*pPosX, PosY, curfontwidth, fgcolor);
      DrawVLine(buffer,xres,*pPosX + curfontwidth -1, PosY +1, FontHeight -1, fgcolor);
      FillRect(buffer,xres,*pPosX, PosY +1, curfontwidth-1, FontHeight-1, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE3: /* |  */
      DrawVLine(buffer,xres,*pPosX, PosY, FontHeight, fgcolor);
      FillRect(buffer,xres,*pPosX +1, PosY, curfontwidth -1, FontHeight, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE4: /*  | */
      DrawVLine(buffer,xres,*pPosX + curfontwidth -1, PosY, FontHeight, fgcolor);
      FillRect(buffer,xres,*pPosX, PosY, curfontwidth -1, FontHeight, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE5: /* |_ */
      DrawHLine(buffer,xres,*pPosX, PosY + FontHeight -1, curfontwidth, fgcolor);
      DrawVLine(buffer,xres,*pPosX, PosY, FontHeight -1, fgcolor);
      FillRect(buffer,xres,*pPosX +1, PosY, curfontwidth-1, FontHeight-1, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE6: /* _ */
      DrawHLine(buffer,xres,*pPosX, PosY + FontHeight -1, curfontwidth, fgcolor);
      FillRect(buffer,xres,*pPosX, PosY, curfontwidth, FontHeight-1, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE7: /* _| */
      DrawHLine(buffer,xres,*pPosX, PosY + FontHeight -1, curfontwidth, fgcolor);
      DrawVLine(buffer,xres,*pPosX + curfontwidth -1, PosY, FontHeight -1, fgcolor);
      FillRect(buffer,xres,*pPosX, PosY, curfontwidth-1, FontHeight-1, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE8: /* Ii */
      FillRect(buffer,xres,*pPosX +1, PosY, curfontwidth -1, FontHeight, bgcolor);
      for (int Row=0; Row < curfontwidth/2; Row++)
        DrawVLine(buffer,xres,*pPosX + Row, PosY + Row, FontHeight - Row, fgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xE9: /* II */
      FillRect(buffer,xres,*pPosX, PosY, curfontwidth/2, FontHeight, fgcolor);
      FillRect(buffer,xres,*pPosX + curfontwidth/2, PosY, (curfontwidth+1)/2, FontHeight, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xEA: /* ∞  */
      FillRect(buffer,xres,*pPosX, PosY, curfontwidth, FontHeight, bgcolor);
      FillRect(buffer,xres,*pPosX, PosY, curfontwidth/2, curfontwidth/2, fgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xEB: /* ¨ */
      FillRect(buffer,xres,*pPosX, PosY +1, curfontwidth, FontHeight -1, bgcolor);
      for (int Row=0; Row < curfontwidth/2; Row++)
        DrawHLine(buffer,xres,*pPosX + Row, PosY + Row, curfontwidth - Row, fgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xEC: /* -- */
      FillRect(buffer, xres,*pPosX, PosY, curfontwidth, curfontwidth/2, fgcolor);
      FillRect(buffer, xres,*pPosX, PosY + curfontwidth/2, curfontwidth, FontHeight - curfontwidth/2, bgcolor);
      *pPosX += curfontwidth;
      return 0;
    case 0xED:
    case 0xEE:
    case 0xEF:
    case 0xF0:
    case 0xF1:
    case 0xF2:
    case 0xF3:
    case 0xF4:
      Char = arrowtable[Char - 0xED];
      break;
    default:
      break;
    }
  }
  if (Char <= 0x20)
  {
    FillRect(buffer, xres, *pPosX, PosY, curfontwidth, factor*FontHeight, bgcolor);
    *pPosX += curfontwidth;
    return -2;
  }
  return Char; // Char is an alphanumeric unicode character
}

TextPageinfo_t* CTeletextDecoder::DecodePage(bool showl25,             // 1=decode Level2.5-graphics
                                            unsigned char* PageChar,  // page buffer, min. 25*40
                                            TextPageAttr_t *PageAtrb, // attribute buffer, min 25*40
                                            bool HintMode,            // 1=show hidden information
                                            bool showflof)            // 1=decode FLOF-line
{
  int col;
  int hold, dhset;
  int foreground, background, doubleheight, doublewidth, charset, previous_charset, mosaictype, IgnoreAtBlackBgSubst, concealed, flashmode, boxwin;
  unsigned char held_mosaic, *p;
  TextCachedPage_t *pCachedPage;

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  /* copy page to decode buffer */
  if (m_txtCache->SubPageTable[m_txtCache->Page] == 0xff) /* not cached: do nothing */
    return NULL;

  if (m_txtCache->ZapSubpageManual)
    pCachedPage = m_txtCache->astCachetable[m_txtCache->Page][m_txtCache->SubPage];
  else
    pCachedPage = m_txtCache->astCachetable[m_txtCache->Page][m_txtCache->SubPageTable[m_txtCache->Page]];
  if (!pCachedPage)  /* not cached: do nothing */
    return nullptr;

  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  appPlayer->LoadPage(m_txtCache->Page, m_txtCache->SubPage, &PageChar[40]);

  memcpy(&PageChar[8], pCachedPage->p0, 24); /* header line without TimeString */

  TextPageinfo_t* PageInfo = &(pCachedPage->pageinfo);
  if (PageInfo->p24)
    memcpy(&PageChar[24*40], PageInfo->p24, 40); /* line 25 for FLOF */

  /* copy TimeString */
  memcpy(&PageChar[32], &m_txtCache->TimeString, 8);

  bool boxed;
  /* check for newsflash & subtitle */
  if (PageInfo->boxed && IsDec(m_txtCache->Page))
    boxed = true;
  else
    boxed = false;


  /* modify header */
  if (boxed)
  {
    memset(PageChar, ' ', 40);
  }
  else
  {
    memset(PageChar, ' ', 8);
    CDVDTeletextTools::Hex2Str((char*)PageChar+3, m_txtCache->Page);
    if (m_txtCache->SubPage)
    {
      *(PageChar+4) ='/';
      *(PageChar+5) ='0';
      CDVDTeletextTools::Hex2Str((char*)PageChar+6, m_txtCache->SubPage);
    }
  }

  if (!IsDec(m_txtCache->Page))
  {
    TextPageAttr_t atr = { TXT_ColorWhite  , TXT_ColorBlack , C_G0P, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0x3f};
    if (PageInfo->function == FUNC_MOT) /* magazine organization table */
    {
      for (col = 0; col < 24*40; col++)
        PageAtrb[col] = atr;
      for (col = 40; col < 24*40; col++)
        PageChar[col] = number2char(PageChar[col]);
      return PageInfo; /* don't interpret irregular pages */
    }
    else if (PageInfo->function == FUNC_GPOP || PageInfo->function == FUNC_POP) /* object definitions */
    {
      for (int col = 0; col < 24*40; col++)
        PageAtrb[col] = atr;

      p = PageChar + 40;
      for (int row = 1; row < 12; row++)
      {
        *p++ = number2char(row); /* first column: number (0-9, A-..) */
        for (int col = 1; col < 40; col += 3)
        {
          int d = CDVDTeletextTools::deh24(p);
          if (d < 0)
          {
            memcpy(p, "???", 3);
          p += 3;
          }
          else
          {
            *p++ = number2char((d >> 6) & 0x1f); /* mode */
            *p++ = number2char(d & 0x3f); /* address */
            *p++ = number2char((d >> 11) & 0x7f); /* data */
          }
        }
      }
      return PageInfo; /* don't interpret irregular pages */
    }
    else if (PageInfo->function == FUNC_GDRCS || PageInfo->function == FUNC_DRCS) /* character definitions */
    {
      return PageInfo; /* don't interpret irregular pages */
    }
    else
    {
      int h, parityerror = 0;

      for (int i = 0; i < 8; i++)
        PageAtrb[i] = atr;

      /* decode parity/hamming */
      for (unsigned int i = 40; i < TELETEXT_PAGE_SIZE; i++)
      {
        PageAtrb[i] = atr;
        p = PageChar + i;
        h = dehamming[*p];
        if (parityerror && h != 0xFF)  /* if no regular page (after any parity error) */
          CDVDTeletextTools::Hex2Str((char*)p, h);  /* first try dehamming */
        else
        {
          if (*p == ' ' || deparity[*p] != ' ') /* correct parity */
            *p &= 127;
          else
          {
            parityerror = 1;
            if (h != 0xFF)  /* first parity error: try dehamming */
              CDVDTeletextTools::Hex2Str((char*)p, h);
            else
              *p = ' ';
          }
        }
      }
      if (parityerror)
      {
        return PageInfo; /* don't interpret irregular pages */
      }
    }
  }
  int mosaic_pending,esc_pending;
  /* decode */
  for (int row = 0; row < ((showflof && PageInfo->p24) ? 25 : 24); row++)
  {
    /* start-of-row default conditions */
    foreground   = TXT_ColorWhite;
    background   = TXT_ColorBlack;
    doubleheight = 0;
    doublewidth  = 0;
    charset      = previous_charset = C_G0P; // remember charset for switching back after mosaic charset was used
    mosaictype   = 0;
    concealed    = 0;
    flashmode    = 0;
    hold         = 0;
    boxwin       = 0;
    held_mosaic  = ' ';
    dhset        = 0;
    IgnoreAtBlackBgSubst = 0;
    mosaic_pending = esc_pending = 0; // we need to render at least one mosaic char if 'esc' is received immediately after mosaic charset switch on

    if (boxed && memchr(&PageChar[row*40], start_box, 40) == 0)
    {
      foreground = TXT_ColorTransp;
      background = TXT_ColorTransp;
    }

    for (int col = 0; col < 40; col++)
    {
      int index = row*40 + col;

      PageAtrb[index].fg = foreground;
      PageAtrb[index].bg = background;
      PageAtrb[index].charset = charset;
      PageAtrb[index].doubleh = doubleheight;
      PageAtrb[index].doublew = (col < 39 ? doublewidth : 0);
      PageAtrb[index].IgnoreAtBlackBgSubst = IgnoreAtBlackBgSubst;
      PageAtrb[index].concealed = concealed;
      PageAtrb[index].flashing  = flashmode;
      PageAtrb[index].boxwin    = boxwin;
      PageAtrb[index].inverted  = 0; // only relevant for Level 2.5
      PageAtrb[index].underline = 0; // only relevant for Level 2.5
      PageAtrb[index].diacrit   = 0; // only relevant for Level 2.5
      PageAtrb[index].setX26    = 0; // only relevant for Level 2.5
      PageAtrb[index].setG0G2   = 0x3f; // only relevant for Level 2.5

      if (PageChar[index] < ' ')
      {
        if (esc_pending) { // mosaic char has been rendered and we can switch charsets
          charset = previous_charset;
          if (charset == C_G0P)
            charset = previous_charset = C_G0S;
          else if (charset == C_G0S)
            charset = previous_charset = C_G0P;
          esc_pending = 0;
        }
        switch (PageChar[index])
        {
        case alpha_black:
        case alpha_red:
        case alpha_green:
        case alpha_yellow:
        case alpha_blue:
        case alpha_magenta:
        case alpha_cyan:
        case alpha_white:
          concealed = 0;
          foreground = PageChar[index] - alpha_black + TXT_ColorBlack;
          if (col == 0 && PageChar[index] == alpha_white)
            PageAtrb[index].fg = TXT_ColorBlack; // indicate level 1 color change on column 0; (hack)
          if ((charset!=C_G0P) && (charset!=C_G0S)) // we need to change charset to state it was before mosaic
            charset = previous_charset;
          break;

        case flash:
          flashmode = 1;
          break;

        case steady:
          flashmode = 0;
          PageAtrb[index].flashing = 0;
          break;

        case end_box:
          boxwin = 0;
          IgnoreAtBlackBgSubst = 0;
          break;

        case start_box:
          if (!boxwin)
            boxwin = 1;
          break;

        case normal_size:
          doubleheight = 0;
          doublewidth = 0;
          PageAtrb[index].doubleh = doubleheight;
          PageAtrb[index].doublew = doublewidth;
          break;

        case double_height:
          if (row < 23)
          {
            doubleheight = 1;
            dhset = 1;
          }
          doublewidth = 0;

          break;

        case double_width:
          if (col < 39)
            doublewidth = 1;
          doubleheight = 0;
          break;

        case double_size:
          if (row < 23)
          {
            doubleheight = 1;
            dhset = 1;
          }
          if (col < 39)
            doublewidth = 1;
          break;

        case mosaic_black:
        case mosaic_red:
        case mosaic_green:
        case mosaic_yellow:
        case mosaic_blue:
        case mosaic_magenta:
        case mosaic_cyan:
        case mosaic_white:
          concealed = 0;
          foreground = PageChar[index] - mosaic_black + TXT_ColorBlack;
          if ((charset==C_G0P) || (charset==C_G0S))
            previous_charset=charset;
          charset = mosaictype ? C_G1S : C_G1C;
          mosaic_pending = 1;
          break;

        case conceal:
          PageAtrb[index].concealed = 1;
          concealed = 1;
          if (!HintMode)
          {
            foreground = background;
            PageAtrb[index].fg = foreground;
          }
          break;

        case contiguous_mosaic:
          mosaictype = 0;
          if (charset == C_G1S)
          {
            charset = C_G1C;
            PageAtrb[index].charset = charset;
          }
          break;

        case separated_mosaic:
          mosaictype = 1;
          if (charset == C_G1C)
          {
            charset = C_G1S;
            PageAtrb[index].charset = charset;
          }
          break;

        case esc:
          if (!mosaic_pending) { // if mosaic is pending we need to wait before mosaic arrives
            if ((charset != C_G0P) && (charset != C_G0S)) // we need to switch to charset which was active before mosaic
              charset = previous_charset;
            if (charset == C_G0P)
              charset = previous_charset = C_G0S;
            else if (charset == C_G0S)
              charset = previous_charset = C_G0P;
          } else esc_pending = 1;
          break;

        case black_background:
          background = TXT_ColorBlack;
          IgnoreAtBlackBgSubst = 0;
          PageAtrb[index].bg = background;
          PageAtrb[index].IgnoreAtBlackBgSubst = IgnoreAtBlackBgSubst;
          break;

        case new_background:
          background = foreground;
          if (background == TXT_ColorBlack)
            IgnoreAtBlackBgSubst = 1;
          else
            IgnoreAtBlackBgSubst = 0;
          PageAtrb[index].bg = background;
          PageAtrb[index].IgnoreAtBlackBgSubst = IgnoreAtBlackBgSubst;
          break;

        case hold_mosaic:
          hold = 1;
          break;

        case release_mosaic:
          hold = 2;
          break;
        }

        /* handle spacing attributes */
        if (hold && (PageAtrb[index].charset == C_G1C || PageAtrb[index].charset == C_G1S))
          PageChar[index] = held_mosaic;
        else
          PageChar[index] = ' ';

        if (hold == 2)
          hold = 0;
      }
      else /* char >= ' ' */
      {
        mosaic_pending = 0; // charset will be switched next if esc_pending
        /* set new held-mosaic char */
        if ((charset == C_G1C || charset == C_G1S) &&
           ((PageChar[index]&0xA0) == 0x20))
          held_mosaic = PageChar[index];
        if (PageAtrb[index].doubleh)
          PageChar[index + 40] = 0xFF;

      }
      if (!(charset == C_G1C || charset == C_G1S))
        held_mosaic = ' '; /* forget if outside mosaic */

    } /* for col */

    /* skip row if doubleheight */
    if (row < 23 && dhset)
    {
      for (int col = 0; col < 40; col++)
      {
        int index = row*40 + col;
        PageAtrb[index+40].bg = PageAtrb[index].bg;
        PageAtrb[index+40].fg = TXT_ColorWhite;
        if (!PageAtrb[index].doubleh)
          PageChar[index+40] = ' ';
        PageAtrb[index+40].flashing = 0;
        PageAtrb[index+40].charset = C_G0P;
        PageAtrb[index+40].doubleh = 0;
        PageAtrb[index+40].doublew = 0;
        PageAtrb[index+40].IgnoreAtBlackBgSubst = 0;
        PageAtrb[index+40].concealed = 0;
        PageAtrb[index+40].flashing  = 0;
        PageAtrb[index+40].boxwin    = PageAtrb[index].boxwin;
      }
      row++;
    }
  } /* for row */
  m_txtCache->FullScrColor = TXT_ColorBlack;

  if (showl25)
    Eval_l25(PageChar, PageAtrb, HintMode);

  /* handle Black Background Color Substitution and transparency (CLUT1#0) */
  {
    int o = 0;
    char bitmask ;

    for (unsigned char row : m_txtCache->FullRowColor)
    {
      for (int c = 0; c < 40; c++)
      {
        bitmask = (PageAtrb[o].bg == 0x08 ? 0x08 : 0x00) | (row == 0x08 ? 0x04 : 0x00) | (PageAtrb[o].boxwin <<1) | (int)boxed;
        switch (bitmask)
        {
          case 0x08:
          case 0x0b:
            if (row == 0x08)
              PageAtrb[o].bg = m_txtCache->FullScrColor;
            else
              PageAtrb[o].bg = row;
            break;
          case 0x01:
          case 0x05:
          case 0x09:
          case 0x0a:
          case 0x0c:
          case 0x0d:
          case 0x0e:
          case 0x0f:
            PageAtrb[o].bg = TXT_ColorTransp;
            break;
        }
        bitmask = (PageAtrb[o].fg  == 0x08 ? 0x08 : 0x00) | (row == 0x08 ? 0x04 : 0x00) | (PageAtrb[o].boxwin <<1) | (int)boxed;
        switch (bitmask)
        {
          case 0x08:
          case 0x0b:
            if (row == 0x08)
              PageAtrb[o].fg = m_txtCache->FullScrColor;
            else
              PageAtrb[o].fg = row;
            break;
          case 0x01:
          case 0x05:
          case 0x09:
          case 0x0a:
          case 0x0c:
          case 0x0d:
          case 0x0e:
          case 0x0f:
            PageAtrb[o].fg = TXT_ColorTransp;
            break;
        }
        o++;
      }
    }
  }
  return PageInfo;
}

void CTeletextDecoder::Eval_l25(unsigned char* PageChar, TextPageAttr_t *PageAtrb, bool HintMode)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  memset(m_txtCache->FullRowColor, 0, sizeof(m_txtCache->FullRowColor));
  m_txtCache->FullScrColor = TXT_ColorBlack;
  m_txtCache->ColorTable   = NULL;

  if (!m_txtCache->astCachetable[m_txtCache->Page][m_txtCache->SubPage])
    return;

  /* normal page */
  if (IsDec(m_txtCache->Page))
  {
    unsigned char APx0, APy0, APx, APy;
    TextPageinfo_t *pi      = &(m_txtCache->astCachetable[m_txtCache->Page][m_txtCache->SubPage]->pageinfo);
    TextCachedPage_t *pmot  = m_txtCache->astCachetable[(m_txtCache->Page & 0xf00) | 0xfe][0];
    int p26Received         = 0;
    int BlackBgSubst        = 0;
    int ColorTableRemapping = 0;

    m_txtCache->pop = m_txtCache->gpop = m_txtCache->drcs = m_txtCache->gdrcs = 0;

    if (pi->ext)
    {
      TextExtData_t *e = pi->ext;

      if (e->p26[0])
        p26Received = 1;

      if (e->p27)
      {
        Textp27_t *p27 = e->p27;
        if (p27[0].l25)
          m_txtCache->gpop = p27[0].page;
        if (p27[1].l25)
          m_txtCache->pop = p27[1].page;
        if (p27[2].l25)
          m_txtCache->gdrcs = p27[2].page;
        if (p27[3].l25)
          m_txtCache->drcs = p27[3].page;
      }

      if (e->p28Received)
      {
        m_txtCache->ColorTable              = e->bgr;
        BlackBgSubst                      = e->BlackBgSubst;
        ColorTableRemapping               = e->ColorTableRemapping;
        memset(m_txtCache->FullRowColor, e->DefRowColor, sizeof(m_txtCache->FullRowColor));
        m_txtCache->FullScrColor            = e->DefScreenColor;
        m_txtCache->NationalSubset          = SetNational(e->DefaultCharset);
        m_txtCache->NationalSubsetSecondary = SetNational(e->SecondCharset);
      } /* e->p28Received */
    }

    if (!m_txtCache->ColorTable && m_txtCache->astP29[m_txtCache->Page >> 8])
    {
      TextExtData_t *e                    = m_txtCache->astP29[m_txtCache->Page >> 8];
      m_txtCache->ColorTable              = e->bgr;
      BlackBgSubst                        = e->BlackBgSubst;
      ColorTableRemapping                 = e->ColorTableRemapping;
      memset(m_txtCache->FullRowColor, e->DefRowColor, sizeof(m_txtCache->FullRowColor));
      m_txtCache->FullScrColor            = e->DefScreenColor;
      m_txtCache->NationalSubset          = SetNational(e->DefaultCharset);
      m_txtCache->NationalSubsetSecondary = SetNational(e->SecondCharset);
    }

    if (ColorTableRemapping)
    {
      for (int i = 0; i < 25*40; i++)
      {
        PageAtrb[i].fg += MapTblFG[ColorTableRemapping - 1];
        if (!BlackBgSubst || PageAtrb[i].bg != TXT_ColorBlack || PageAtrb[i].IgnoreAtBlackBgSubst)
          PageAtrb[i].bg += MapTblBG[ColorTableRemapping - 1];
      }
    }

    /* determine ?pop/?drcs from MOT */
    if (pmot)
    {
      unsigned char pmot_data[23*40];
      auto& components = CServiceBroker::GetAppComponents();
      const auto appPlayer = components.GetComponent<CApplicationPlayer>();
      appPlayer->LoadPage((m_txtCache->Page & 0xf00) | 0xfe, 0, pmot_data);

      unsigned char *p  = pmot_data;      /* start of link data */
      int o             = 2 * (((m_txtCache->Page & 0xf0) >> 4) * 10 + (m_txtCache->Page & 0x0f));  /* offset of links for current page */
      int opop          = p[o] & 0x07;    /* index of POP link */
      int odrcs         = p[o+1] & 0x07;  /* index of DRCS link */
      unsigned char obj[3*4*4];           /* types* objects * (triplet,packet,subp,high) */
      unsigned char type,ct, tstart = 4*4;
      memset(obj,0,sizeof(obj));

      if (p[o] & 0x08) /* GPOP data used */
      {
        if (!m_txtCache->gpop || !(p[18*40] & 0x08)) /* no p27 data or higher prio of MOT link */
        {
          m_txtCache->gpop = ((p[18*40] << 8) | (p[18*40+1] << 4) | p[18*40+2]) & 0x7ff;
          if ((m_txtCache->gpop & 0xff) == 0xff)
            m_txtCache->gpop = 0;
          else
          {
            if (m_txtCache->gpop < 0x100)
              m_txtCache->gpop += 0x800;
            if (!p26Received)
            {
              ct = 2;
              while (ct)
              {
                ct--;
                type = (p[18*40+5] >> 2*ct) & 0x03;

                if (type == 0) continue;
                  obj[(type-1)*(tstart)+ct*4  ] = 3 * ((p[18*40+7+ct*2] >> 1) & 0x03) + type; //triplet
                  obj[(type-1)*(tstart)+ct*4+1] = ((p[18*40+7+ct*2] & 0x08) >> 3) + 1       ; //packet
                  obj[(type-1)*(tstart)+ct*4+2] = p[18*40+6+ct*2] & 0x0f                    ; //subp
                  obj[(type-1)*(tstart)+ct*4+3] = p[18*40+7+ct*2] & 0x01                    ; //high
              }
            }
          }
        }
      }
      if (opop) /* POP data used */
      {
        opop = 18*40 + 10*opop;  /* offset to POP link */
        if (!m_txtCache->pop || !(p[opop] & 0x08)) /* no p27 data or higher prio of MOT link */
        {
          m_txtCache->pop = ((p[opop] << 8) | (p[opop+1] << 4) | p[opop+2]) & 0x7ff;
          if ((m_txtCache->pop & 0xff) == 0xff)
            m_txtCache->pop = 0;
          else
          {
            if (m_txtCache->pop < 0x100)
              m_txtCache->pop += 0x800;
            if (!p26Received)
            {
              ct = 2;
              while (ct)
              {
                ct--;
                type = (p[opop+5] >> 2*ct) & 0x03;

                if (type == 0) continue;
                  obj[(type-1)*(tstart)+(ct+2)*4  ] = 3 * ((p[opop+7+ct*2] >> 1) & 0x03) + type; //triplet
                  obj[(type-1)*(tstart)+(ct+2)*4+1] = ((p[opop+7+ct*2] & 0x08) >> 3) + 1       ; //packet
                  obj[(type-1)*(tstart)+(ct+2)*4+2] = p[opop+6+ct*2]                           ; //subp
                  obj[(type-1)*(tstart)+(ct+2)*4+3] = p[opop+7+ct*2] & 0x01                    ; //high
              }
            }
          }
        }
      }
      // eval default objects in correct order
      for (int i = 0; i < 12; i++)
      {
        if (obj[i*4] != 0)
        {
          APx0 = APy0 = APx = APy = m_txtCache->tAPx = m_txtCache->tAPy = 0;
          Eval_NumberedObject(i % 4 > 1 ? m_txtCache->pop : m_txtCache->gpop, obj[i*4+2], obj[i*4+1], obj[i*4], obj[i*4+3], &APx, &APy, &APx0, &APy0, PageChar, PageAtrb);
        }
      }

      if (p[o+1] & 0x08) /* GDRCS data used */
      {
        if (!m_txtCache->gdrcs || !(p[20*40] & 0x08)) /* no p27 data or higher prio of MOT link */
        {
          m_txtCache->gdrcs = ((p[20*40] << 8) | (p[20*40+1] << 4) | p[20*40+2]) & 0x7ff;
          if ((m_txtCache->gdrcs & 0xff) == 0xff)
            m_txtCache->gdrcs = 0;
          else if (m_txtCache->gdrcs < 0x100)
            m_txtCache->gdrcs += 0x800;
        }
      }
      if (odrcs) /* DRCS data used */
      {
        odrcs = 20*40 + 4*odrcs;  /* offset to DRCS link */
        if (!m_txtCache->drcs || !(p[odrcs] & 0x08)) /* no p27 data or higher prio of MOT link */
        {
          m_txtCache->drcs = ((p[odrcs] << 8) | (p[odrcs+1] << 4) | p[odrcs+2]) & 0x7ff;
          if ((m_txtCache->drcs & 0xff) == 0xff)
            m_txtCache->drcs = 0;
          else if (m_txtCache->drcs < 0x100)
            m_txtCache->drcs += 0x800;
        }
      }
      if (m_txtCache->astCachetable[m_txtCache->gpop][0])
        m_txtCache->astCachetable[m_txtCache->gpop][0]->pageinfo.function = FUNC_GPOP;
      if (m_txtCache->astCachetable[m_txtCache->pop][0])
        m_txtCache->astCachetable[m_txtCache->pop][0]->pageinfo.function = FUNC_POP;
      if (m_txtCache->astCachetable[m_txtCache->gdrcs][0])
        m_txtCache->astCachetable[m_txtCache->gdrcs][0]->pageinfo.function = FUNC_GDRCS;
      if (m_txtCache->astCachetable[m_txtCache->drcs][0])
        m_txtCache->astCachetable[m_txtCache->drcs][0]->pageinfo.function = FUNC_DRCS;
    } /* if mot */

    /* evaluate local extension data from p26 */
    if (p26Received)
    {
      APx0 = APy0 = APx = APy = m_txtCache->tAPx = m_txtCache->tAPy = 0;
      Eval_Object(13 * (23-2 + 2), m_txtCache->astCachetable[m_txtCache->Page][m_txtCache->SubPage], &APx, &APy, &APx0, &APy0, OBJ_ACTIVE, &PageChar[40], PageChar, PageAtrb); /* 1st triplet p26/0 */
    }

    {
      int o = 0;
      for (unsigned char row : m_txtCache->FullRowColor)
      {
        for (int c = 0; c < 40; c++)
        {
          if (BlackBgSubst && PageAtrb[o].bg == TXT_ColorBlack && !(PageAtrb[o].IgnoreAtBlackBgSubst))
          {
            if (row == 0x08)
              PageAtrb[o].bg = m_txtCache->FullScrColor;
            else
              PageAtrb[o].bg = row;
          }
          o++;
        }
      }
    }

    if (!HintMode)
    {
      for (int i = 0; i < 25*40; i++)
      {
        if (PageAtrb[i].concealed) PageAtrb[i].fg = PageAtrb[i].bg;
      }
    }
  } /* is_dec(page) */
}

/* dump interpreted object data to stdout */
/* in: 18 bit object data */
/* out: termination info, >0 if end of object */
void CTeletextDecoder::Eval_Object(int iONr, TextCachedPage_t *pstCachedPage,
            unsigned char *pAPx, unsigned char *pAPy,
            unsigned char *pAPx0, unsigned char *pAPy0,
            tObjType ObjType, unsigned char* pagedata, unsigned char* PageChar, TextPageAttr_t* PageAtrb)
{
  int iOData;
  int iONr1 = iONr + 1; /* don't terminate after first triplet */
  unsigned char drcssubp=0, gdrcssubp=0;
  signed char endcol = -1; /* last column to which to extend attribute changes */
  TextPageAttr_t attrPassive = { TXT_ColorWhite  , TXT_ColorBlack , C_G0P, 0, 0, 1 ,0, 0, 0, 0, 0, 0, 0, 0x3f}; /* current attribute for passive objects */

  do
  {
    iOData = iTripletNumber2Data(iONr, pstCachedPage, pagedata);  /* get triplet data, next triplet */
    if (iOData < 0) /* invalid number, not cached, or hamming error: terminate */
      break;

    if (endcol < 0)
    {
      if (ObjType == OBJ_ACTIVE)
      {
        endcol = 40;
      }
      else if (ObjType == OBJ_ADAPTIVE) /* search end of line */
      {
        for (int i = iONr; i <= 506; i++)
        {
          int iTempOData = iTripletNumber2Data(i, pstCachedPage, pagedata); /* get triplet data, next triplet */
          int iAddress = (iTempOData      ) & 0x3f;
          int iMode    = (iTempOData >>  6) & 0x1f;
          //int iData    = (iTempOData >> 11) & 0x7f;
          if (iTempOData < 0 || /* invalid number, not cached, or hamming error: terminate */
             (iAddress >= 40  /* new row: row address and */
             && (iMode == 0x01 || /* Full Row Color or */
                iMode == 0x04 || /* Set Active Position */
                (iMode >= 0x15 && iMode <= 0x17) || /* Object Definition */
                iMode == 0x17))) /* Object Termination */
            break;
          if (iAddress < 40 && iMode != 0x06)
            endcol = iAddress;
        }
      }
    }
    iONr++;
  }
  while (0 == Eval_Triplet(iOData, pstCachedPage, pAPx, pAPy, pAPx0, pAPy0, &drcssubp, &gdrcssubp, &endcol, &attrPassive, pagedata, PageChar, PageAtrb) || iONr1 == iONr); /* repeat until termination reached */
}

void CTeletextDecoder::Eval_NumberedObject(int p, int s, int packet, int triplet, int high,
                 unsigned char *pAPx, unsigned char *pAPy,
                 unsigned char *pAPx0, unsigned char *pAPy0, unsigned char* PageChar, TextPageAttr_t* PageAtrb)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  if (!packet || 0 == m_txtCache->astCachetable[p][s])
    return;

  unsigned char pagedata[23*40];
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  appPlayer->LoadPage(p, s, pagedata);

  int idata = CDVDTeletextTools::deh24(pagedata + 40*(packet-1) + 1 + 3*triplet);
  int iONr;

  if (idata < 0)  /* hamming error: ignore triplet */
    return;
  if (high)
    iONr = idata >> 9; /* triplet number of odd object data */
  else
    iONr = idata & 0x1ff; /* triplet number of even object data */
  if (iONr <= 506)
  {
    Eval_Object(iONr, m_txtCache->astCachetable[p][s], pAPx, pAPy, pAPx0, pAPy0, (tObjType)(triplet % 3),pagedata, PageChar, PageAtrb);
  }
}

int CTeletextDecoder::Eval_Triplet(int iOData, TextCachedPage_t *pstCachedPage,
            unsigned char *pAPx, unsigned char *pAPy,
            unsigned char *pAPx0, unsigned char *pAPy0,
            unsigned char *drcssubp, unsigned char *gdrcssubp,
            signed char *endcol, TextPageAttr_t *attrPassive, unsigned char* pagedata, unsigned char* PageChar, TextPageAttr_t* PageAtrb)
{
  int iAddress = (iOData      ) & 0x3f;
  int iMode    = (iOData >>  6) & 0x1f;
  int iData    = (iOData >> 11) & 0x7f;

  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  if (iAddress < 40) /* column addresses */
  {
    int offset;  /* offset to PageChar and PageAtrb */

    if (iMode != 0x06)
      *pAPx = iAddress;  /* new Active Column */
    offset = (*pAPy0 + *pAPy) * 40 + *pAPx0 + *pAPx;  /* offset to PageChar and PageAtrb */

    switch (iMode)
    {
    case 0x00:
      if (0 == (iData>>5))
      {
        int newcolor = iData & 0x1f;
        if (*endcol < 0) /* passive object */
          attrPassive->fg = newcolor;
        else if (*endcol == 40) /* active object */
        {
          TextPageAttr_t *p = &PageAtrb[offset];
          int oldcolor = (p)->fg; /* current color (set-after) */
          int c = *pAPx0 + *pAPx;  /* current column absolute */
          do
          {
            p->fg = newcolor;
            p++;
            c++;
          } while (c < 40 && p->fg == oldcolor);  /* stop at change by level 1 page */
        }
        else /* adaptive object */
        {
          TextPageAttr_t *p = &PageAtrb[offset];
          int c = *pAPx;  /* current column relative to object origin */
          do
          {
            p->fg = newcolor;
            p++;
            c++;
          } while (c <= *endcol);
        }
      }
      break;
    case 0x01:
      if (iData >= 0x20)
      {
        PageChar[offset] = iData;
        if (*endcol < 0) /* passive object */
        {
          attrPassive->charset = C_G1C; /* FIXME: separated? */
          PageAtrb[offset] = *attrPassive;
        }
        else if (PageAtrb[offset].charset != C_G1S)
          PageAtrb[offset].charset = C_G1C; /* FIXME: separated? */
      }
      break;
    case 0x02:
    case 0x0b:
      PageChar[offset] = iData;
      if (*endcol < 0) /* passive object */
      {
        attrPassive->charset = C_G3;
        PageAtrb[offset] = *attrPassive;
      }
      else
        PageAtrb[offset].charset = C_G3;
      break;
    case 0x03:
      if (0 == (iData>>5))
      {
        int newcolor = iData & 0x1f;
        if (*endcol < 0) /* passive object */
          attrPassive->bg = newcolor;
        else if (*endcol == 40) /* active object */
        {
          TextPageAttr_t *p = &PageAtrb[offset];
          int oldcolor = (p)->bg; /* current color (set-after) */
          int c = *pAPx0 + *pAPx;  /* current column absolute */
          do
          {
            p->bg = newcolor;
            if (newcolor == TXT_ColorBlack)
              p->IgnoreAtBlackBgSubst = 1;
            p++;
            c++;
          } while (c < 40 && p->bg == oldcolor);  /* stop at change by level 1 page */
        }
        else /* adaptive object */
        {
          TextPageAttr_t *p = &PageAtrb[offset];
          int c = *pAPx;  /* current column relative to object origin */
          do
          {
            p->bg = newcolor;
            if (newcolor == TXT_ColorBlack)
              p->IgnoreAtBlackBgSubst = 1;
            p++;
            c++;
          } while (c <= *endcol);
        }
      }
      break;
    case 0x06:
      /* ignore */
      break;
    case 0x07:
      if ((iData & 0x60) != 0) break; // reserved data field
      if (*endcol < 0) /* passive object */
      {
        attrPassive->flashing=iData & 0x1f;
        PageAtrb[offset] = *attrPassive;
      }
      else
        PageAtrb[offset].flashing=iData & 0x1f;
      break;
    case 0x08:
      if (*endcol < 0) /* passive object */
      {
        attrPassive->setG0G2=iData & 0x3f;
        PageAtrb[offset] = *attrPassive;
      }
      else
        PageAtrb[offset].setG0G2=iData & 0x3f;
      break;
    case 0x09:
      PageChar[offset] = iData;
      if (*endcol < 0) /* passive object */
      {
        attrPassive->charset = C_G0P; /* FIXME: secondary? */
        attrPassive->setX26  = 1;
        PageAtrb[offset] = *attrPassive;
      }
      else
      {
        PageAtrb[offset].charset = C_G0P; /* FIXME: secondary? */
        PageAtrb[offset].setX26  = 1;
      }
      break;
//    case 0x0b: (see 0x02)
    case 0x0c:
    {
      int conc = (iData & 0x04);
      int inv  = (iData & 0x10);
      int dw   = (iData & 0x40 ?1:0);
      int dh   = (iData & 0x01 ?1:0);
      int sep  = (iData & 0x20);
      int bw   = (iData & 0x02 ?1:0);
      if (*endcol < 0) /* passive object */
      {
        if (conc)
        {
          attrPassive->concealed = 1;
          attrPassive->fg = attrPassive->bg;
        }
        attrPassive->inverted = (inv ? 1- attrPassive->inverted : 0);
        attrPassive->doubleh = dh;
        attrPassive->doublew = dw;
        attrPassive->boxwin = bw;
        if (bw) attrPassive->IgnoreAtBlackBgSubst = 0;
        if (sep)
        {
          if (attrPassive->charset == C_G1C)
            attrPassive->charset = C_G1S;
          else
            attrPassive->underline = 1;
        }
        else
        {
          if (attrPassive->charset == C_G1S)
            attrPassive->charset = C_G1C;
          else
            attrPassive->underline = 0;
        }
      }
      else
      {

        int c = *pAPx0 + (*endcol == 40 ? *pAPx : 0);  /* current column */
        TextPageAttr_t *p = &PageAtrb[offset];
        do
        {
          p->inverted = (inv ? 1- p->inverted : 0);
          if (conc)
          {
            p->concealed = 1;
            p->fg = p->bg;
          }
          if (sep)
          {
            if (p->charset == C_G1C)
              p->charset = C_G1S;
            else
              p->underline = 1;
          }
          else
          {
            if (p->charset == C_G1S)
              p->charset = C_G1C;
            else
              p->underline = 0;
          }
          p->doublew = dw;
          p->doubleh = dh;
          p->boxwin = bw;
          if (bw) p->IgnoreAtBlackBgSubst = 0;
          p++;
          c++;
        } while (c < *endcol);
      }
      break;
    }
    case 0x0d:
      PageChar[offset] = iData & 0x3f;
      if (*endcol < 0) /* passive object */
      {
        attrPassive->charset = C_OFFSET_DRCS + ((iData & 0x40) ? (0x10 + *drcssubp) : *gdrcssubp);
        PageAtrb[offset] = *attrPassive;
      }
      else
        PageAtrb[offset].charset = C_OFFSET_DRCS + ((iData & 0x40) ? (0x10 + *drcssubp) : *gdrcssubp);
      break;
    case 0x0f:
      PageChar[offset] = iData;
      if (*endcol < 0) /* passive object */
      {
        attrPassive->charset = C_G2;
        PageAtrb[offset] = *attrPassive;
      }
      else
        PageAtrb[offset].charset = C_G2;
      break;
    default:
      if (iMode == 0x10 && iData == 0x2a)
        iData = '@';
      if (iMode >= 0x10)
      {
        PageChar[offset] = iData;
        if (*endcol < 0) /* passive object */
        {
          attrPassive->charset = C_G0P;
          attrPassive->diacrit = iMode & 0x0f;
          attrPassive->setX26  = 1;
          PageAtrb[offset] = *attrPassive;
        }
        else
        {
          PageAtrb[offset].charset = C_G0P;
          PageAtrb[offset].diacrit = iMode & 0x0f;
          PageAtrb[offset].setX26  = 1;
        }
      }
      break; /* unsupported or not yet implemented mode: ignore */
    } /* switch (iMode) */
  }
  else /* ================= (iAddress >= 40): row addresses ====================== */
  {
    switch (iMode)
    {
    case 0x00:
      if (0 == (iData>>5))
      {
        m_txtCache->FullScrColor = iData & 0x1f;
      }
      break;
    case 0x01:
      if (*endcol == 40) /* active object */
      {
        *pAPy = RowAddress2Row(iAddress);  /* new Active Row */

        int color = iData & 0x1f;
        int row = *pAPy0 + *pAPy;
        int maxrow;

        if (row <= 24 && 0 == (iData>>5))
          maxrow = row;
        else if (3 == (iData>>5))
          maxrow = 24;
        else
          maxrow = -1;
        for (; row <= maxrow; row++)
          m_txtCache->FullRowColor[row] = color;
        *endcol = -1;
      }
      break;
    case 0x04:
      *pAPy = RowAddress2Row(iAddress); /* new Active Row */
      if (iData < 40)
        *pAPx = iData;  /* new Active Column */
      *endcol = -1; /* FIXME: check if row changed? */
      break;
    case 0x07:
      if (iAddress == 0x3f)
      {
        *pAPx = *pAPy = 0; /* new Active Position 0,0 */
        if (*endcol == 40) /* active object */
        {
          int color = iData & 0x1f;
          int row = *pAPy0; // + *pAPy;
          int maxrow;

          if (row <= 24 && 0 == (iData>>5))
            maxrow = row;
          else if (3 == (iData>>5))
            maxrow = 24;
          else
            maxrow = -1;
          for (; row <= maxrow; row++)
            m_txtCache->FullRowColor[row] = color;
        }
        *endcol = -1;
      }
      break;
    case 0x08:
    case 0x09:
    case 0x0a:
    case 0x0b:
    case 0x0c:
    case 0x0d:
    case 0x0e:
    case 0x0f:
      /* ignore */
      break;
    case 0x10:
      m_txtCache->tAPy = iAddress - 40;
      m_txtCache->tAPx = iData;
      break;
    case 0x11:
    case 0x12:
    case 0x13:
      if (iAddress & 0x10)  /* POP or GPOP */
      {
        unsigned char APx = 0, APy = 0;
        unsigned char APx0 = *pAPx0 + *pAPx + m_txtCache->tAPx, APy0 = *pAPy0 + *pAPy + m_txtCache->tAPy;
        int triplet = 3 * ((iData >> 5) & 0x03) + (iMode & 0x03);
        int packet = (iAddress & 0x03) + 1;
        int subp = iData & 0x0f;
        int high = (iData >> 4) & 0x01;


        if (APx0 < 40) /* not in side panel */
        {
          Eval_NumberedObject((iAddress & 0x08) ? m_txtCache->gpop : m_txtCache->pop, subp, packet, triplet, high, &APx, &APy, &APx0, &APy0, PageChar,PageAtrb);
        }
      }
      else if (iAddress & 0x08)  /* local: eval invoked object */
      {
        unsigned char APx = 0, APy = 0;
        unsigned char APx0 = *pAPx0 + *pAPx + m_txtCache->tAPx, APy0 = *pAPy0 + *pAPy + m_txtCache->tAPy;
        int descode = ((iAddress & 0x01) << 3) | (iData >> 4);
        int triplet = iData & 0x0f;

        if (APx0 < 40) /* not in side panel */
        {
          Eval_Object(13 * 23 + 13 * descode + triplet, pstCachedPage, &APx, &APy, &APx0, &APy0, (tObjType)(triplet % 3), pagedata, PageChar, PageAtrb);
        }
      }
      break;
    case 0x15:
    case 0x16:
    case 0x17:
      if (0 == (iAddress & 0x08))  /* Object Definition illegal or only level 3.5 */
        break; /* ignore */

      m_txtCache->tAPx = m_txtCache->tAPy = 0;
      *endcol = -1;
      return 0xFF; /* termination by object definition */
      break;
    case 0x18:
      if (0 == (iData & 0x10)) /* DRCS Mode reserved or only level 3.5 */
        break; /* ignore */

      if (iData & 0x40)
        *drcssubp = iData & 0x0f;
      else
        *gdrcssubp = iData & 0x0f;
      break;
    case 0x1f:
      m_txtCache->tAPx = m_txtCache->tAPy = 0;
      *endcol = -1;
      return 0x80 | iData; /* explicit termination */
      break;
    default:
      break; /* unsupported or not yet implemented mode: ignore */
    } /* switch (iMode) */
  } /* (iAddress >= 40): row addresses */

  if (iAddress < 40 || iMode != 0x10) /* leave temp. AP-Offset unchanged only immediately after definition */
    m_txtCache->tAPx = m_txtCache->tAPy = 0;

  return 0; /* normal exit, no termination */
}

/* get object data */
/* in: absolute triplet number (0..506, start at packet 3 byte 1) */
/* in: pointer to cache struct of page data */
/* out: 18 bit triplet data, <0 if invalid number, not cached, or hamming error */
int CTeletextDecoder::iTripletNumber2Data(int iONr, TextCachedPage_t *pstCachedPage, unsigned char* pagedata)
{
  if (iONr > 506 || 0 == pstCachedPage)
    return -1;

  unsigned char *p;
  int packet = (iONr / 13) + 3;
  int packetoffset = 3 * (iONr % 13);

  if (packet <= 23)
    p = pagedata + 40*(packet-1) + packetoffset + 1;
  else if (packet <= 25)
  {
    if (0 == pstCachedPage->pageinfo.p24)
      return -1;
    p = pstCachedPage->pageinfo.p24 + 40*(packet-24) + packetoffset + 1;
  }
  else
  {
    int descode = packet - 26;
    if (0 == pstCachedPage->pageinfo.ext)
      return -1;
    if (0 == pstCachedPage->pageinfo.ext->p26[descode])
      return -1;
    p = pstCachedPage->pageinfo.ext->p26[descode] + packetoffset;  /* first byte (=designation code) is not cached */
  }
  return CDVDTeletextTools::deh24(p);
}

int CTeletextDecoder::SetNational(unsigned char sec)
{
  std::unique_lock<CCriticalSection> lock(m_txtCache->m_critSection);

  switch (sec)
  {
    case 0x08:
      return NAT_PL; //polish
    case 0x16:
    case 0x36:
      return NAT_TR; //turkish
    case 0x1d:
      return NAT_SR; //serbian, croatian, slovenian
    case 0x20:
      return NAT_SC; // serbian, croatian
    case 0x24:
      return NAT_RB; // russian, bulgarian
    case 0x25:
      return NAT_UA; // ukrainian
    case 0x22:
      return NAT_ET; // estonian
    case 0x23:
      return NAT_LV; // latvian, lithuanian
    case 0x37:
      return NAT_GR; // greek
    case 0x55:
      return NAT_HB; // hebrew
    case 0x47:
    case 0x57:
      return NAT_AR; // arabic
  }
  return CountryConversionTable[sec & 0x07];
}

int CTeletextDecoder::NextHex(int i) /* return next existing non-decimal page number */
{
  int startpage = i;
  if (startpage < 0x100)
    startpage = 0x100;

  do
  {
    i++;
    if (i > 0x8FF)
      i = 0x100;
    if (i == startpage)
      break;
  }  while ((m_txtCache->SubPageTable[i] == 0xFF) || IsDec(i));
  return i;
}

void CTeletextDecoder::SetColors(const unsigned short *pcolormap, int offset, int number)
{
  int j = offset; /* index in global color table */

  for (int i = 0; i < number; i++)
  {
    int r = ((pcolormap[i] >> 8) & 0xf) << 4;
    int g = ((pcolormap[i] >> 4) & 0xf) << 4;
    int b = ((pcolormap[i])      & 0xf) << 4;

    if (m_RenderInfo.rd0[j] != r)
    {
      m_RenderInfo.rd0[j] = r;
    }
    if (m_RenderInfo.gn0[j] != g)
    {
      m_RenderInfo.gn0[j] = g;
    }
    if (m_RenderInfo.bl0[j] != b)
    {
      m_RenderInfo.bl0[j] = b;
    }
    j++;
  }
}

UTILS::COLOR::Color CTeletextDecoder::GetColorRGB(enumTeletextColor ttc)
{
  switch (ttc)
  {
    case TXT_ColorBlack:       return 0xFF000000;
    case TXT_ColorRed:         return 0xFFFC1414;
    case TXT_ColorGreen:       return 0xFF24FC24;
    case TXT_ColorYellow:      return 0xFFFCC024;
    case TXT_ColorBlue:        return 0xFF0000FC;
    case TXT_ColorMagenta:     return 0xFFB000FC;
    case TXT_ColorCyan:        return 0xFF00FCFC;
    case TXT_ColorWhite:       return 0xFFFCFCFC;
    case TXT_ColorTransp:      return 0x00000000;
    default:                   break;
  }

 /* Get colors for CLUTs 2+3 */
  int index = (int)ttc;
  UTILS::COLOR::Color color = (m_RenderInfo.tr0[index] << 24) | (m_RenderInfo.bl0[index] << 16) |
                              (m_RenderInfo.gn0[index] << 8) | m_RenderInfo.rd0[index];
  return color;
}

