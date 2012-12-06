#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/StdString.h"

#define FLOFSIZE 4
#define SUBTITLE_CACHESIZE 50

#define number2char(c) ((c) + (((c) <= 9) ? '0' : ('A' - 10)))

enum /* indices in atrtable */
{
  ATR_WB, /* white on black */
  ATR_PassiveDefault, /* Default for passive objects: white on black, ignore at Black Background Color Substitution */
  ATR_L250, /* line25 */
  ATR_L251, /* line25 */
  ATR_L252, /* line25 */
  ATR_L253, /* line25 */
  ATR_TOPMENU0, /* topmenu */
  ATR_TOPMENU1, /* topmenu */
  ATR_TOPMENU2, /* topmenu */
  ATR_TOPMENU3, /* topmenu */
  ATR_MSG0, /* message */
  ATR_MSG1, /* message */
  ATR_MSG2, /* message */
  ATR_MSG3, /* message */
  ATR_MSGDRM0, /* message */
  ATR_MSGDRM1, /* message */
  ATR_MSGDRM2, /* message */
  ATR_MSGDRM3, /* message */
  ATR_MENUHIL0, /* hilit menu line */
  ATR_MENUHIL1, /* hilit menu line */
  ATR_MENUHIL2, /* hilit menu line */
  ATR_MENU0, /* menu line */
  ATR_MENU1, /* menu line */
  ATR_MENU2, /* menu line */
  ATR_MENU3, /* menu line */
  ATR_MENU4, /* menu line */
  ATR_MENU5, /* menu line */
  ATR_MENU6, /* menu line */
  ATR_CATCHMENU0, /* catch menu line */
  ATR_CATCHMENU1 /* catch menu line */
};

/* colortable */
enum enumTeletextColor
{
  TXT_ColorBlack = 0,
  TXT_ColorRed, /* 1 */
  TXT_ColorGreen, /* 2 */
  TXT_ColorYellow, /* 3 */
  TXT_ColorBlue,  /* 4 */
  TXT_ColorMagenta,  /* 5 */
  TXT_ColorCyan,  /* 6 */
  TXT_ColorWhite, /* 7 */
  TXT_ColorMenu1 = (4*8),
  TXT_ColorMenu2,
  TXT_ColorMenu3,
  TXT_ColorTransp,
  TXT_ColorTransp2,
  TXT_Color_SIZECOLTABLE
};

enum /* options for charset */
{
  C_G0P = 0, /* primary G0 */
  C_G0S, /* secondary G0 */
  C_G1C, /* G1 contiguous */
  C_G1S, /* G1 separate */
  C_G2,
  C_G3,
  C_OFFSET_DRCS = 32
  /* 32..47: 32+subpage# GDRCS (offset/20 in PageChar) */
  /* 48..63: 48+subpage#  DRCS (offset/20 in PageChar) */
};

enum /* page function */
{
  FUNC_LOP = 0, /* Basic Level 1 Teletext page (LOP) */
  FUNC_DATA, /* Data broadcasting page coded according to EN 300 708 [2] clause 4 */
  FUNC_GPOP, /* Global Object definition page (GPOP) - (see clause 10.5.1) */
  FUNC_POP, /* Normal Object definition page (POP) - (see clause 10.5.1) */
  FUNC_GDRCS, /* Global DRCS downloading page (GDRCS) - (see clause 10.5.2) */
  FUNC_DRCS, /* Normal DRCS downloading page (DRCS) - (see clause 10.5.2) */
  FUNC_MOT, /* Magazine Organization table (MOT) - (see clause 10.6) */
  FUNC_MIP, /* Magazine Inventory page (MIP) - (see clause 11.3) */
  FUNC_BTT, /* Basic TOP table (BTT) } */
  FUNC_AIT, /* Additional Information Table (AIT) } (see clause 11.2) */
  FUNC_MPT, /* Multi-page table (MPT) } */
  FUNC_MPTEX, /* Multi-page extension table (MPT-EX) } */
  FUNC_TRIGGER /* Page contain trigger messages defined according to [8] */
};

enum
{
  NAT_DEFAULT = 0,
  NAT_CZ = 1,
  NAT_UK = 2,
  NAT_ET = 3,
  NAT_FR = 4,
  NAT_DE = 5,
  NAT_IT = 6,
  NAT_LV = 7,
  NAT_PL = 8,
  NAT_SP = 9,
  NAT_RO = 10,
  NAT_SR = 11,
  NAT_SW = 12,
  NAT_TR = 13,
  NAT_MAX_FROM_HEADER = 13,
  NAT_SC = 14,
  NAT_RB = 15,
  NAT_UA = 16,
  NAT_GR = 17,
  NAT_HB = 18,
  NAT_AR = 19
};

const unsigned char CountryConversionTable[] = { NAT_UK, NAT_DE, NAT_SW, NAT_IT, NAT_FR, NAT_SP, NAT_CZ, NAT_RO};
const unsigned char MapTblFG[] = {  0,  0,  8,  8, 16, 16, 16 };
const unsigned char MapTblBG[] = {  8, 16,  8, 16,  8, 16, 24 };
const unsigned short DefaultColors[] =  /* 0x0bgr */
{
  0x000, 0x00f, 0x0f0, 0x0ff, 0xf00, 0xf0f, 0xff0, 0xfff,
  0x000, 0x007, 0x070, 0x077, 0x700, 0x707, 0x770, 0x777,
  0x50f, 0x07f, 0x7f0, 0xbff, 0xac0, 0x005, 0x256, 0x77c,
  0x333, 0x77f, 0x7f7, 0x7ff, 0xf77, 0xf7f, 0xff7, 0xddd,
  0x420, 0x210, 0x420, 0x000, 0x000
};

/* hamming table */
const unsigned char dehamming[] =
{
  0x01, 0xFF, 0x01, 0x01, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0x02, 0x01, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,
  0xFF, 0x00, 0x01, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x06, 0xFF, 0xFF, 0x0B, 0xFF, 0x00, 0x03, 0xFF,
  0xFF, 0x0C, 0x01, 0xFF, 0x04, 0xFF, 0xFF, 0x07, 0x06, 0xFF, 0xFF, 0x07, 0xFF, 0x07, 0x07, 0x07,
  0x06, 0xFF, 0xFF, 0x05, 0xFF, 0x00, 0x0D, 0xFF, 0x06, 0x06, 0x06, 0xFF, 0x06, 0xFF, 0xFF, 0x07,
  0xFF, 0x02, 0x01, 0xFF, 0x04, 0xFF, 0xFF, 0x09, 0x02, 0x02, 0xFF, 0x02, 0xFF, 0x02, 0x03, 0xFF,
  0x08, 0xFF, 0xFF, 0x05, 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x03, 0xFF, 0x03, 0x03,
  0x04, 0xFF, 0xFF, 0x05, 0x04, 0x04, 0x04, 0xFF, 0xFF, 0x02, 0x0F, 0xFF, 0x04, 0xFF, 0xFF, 0x07,
  0xFF, 0x05, 0x05, 0x05, 0x04, 0xFF, 0xFF, 0x05, 0x06, 0xFF, 0xFF, 0x05, 0xFF, 0x0E, 0x03, 0xFF,
  0xFF, 0x0C, 0x01, 0xFF, 0x0A, 0xFF, 0xFF, 0x09, 0x0A, 0xFF, 0xFF, 0x0B, 0x0A, 0x0A, 0x0A, 0xFF,
  0x08, 0xFF, 0xFF, 0x0B, 0xFF, 0x00, 0x0D, 0xFF, 0xFF, 0x0B, 0x0B, 0x0B, 0x0A, 0xFF, 0xFF, 0x0B,
  0x0C, 0x0C, 0xFF, 0x0C, 0xFF, 0x0C, 0x0D, 0xFF, 0xFF, 0x0C, 0x0F, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,
  0xFF, 0x0C, 0x0D, 0xFF, 0x0D, 0xFF, 0x0D, 0x0D, 0x06, 0xFF, 0xFF, 0x0B, 0xFF, 0x0E, 0x0D, 0xFF,
  0x08, 0xFF, 0xFF, 0x09, 0xFF, 0x09, 0x09, 0x09, 0xFF, 0x02, 0x0F, 0xFF, 0x0A, 0xFF, 0xFF, 0x09,
  0x08, 0x08, 0x08, 0xFF, 0x08, 0xFF, 0xFF, 0x09, 0x08, 0xFF, 0xFF, 0x0B, 0xFF, 0x0E, 0x03, 0xFF,
  0xFF, 0x0C, 0x0F, 0xFF, 0x04, 0xFF, 0xFF, 0x09, 0x0F, 0xFF, 0x0F, 0x0F, 0xFF, 0x0E, 0x0F, 0xFF,
  0x08, 0xFF, 0xFF, 0x05, 0xFF, 0x0E, 0x0D, 0xFF, 0xFF, 0x0E, 0x0F, 0xFF, 0x0E, 0x0E, 0xFF, 0x0E
};

/* odd parity table, error=0x20 (space) */
const unsigned char deparity[] =
{
  ' ' , 0x01, 0x02, ' ' , 0x04, ' ' , ' ' , 0x07, 0x08, ' ' , ' ' , 0x0b, ' ' , 0x0d, 0x0e, ' ' ,
  0x10, ' ' , ' ' , 0x13, ' ' , 0x15, 0x16, ' ' , ' ' , 0x19, 0x1a, ' ' , 0x1c, ' ' , ' ' , 0x1f,
  0x20, ' ' , ' ' , 0x23, ' ' , 0x25, 0x26, ' ' , ' ' , 0x29, 0x2a, ' ' , 0x2c, ' ' , ' ' , 0x2f,
  ' ' , 0x31, 0x32, ' ' , 0x34, ' ' , ' ' , 0x37, 0x38, ' ' , ' ' , 0x3b, ' ' , 0x3d, 0x3e, ' ' ,
  0x40, ' ' , ' ' , 0x43, ' ' , 0x45, 0x46, ' ' , ' ' , 0x49, 0x4a, ' ' , 0x4c, ' ' , ' ' , 0x4f,
  ' ' , 0x51, 0x52, ' ' , 0x54, ' ' , ' ' , 0x57, 0x58, ' ' , ' ' , 0x5b, ' ' , 0x5d, 0x5e, ' ' ,
  ' ' , 0x61, 0x62, ' ' , 0x64, ' ' , ' ' , 0x67, 0x68, ' ' , ' ' , 0x6b, ' ' , 0x6d, 0x6e, ' ' ,
  0x70, ' ' , ' ' , 0x73, ' ' , 0x75, 0x76, ' ' , ' ' , 0x79, 0x7a, ' ' , 0x7c, ' ' , ' ' , 0x7f,
  0x00, ' ' , ' ' , 0x03, ' ' , 0x05, 0x06, ' ' , ' ' , 0x09, 0x0a, ' ' , 0x0c, ' ' , ' ' , 0x0f,
  ' ' , 0x11, 0x12, ' ' , 0x14, ' ' , ' ' , 0x17, 0x18, ' ' , ' ' , 0x1b, ' ' , 0x1d, 0x1e, ' ' ,
  ' ' , 0x21, 0x22, ' ' , 0x24, ' ' , ' ' , 0x27, 0x28, ' ' , ' ' , 0x2b, ' ' , 0x2d, 0x2e, ' ' ,
  0x30, ' ' , ' ' , 0x33, ' ' , 0x35, 0x36, ' ' , ' ' , 0x39, 0x3a, ' ' , 0x3c, ' ' , ' ' , 0x3f,
  ' ' , 0x41, 0x42, ' ' , 0x44, ' ' , ' ' , 0x47, 0x48, ' ' , ' ' , 0x4b, ' ' , 0x4d, 0x4e, ' ' ,
  0x50, ' ' , ' ' , 0x53, ' ' , 0x55, 0x56, ' ' , ' ' , 0x59, 0x5a, ' ' , 0x5c, ' ' , ' ' , 0x5f,
  0x60, ' ' , ' ' , 0x63, ' ' , 0x65, 0x66, ' ' , ' ' , 0x69, 0x6a, ' ' , 0x6c, ' ' , ' ' , 0x6f,
  ' ' , 0x71, 0x72, ' ' , 0x74, ' ' , ' ' , 0x77, 0x78, ' ' , ' ' , 0x7b, ' ' , 0x7d, 0x7e, ' ' ,
};

/*
 *  [AleVT]
 *
 *  This table generates the parity checks for hamm24/18 decoding.
 *  Bit 0 is for test A, 1 for B, ...
 *
 *  Thanks to R. Gancarz for this fine table *g*
 */
const unsigned char hamm24par[3][256] =
{
    {
        /* Parities of first byte */
   0, 33, 34,  3, 35,  2,  1, 32, 36,  5,  6, 39,  7, 38, 37,  4,
  37,  4,  7, 38,  6, 39, 36,  5,  1, 32, 35,  2, 34,  3,  0, 33,
  38,  7,  4, 37,  5, 36, 39,  6,  2, 35, 32,  1, 33,  0,  3, 34,
   3, 34, 33,  0, 32,  1,  2, 35, 39,  6,  5, 36,  4, 37, 38,  7,
  39,  6,  5, 36,  4, 37, 38,  7,  3, 34, 33,  0, 32,  1,  2, 35,
   2, 35, 32,  1, 33,  0,  3, 34, 38,  7,  4, 37,  5, 36, 39,  6,
   1, 32, 35,  2, 34,  3,  0, 33, 37,  4,  7, 38,  6, 39, 36,  5,
  36,  5,  6, 39,  7, 38, 37,  4,  0, 33, 34,  3, 35,  2,  1, 32,
  40,  9, 10, 43, 11, 42, 41,  8, 12, 45, 46, 15, 47, 14, 13, 44,
  13, 44, 47, 14, 46, 15, 12, 45, 41,  8, 11, 42, 10, 43, 40,  9,
  14, 47, 44, 13, 45, 12, 15, 46, 42, 11,  8, 41,  9, 40, 43, 10,
  43, 10,  9, 40,  8, 41, 42, 11, 15, 46, 45, 12, 44, 13, 14, 47,
  15, 46, 45, 12, 44, 13, 14, 47, 43, 10,  9, 40,  8, 41, 42, 11,
  42, 11,  8, 41,  9, 40, 43, 10, 14, 47, 44, 13, 45, 12, 15, 46,
  41,  8, 11, 42, 10, 43, 40,  9, 13, 44, 47, 14, 46, 15, 12, 45,
  12, 45, 46, 15, 47, 14, 13, 44, 40,  9, 10, 43, 11, 42, 41,  8
    }, {
        /* Parities of second byte */
   0, 41, 42,  3, 43,  2,  1, 40, 44,  5,  6, 47,  7, 46, 45,  4,
  45,  4,  7, 46,  6, 47, 44,  5,  1, 40, 43,  2, 42,  3,  0, 41,
  46,  7,  4, 45,  5, 44, 47,  6,  2, 43, 40,  1, 41,  0,  3, 42,
   3, 42, 41,  0, 40,  1,  2, 43, 47,  6,  5, 44,  4, 45, 46,  7,
  47,  6,  5, 44,  4, 45, 46,  7,  3, 42, 41,  0, 40,  1,  2, 43,
   2, 43, 40,  1, 41,  0,  3, 42, 46,  7,  4, 45,  5, 44, 47,  6,
   1, 40, 43,  2, 42,  3,  0, 41, 45,  4,  7, 46,  6, 47, 44,  5,
  44,  5,  6, 47,  7, 46, 45,  4,  0, 41, 42,  3, 43,  2,  1, 40,
  48, 25, 26, 51, 27, 50, 49, 24, 28, 53, 54, 31, 55, 30, 29, 52,
  29, 52, 55, 30, 54, 31, 28, 53, 49, 24, 27, 50, 26, 51, 48, 25,
  30, 55, 52, 29, 53, 28, 31, 54, 50, 27, 24, 49, 25, 48, 51, 26,
  51, 26, 25, 48, 24, 49, 50, 27, 31, 54, 53, 28, 52, 29, 30, 55,
  31, 54, 53, 28, 52, 29, 30, 55, 51, 26, 25, 48, 24, 49, 50, 27,
  50, 27, 24, 49, 25, 48, 51, 26, 30, 55, 52, 29, 53, 28, 31, 54,
  49, 24, 27, 50, 26, 51, 48, 25, 29, 52, 55, 30, 54, 31, 28, 53,
  28, 53, 54, 31, 55, 30, 29, 52, 48, 25, 26, 51, 27, 50, 49, 24
    }, {
        /* Parities of third byte */
  63, 14, 13, 60, 12, 61, 62, 15, 11, 58, 57,  8, 56,  9, 10, 59,
  10, 59, 56,  9, 57,  8, 11, 58, 62, 15, 12, 61, 13, 60, 63, 14,
   9, 56, 59, 10, 58, 11,  8, 57, 61, 12, 15, 62, 14, 63, 60, 13,
  60, 13, 14, 63, 15, 62, 61, 12,  8, 57, 58, 11, 59, 10,  9, 56,
   8, 57, 58, 11, 59, 10,  9, 56, 60, 13, 14, 63, 15, 62, 61, 12,
  61, 12, 15, 62, 14, 63, 60, 13,  9, 56, 59, 10, 58, 11,  8, 57,
  62, 15, 12, 61, 13, 60, 63, 14, 10, 59, 56,  9, 57,  8, 11, 58,
  11, 58, 57,  8, 56,  9, 10, 59, 63, 14, 13, 60, 12, 61, 62, 15,
  31, 46, 45, 28, 44, 29, 30, 47, 43, 26, 25, 40, 24, 41, 42, 27,
  42, 27, 24, 41, 25, 40, 43, 26, 30, 47, 44, 29, 45, 28, 31, 46,
  41, 24, 27, 42, 26, 43, 40, 25, 29, 44, 47, 30, 46, 31, 28, 45,
  28, 45, 46, 31, 47, 30, 29, 44, 40, 25, 26, 43, 27, 42, 41, 24,
  40, 25, 26, 43, 27, 42, 41, 24, 28, 45, 46, 31, 47, 30, 29, 44,
  29, 44, 47, 30, 46, 31, 28, 45, 41, 24, 27, 42, 26, 43, 40, 25,
  30, 47, 44, 29, 45, 28, 31, 46, 42, 27, 24, 41, 25, 40, 43, 26,
  43, 26, 25, 40, 24, 41, 42, 27, 31, 46, 45, 28, 44, 29, 30, 47
    }
};

/*
 *  [AleVT]
 *
 *  Table to extract the lower 4 bit from hamm24/18 encoded bytes
 */
const unsigned char hamm24val[256] =
{
      0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,
      2,  2,  2,  2,  3,  3,  3,  3,  2,  2,  2,  2,  3,  3,  3,  3,
      4,  4,  4,  4,  5,  5,  5,  5,  4,  4,  4,  4,  5,  5,  5,  5,
      6,  6,  6,  6,  7,  7,  7,  7,  6,  6,  6,  6,  7,  7,  7,  7,
      8,  8,  8,  8,  9,  9,  9,  9,  8,  8,  8,  8,  9,  9,  9,  9,
     10, 10, 10, 10, 11, 11, 11, 11, 10, 10, 10, 10, 11, 11, 11, 11,
     12, 12, 12, 12, 13, 13, 13, 13, 12, 12, 12, 12, 13, 13, 13, 13,
     14, 14, 14, 14, 15, 15, 15, 15, 14, 14, 14, 14, 15, 15, 15, 15,
      0,  0,  0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,
      2,  2,  2,  2,  3,  3,  3,  3,  2,  2,  2,  2,  3,  3,  3,  3,
      4,  4,  4,  4,  5,  5,  5,  5,  4,  4,  4,  4,  5,  5,  5,  5,
      6,  6,  6,  6,  7,  7,  7,  7,  6,  6,  6,  6,  7,  7,  7,  7,
      8,  8,  8,  8,  9,  9,  9,  9,  8,  8,  8,  8,  9,  9,  9,  9,
     10, 10, 10, 10, 11, 11, 11, 11, 10, 10, 10, 10, 11, 11, 11, 11,
     12, 12, 12, 12, 13, 13, 13, 13, 12, 12, 12, 12, 13, 13, 13, 13,
     14, 14, 14, 14, 15, 15, 15, 15, 14, 14, 14, 14, 15, 15, 15, 15
};

const signed char hamm24err[64] =
{
     0, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
    -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
     0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,
     0,  0,  0,  0,   0,  0,  0,  0,  -1, -1, -1, -1,  -1, -1, -1, -1,
};

/*
 *  [AleVT]
 *
 *  Mapping from parity checks made by table hamm24par to faulty bit
 *  in the decoded 18 bit word.
 */
const unsigned int hamm24cor[64] =
{
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
    0x00000, 0x00000, 0x00000, 0x00001, 0x00000, 0x00002, 0x00004, 0x00008,
    0x00000, 0x00010, 0x00020, 0x00040, 0x00080, 0x00100, 0x00200, 0x00400,
    0x00000, 0x00800, 0x01000, 0x02000, 0x04000, 0x08000, 0x10000, 0x20000,
    0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
};

inline int IsDec(int i)
{
  return ((i & 0x00F) <= 9) && ((i & 0x0F0) <= 0x90);
}

/* struct for page attributes */
typedef struct
{
  unsigned char fg      :6;           /* foreground color */
  unsigned char bg      :6;           /* background color */
  unsigned char charset :6;           /* see enum above */
  unsigned char doubleh :1;           /* double height */
  unsigned char doublew :1;           /* double width */
  /* ignore at Black Background Color Substitution */
  /* black background set by New Background ($1d) instead of start-of-row default or Black Backgr. ($1c) */
  /* or black background set by level 2.5 extensions */
  unsigned char IgnoreAtBlackBgSubst:1;
  unsigned char concealed:1;            /* concealed information */
  unsigned char inverted :1;            /* colors inverted */
  unsigned char flashing :5;            /* flash mode */
  unsigned char diacrit  :4;            /* diacritical mark */
  unsigned char underline:1;            /* Text underlined */
  unsigned char boxwin   :1;            /* Text boxed/windowed */
  unsigned char setX26   :1;            /* Char is set by packet X/26 (no national subset used) */
  unsigned char setG0G2  :7;            /* G0+G2 set designation  */
} TextPageAttr_t;

/* struct for (G)POP/(G)DRCS links for level 2.5, allocated at reception of p27/4 or /5, initialized with 0 after allocation */
typedef struct
{
  unsigned short page;                  /* linked page number */
  unsigned short subpage;               /* 1 bit for each needed (1) subpage */
  unsigned char l25:1;                  /* 1: page required at level 2.5 */
  unsigned char l35:1;                  /* 1: page required at level 3.5 */
  unsigned char drcs:1;                 /* 1: link to (G)DRCS, 0: (G)POP */
  unsigned char local:1;                /* 1: global (G*), 0: local */
} Textp27_t;

/* struct for extension data for level 2.5, allocated at reception, initialized with 0 after allocation */
typedef struct
{
  unsigned char *p26[16];               /* array of pointers to max. 16 designation codes of packet 26 */
  Textp27_t *p27;                       /* array of 4 structs for (G)POP/(G)DRCS links for level 2.5 */
  unsigned short bgr[16];               /* CLUT 2+3, 2*8 colors, 0x0bgr */
  unsigned char DefaultCharset:7;       /* default G0/G2 charset + national option */
  unsigned char LSP:1;                  /* 1: left side panel to be displayed */
  unsigned char SecondCharset:7;        /* second G0 charset */
  unsigned char RSP:1;                  /* 1: right side panel to be displayed */
  unsigned char DefScreenColor:5;       /* default screen color (above and below lines 0..24) */
  unsigned char ColorTableRemapping:3;  /* 1: index in table of CLUTs to use */
  unsigned char DefRowColor:5;          /* default row color (left and right to lines 0..24) */
  unsigned char BlackBgSubst:1;         /* 1: substitute black background (as result of start-of-line or 1c, not 00/10+1d) */
  unsigned char SPL25:1;                /* 1: side panel required at level 2.5 */
  unsigned char p28Received:1;          /* 1: extension data valid (p28/0 received) */
  unsigned char LSPColumns:4;           /* number of columns in left side panel, 0->16, rsp=16-lsp */
} TextExtData_t;

/* struct for pageinfo, max. 16 Bytes, at beginning of each cached page buffer, initialized with 0 after allocation */
typedef struct
{
  unsigned char *p24;                   /* pointer to lines 25+26 (packets 24+25) (2*40 bytes) for FLOF or level 2.5 data */
  TextExtData_t *ext;                   /* pointer to array[16] of data for level 2.5 */
  unsigned char boxed         :1;       /* p0: boxed (newsflash or subtitle) */
  unsigned char nationalvalid :1;       /* p0: national option character subset is valid (no biterror detected) */
  unsigned char national      :3;       /* p0: national option character subset */
  unsigned char function      :3;       /* p28/0: page function */
} TextPageinfo_t;

/* one cached page: struct for pageinfo, 24 lines page data */
typedef struct
{
  TextPageinfo_t pageinfo;
  unsigned char p0[24];                 /* packet 0: center of headline */
  unsigned char data[23*40];            /* packet 1-23 */
} TextCachedPage_t;

typedef struct
{
  short page;
  short language;
} TextSubtitle_t;

typedef struct
{
  bool Valid;
  long Timestamp;
  unsigned char  PageChar[40 * 25];
  TextPageAttr_t PageAtrb[40 * 25];
} TextSubtitleCache_t;

/* main data structure */
typedef struct TextCacheStruct_t
{
  int               CurrentPage[9];
  int               CurrentSubPage[9];
  TextExtData_t    *astP29[9];
  TextCachedPage_t *astCachetable[0x900][0x80];
  unsigned char     SubPageTable[0x900];
  unsigned char     BasicTop[0x900];
  short             FlofPages[0x900][FLOFSIZE];
  char              ADIPTable[0x900][13];
  int               ADIP_PgMax;
  int               ADIP_Pg[10];
  bool              BTTok;
  int               CachedPages;
  int               PageReceiving;
  int               Page;
  int               SubPage;
  bool              PageUpdate;
  int               NationalSubset;
  int               NationalSubsetSecondary;
  bool              ZapSubpageManual;
  TextSubtitle_t    SubtitlePages[8];
  unsigned char     TimeString[8];
  int               vtxtpid;

  /* cachetable for packets 29 (one for each magazine) */
  /* cachetable */
  unsigned char   FullRowColor[25];
  unsigned char   FullScrColor;
  unsigned char   tAPx, tAPy;              /* temporary offset to Active Position for objects */
  short           pop, gpop, drcs, gdrcs;
  unsigned short *ColorTable;

  CStdString      line30;
} TextCacheStruct_t;

/* struct for all Information needed for Page Rendering */
typedef struct
{
  bool PageCatching;
  bool TranspMode;
  bool HintMode;
  bool ShowFlof;
  bool Show39;
  bool Showl25;
  bool ShowHex;
  int ZoomMode;

  int InputCounter;
  int ClearBBColor;
  int Prev_100, Prev_10, Next_10, Next_100;
  int Height;
  int Width;
  int FontHeight;
  int FontWidth;
  int FontWidth_Normal;
  unsigned short rd0[TXT_Color_SIZECOLTABLE];
  unsigned short gn0[TXT_Color_SIZECOLTABLE];
  unsigned short bl0[TXT_Color_SIZECOLTABLE];
  unsigned short tr0[TXT_Color_SIZECOLTABLE];
  TextSubtitleCache_t *SubtitleCache[SUBTITLE_CACHESIZE];
  unsigned char PageChar[25*40];
  TextPageAttr_t PageAtrb[25*40];
  TextPageinfo_t *PageInfo;
  int PosX;
  int PosY;
  int nofirst;
  unsigned char axdrcs[12+1+10+1];
  int TTFShiftX, TTFShiftY; /* parameters for adapting to various TTF fonts */
  bool Boxed;
  int ScreenMode, PrevScreenMode;
  bool DelayStarted;
  unsigned int SubtitleDelay;
} TextRenderInfo_t;

class CDVDTeletextTools
{
public:
  static void NextDec(int *i);
  static void PrevDec(int *i);
  static void Hex2Str(char *s, unsigned int n);
  static signed int deh24(unsigned char *p);
};
