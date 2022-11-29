/*
 * CaseFoldingTableGenerator.cpp
 *
 *  Created on: Nov 1, 2022
 *      Author: fbacher
 *
 *  Creates C++ static data for simple folding for use in StringUtils.
 *
 *  The input data (unicode_fold_upper & unicode_fold_lower) was derived from
 *  https://www.unicode.org/Public/UCD/latest/ucd/CaseFolding.txt. A copy of
 *  this table is included with this source. The layout of the data in CaseFolding.txt
 *  is straightforward and documented fully there. To summarize:
 *    Each line of data is of the format:
 *
 *    <upper_case_hex_value>; Status_code; <folded_case_hex_value>; <comment>
 *
 *    The status_code is one of:
 *    C - line applies to both simple and complex case folding (included in table below)
 *    F - only applies to full case folding (not supported here and so not included
 *        below)
 *    S - line applies to simple case folding (included in table below)
 *    T - special (Turkic) NOT included below since we don't want to follow Turkic
 *        rules.
 *
 *
 *  The tables unicode_fold_upper & unicode_fold_lower are parallel arrays.
 *  If you want to fold the case of character X, you lookup up X in unicode_fold_upper.
 *  If present, the folded case for X, located at unicode_fold_upper[n] is at
 *  unicode_fold_lower[n].
 *
 *  Note that there are some cases where more than one character in unicode_fold_upper
 *  maps to the same unicode_fold_lower. This prevents the ability to have a
 *  "FoldCaseUpper" where case folding produces upper-case versions of letters
 *  instead of lower-case ones. Further, note that although most FoldCase characters
 *  are the lower-case versions. This is not universal.
 *
 *  Running this program produces c++ code that is pasted (after some massaging) into
 *  StringUtils.cpp. The comments below, which describe the pasted code, are also present
 *  in StringUtils.cpp.
 *
 *  The tables below are logically indexed by the 32-bit Unicode value of the
 *  character which is to be case folded. The value found in the table is
 *  the case folded value, or 0 if no such value exists.
 *
 *  A char32_t contains a 32-bit Unicode codepoint, although only 24 bits is
 *  used. The array FOLDCASE_INDEX is indexed by the upper 16-bits of the
 *  the 24-bit codepoint, yielding a pointer to another table indexed by
 *  the lower 8-bits of the codepoint (FOLDCASE_0x0001, etc.) to get the lower-case equivalent
 *  for the original codepoint (see StringUtils::FoldCaseChar).
 *
 *  Specifically, FOLDCASE_0x...[0] contains the number of elements in the
 *  array. This helps reduce the size of the table. All other non-zero elements
 *  contain the Unicode value for a codepoint which can be case folded into another codepoint.
 *   This means that for "A", 0x041, the FoldCase value can be found by:
 *
 *    high_bytes = 0x41 >> 8; => 0
 *    char32_t* table = FOLDCASE_INDEX[high_bytes]; => address of FOLDCASE_0000
 *    uint16_t low_byte = c & 0xFF; => 0x41
 *    char32_t foldedChar = table[low_byte + 1]; => 0x61 'a'
 *
 *
 static const char32_t FOLDCASE_00000[] = {
    0x0df, // This table contains 0xdf values (max 255 + 1 (the added 1 is for this element))
    00000, 00000, 00000, 00000, 00000, // zeros = no FoldCase value
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 0x061, 0x062, 0x063, 0x064, 0x065,
    0x066, 0x067, 0x068, 0x069, 0x06a, 0x06b, 0x06c, 0x06d, 0x06e, 0x06f, 0x070, 0x071, 0x072,
    0x073, 0x074, 0x075, 0x076, 0x077, 0x078, 0x079, 0x07a, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 00000, 00000, 0x3bc, 00000, 00000, 00000, 00000, 00000,
    00000, 00000, 00000, 00000, 00000, 0x0e0, 0x0e1, 0x0e2, 0x0e3, 0x0e4, 0x0e5, 0x0e6, 0x0e7,
    0x0e8, 0x0e9, 0x0ea, 0x0eb, 0x0ec, 0x0ed, 0x0ee, 0x0ef, 0x0f0, 0x0f1, 0x0f2, 0x0f3, 0x0f4,
    0x0f5, 0x0f6, 00000, 0x0f8, 0x0f9, 0x0fa, 0x0fb, 0x0fc, 0x0fd, 0x0fe};
 */

#include <algorithm>
#include <codecvt>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

static const char32_t unicode_fold_upper[] = {
    (char32_t)0x0041,  (char32_t)0x0042,  (char32_t)0x0043,  (char32_t)0x0044,  (char32_t)0x0045,
    (char32_t)0x0046,  (char32_t)0x0047,  (char32_t)0x0048,  (char32_t)0x0049,  (char32_t)0x004A,
    (char32_t)0x004B,  (char32_t)0x004C,  (char32_t)0x004D,  (char32_t)0x004E,  (char32_t)0x004F,
    (char32_t)0x0050,  (char32_t)0x0051,  (char32_t)0x0052,  (char32_t)0x0053,  (char32_t)0x0054,
    (char32_t)0x0055,  (char32_t)0x0056,  (char32_t)0x0057,  (char32_t)0x0058,  (char32_t)0x0059,
    (char32_t)0x005A,  (char32_t)0x00B5,  (char32_t)0x00C0,  (char32_t)0x00C1,  (char32_t)0x00C2,
    (char32_t)0x00C3,  (char32_t)0x00C4,  (char32_t)0x00C5,  (char32_t)0x00C6,  (char32_t)0x00C7,
    (char32_t)0x00C8,  (char32_t)0x00C9,  (char32_t)0x00CA,  (char32_t)0x00CB,  (char32_t)0x00CC,
    (char32_t)0x00CD,  (char32_t)0x00CE,  (char32_t)0x00CF,  (char32_t)0x00D0,  (char32_t)0x00D1,
    (char32_t)0x00D2,  (char32_t)0x00D3,  (char32_t)0x00D4,  (char32_t)0x00D5,  (char32_t)0x00D6,
    (char32_t)0x00D8,  (char32_t)0x00D9,  (char32_t)0x00DA,  (char32_t)0x00DB,  (char32_t)0x00DC,
    (char32_t)0x00DD,  (char32_t)0x00DE,  (char32_t)0x0100,  (char32_t)0x0102,  (char32_t)0x0104,
    (char32_t)0x0106,  (char32_t)0x0108,  (char32_t)0x010A,  (char32_t)0x010C,  (char32_t)0x010E,
    (char32_t)0x0110,  (char32_t)0x0112,  (char32_t)0x0114,  (char32_t)0x0116,  (char32_t)0x0118,
    (char32_t)0x011A,  (char32_t)0x011C,  (char32_t)0x011E,  (char32_t)0x0120,  (char32_t)0x0122,
    (char32_t)0x0124,  (char32_t)0x0126,  (char32_t)0x0128,  (char32_t)0x012A,  (char32_t)0x012C,
    (char32_t)0x012E,  (char32_t)0x0132,  (char32_t)0x0134,  (char32_t)0x0136,  (char32_t)0x0139,
    (char32_t)0x013B,  (char32_t)0x013D,  (char32_t)0x013F,  (char32_t)0x0141,  (char32_t)0x0143,
    (char32_t)0x0145,  (char32_t)0x0147,  (char32_t)0x014A,  (char32_t)0x014C,  (char32_t)0x014E,
    (char32_t)0x0150,  (char32_t)0x0152,  (char32_t)0x0154,  (char32_t)0x0156,  (char32_t)0x0158,
    (char32_t)0x015A,  (char32_t)0x015C,  (char32_t)0x015E,  (char32_t)0x0160,  (char32_t)0x0162,
    (char32_t)0x0164,  (char32_t)0x0166,  (char32_t)0x0168,  (char32_t)0x016A,  (char32_t)0x016C,
    (char32_t)0x016E,  (char32_t)0x0170,  (char32_t)0x0172,  (char32_t)0x0174,  (char32_t)0x0176,
    (char32_t)0x0178,  (char32_t)0x0179,  (char32_t)0x017B,  (char32_t)0x017D,  (char32_t)0x017F,
    (char32_t)0x0181,  (char32_t)0x0182,  (char32_t)0x0184,  (char32_t)0x0186,  (char32_t)0x0187,
    (char32_t)0x0189,  (char32_t)0x018A,  (char32_t)0x018B,  (char32_t)0x018E,  (char32_t)0x018F,
    (char32_t)0x0190,  (char32_t)0x0191,  (char32_t)0x0193,  (char32_t)0x0194,  (char32_t)0x0196,
    (char32_t)0x0197,  (char32_t)0x0198,  (char32_t)0x019C,  (char32_t)0x019D,  (char32_t)0x019F,
    (char32_t)0x01A0,  (char32_t)0x01A2,  (char32_t)0x01A4,  (char32_t)0x01A6,  (char32_t)0x01A7,
    (char32_t)0x01A9,  (char32_t)0x01AC,  (char32_t)0x01AE,  (char32_t)0x01AF,  (char32_t)0x01B1,
    (char32_t)0x01B2,  (char32_t)0x01B3,  (char32_t)0x01B5,  (char32_t)0x01B7,  (char32_t)0x01B8,
    (char32_t)0x01BC,  (char32_t)0x01C4,  (char32_t)0x01C5,  (char32_t)0x01C7,  (char32_t)0x01C8,
    (char32_t)0x01CA,  (char32_t)0x01CB,  (char32_t)0x01CD,  (char32_t)0x01CF,  (char32_t)0x01D1,
    (char32_t)0x01D3,  (char32_t)0x01D5,  (char32_t)0x01D7,  (char32_t)0x01D9,  (char32_t)0x01DB,
    (char32_t)0x01DE,  (char32_t)0x01E0,  (char32_t)0x01E2,  (char32_t)0x01E4,  (char32_t)0x01E6,
    (char32_t)0x01E8,  (char32_t)0x01EA,  (char32_t)0x01EC,  (char32_t)0x01EE,  (char32_t)0x01F1,
    (char32_t)0x01F2,  (char32_t)0x01F4,  (char32_t)0x01F6,  (char32_t)0x01F7,  (char32_t)0x01F8,
    (char32_t)0x01FA,  (char32_t)0x01FC,  (char32_t)0x01FE,  (char32_t)0x0200,  (char32_t)0x0202,
    (char32_t)0x0204,  (char32_t)0x0206,  (char32_t)0x0208,  (char32_t)0x020A,  (char32_t)0x020C,
    (char32_t)0x020E,  (char32_t)0x0210,  (char32_t)0x0212,  (char32_t)0x0214,  (char32_t)0x0216,
    (char32_t)0x0218,  (char32_t)0x021A,  (char32_t)0x021C,  (char32_t)0x021E,  (char32_t)0x0220,
    (char32_t)0x0222,  (char32_t)0x0224,  (char32_t)0x0226,  (char32_t)0x0228,  (char32_t)0x022A,
    (char32_t)0x022C,  (char32_t)0x022E,  (char32_t)0x0230,  (char32_t)0x0232,  (char32_t)0x023A,
    (char32_t)0x023B,  (char32_t)0x023D,  (char32_t)0x023E,  (char32_t)0x0241,  (char32_t)0x0243,
    (char32_t)0x0244,  (char32_t)0x0245,  (char32_t)0x0246,  (char32_t)0x0248,  (char32_t)0x024A,
    (char32_t)0x024C,  (char32_t)0x024E,  (char32_t)0x0345,  (char32_t)0x0370,  (char32_t)0x0372,
    (char32_t)0x0376,  (char32_t)0x037F,  (char32_t)0x0386,  (char32_t)0x0388,  (char32_t)0x0389,
    (char32_t)0x038A,  (char32_t)0x038C,  (char32_t)0x038E,  (char32_t)0x038F,  (char32_t)0x0391,
    (char32_t)0x0392,  (char32_t)0x0393,  (char32_t)0x0394,  (char32_t)0x0395,  (char32_t)0x0396,
    (char32_t)0x0397,  (char32_t)0x0398,  (char32_t)0x0399,  (char32_t)0x039A,  (char32_t)0x039B,
    (char32_t)0x039C,  (char32_t)0x039D,  (char32_t)0x039E,  (char32_t)0x039F,  (char32_t)0x03A0,
    (char32_t)0x03A1,  (char32_t)0x03A3,  (char32_t)0x03A4,  (char32_t)0x03A5,  (char32_t)0x03A6,
    (char32_t)0x03A7,  (char32_t)0x03A8,  (char32_t)0x03A9,  (char32_t)0x03AA,  (char32_t)0x03AB,
    (char32_t)0x03C2,  (char32_t)0x03CF,  (char32_t)0x03D0,  (char32_t)0x03D1,  (char32_t)0x03D5,
    (char32_t)0x03D6,  (char32_t)0x03D8,  (char32_t)0x03DA,  (char32_t)0x03DC,  (char32_t)0x03DE,
    (char32_t)0x03E0,  (char32_t)0x03E2,  (char32_t)0x03E4,  (char32_t)0x03E6,  (char32_t)0x03E8,
    (char32_t)0x03EA,  (char32_t)0x03EC,  (char32_t)0x03EE,  (char32_t)0x03F0,  (char32_t)0x03F1,
    (char32_t)0x03F4,  (char32_t)0x03F5,  (char32_t)0x03F7,  (char32_t)0x03F9,  (char32_t)0x03FA,
    (char32_t)0x03FD,  (char32_t)0x03FE,  (char32_t)0x03FF,  (char32_t)0x0400,  (char32_t)0x0401,
    (char32_t)0x0402,  (char32_t)0x0403,  (char32_t)0x0404,  (char32_t)0x0405,  (char32_t)0x0406,
    (char32_t)0x0407,  (char32_t)0x0408,  (char32_t)0x0409,  (char32_t)0x040A,  (char32_t)0x040B,
    (char32_t)0x040C,  (char32_t)0x040D,  (char32_t)0x040E,  (char32_t)0x040F,  (char32_t)0x0410,
    (char32_t)0x0411,  (char32_t)0x0412,  (char32_t)0x0413,  (char32_t)0x0414,  (char32_t)0x0415,
    (char32_t)0x0416,  (char32_t)0x0417,  (char32_t)0x0418,  (char32_t)0x0419,  (char32_t)0x041A,
    (char32_t)0x041B,  (char32_t)0x041C,  (char32_t)0x041D,  (char32_t)0x041E,  (char32_t)0x041F,
    (char32_t)0x0420,  (char32_t)0x0421,  (char32_t)0x0422,  (char32_t)0x0423,  (char32_t)0x0424,
    (char32_t)0x0425,  (char32_t)0x0426,  (char32_t)0x0427,  (char32_t)0x0428,  (char32_t)0x0429,
    (char32_t)0x042A,  (char32_t)0x042B,  (char32_t)0x042C,  (char32_t)0x042D,  (char32_t)0x042E,
    (char32_t)0x042F,  (char32_t)0x0460,  (char32_t)0x0462,  (char32_t)0x0464,  (char32_t)0x0466,
    (char32_t)0x0468,  (char32_t)0x046A,  (char32_t)0x046C,  (char32_t)0x046E,  (char32_t)0x0470,
    (char32_t)0x0472,  (char32_t)0x0474,  (char32_t)0x0476,  (char32_t)0x0478,  (char32_t)0x047A,
    (char32_t)0x047C,  (char32_t)0x047E,  (char32_t)0x0480,  (char32_t)0x048A,  (char32_t)0x048C,
    (char32_t)0x048E,  (char32_t)0x0490,  (char32_t)0x0492,  (char32_t)0x0494,  (char32_t)0x0496,
    (char32_t)0x0498,  (char32_t)0x049A,  (char32_t)0x049C,  (char32_t)0x049E,  (char32_t)0x04A0,
    (char32_t)0x04A2,  (char32_t)0x04A4,  (char32_t)0x04A6,  (char32_t)0x04A8,  (char32_t)0x04AA,
    (char32_t)0x04AC,  (char32_t)0x04AE,  (char32_t)0x04B0,  (char32_t)0x04B2,  (char32_t)0x04B4,
    (char32_t)0x04B6,  (char32_t)0x04B8,  (char32_t)0x04BA,  (char32_t)0x04BC,  (char32_t)0x04BE,
    (char32_t)0x04C0,  (char32_t)0x04C1,  (char32_t)0x04C3,  (char32_t)0x04C5,  (char32_t)0x04C7,
    (char32_t)0x04C9,  (char32_t)0x04CB,  (char32_t)0x04CD,  (char32_t)0x04D0,  (char32_t)0x04D2,
    (char32_t)0x04D4,  (char32_t)0x04D6,  (char32_t)0x04D8,  (char32_t)0x04DA,  (char32_t)0x04DC,
    (char32_t)0x04DE,  (char32_t)0x04E0,  (char32_t)0x04E2,  (char32_t)0x04E4,  (char32_t)0x04E6,
    (char32_t)0x04E8,  (char32_t)0x04EA,  (char32_t)0x04EC,  (char32_t)0x04EE,  (char32_t)0x04F0,
    (char32_t)0x04F2,  (char32_t)0x04F4,  (char32_t)0x04F6,  (char32_t)0x04F8,  (char32_t)0x04FA,
    (char32_t)0x04FC,  (char32_t)0x04FE,  (char32_t)0x0500,  (char32_t)0x0502,  (char32_t)0x0504,
    (char32_t)0x0506,  (char32_t)0x0508,  (char32_t)0x050A,  (char32_t)0x050C,  (char32_t)0x050E,
    (char32_t)0x0510,  (char32_t)0x0512,  (char32_t)0x0514,  (char32_t)0x0516,  (char32_t)0x0518,
    (char32_t)0x051A,  (char32_t)0x051C,  (char32_t)0x051E,  (char32_t)0x0520,  (char32_t)0x0522,
    (char32_t)0x0524,  (char32_t)0x0526,  (char32_t)0x0528,  (char32_t)0x052A,  (char32_t)0x052C,
    (char32_t)0x052E,  (char32_t)0x0531,  (char32_t)0x0532,  (char32_t)0x0533,  (char32_t)0x0534,
    (char32_t)0x0535,  (char32_t)0x0536,  (char32_t)0x0537,  (char32_t)0x0538,  (char32_t)0x0539,
    (char32_t)0x053A,  (char32_t)0x053B,  (char32_t)0x053C,  (char32_t)0x053D,  (char32_t)0x053E,
    (char32_t)0x053F,  (char32_t)0x0540,  (char32_t)0x0541,  (char32_t)0x0542,  (char32_t)0x0543,
    (char32_t)0x0544,  (char32_t)0x0545,  (char32_t)0x0546,  (char32_t)0x0547,  (char32_t)0x0548,
    (char32_t)0x0549,  (char32_t)0x054A,  (char32_t)0x054B,  (char32_t)0x054C,  (char32_t)0x054D,
    (char32_t)0x054E,  (char32_t)0x054F,  (char32_t)0x0550,  (char32_t)0x0551,  (char32_t)0x0552,
    (char32_t)0x0553,  (char32_t)0x0554,  (char32_t)0x0555,  (char32_t)0x0556,  (char32_t)0x10A0,
    (char32_t)0x10A1,  (char32_t)0x10A2,  (char32_t)0x10A3,  (char32_t)0x10A4,  (char32_t)0x10A5,
    (char32_t)0x10A6,  (char32_t)0x10A7,  (char32_t)0x10A8,  (char32_t)0x10A9,  (char32_t)0x10AA,
    (char32_t)0x10AB,  (char32_t)0x10AC,  (char32_t)0x10AD,  (char32_t)0x10AE,  (char32_t)0x10AF,
    (char32_t)0x10B0,  (char32_t)0x10B1,  (char32_t)0x10B2,  (char32_t)0x10B3,  (char32_t)0x10B4,
    (char32_t)0x10B5,  (char32_t)0x10B6,  (char32_t)0x10B7,  (char32_t)0x10B8,  (char32_t)0x10B9,
    (char32_t)0x10BA,  (char32_t)0x10BB,  (char32_t)0x10BC,  (char32_t)0x10BD,  (char32_t)0x10BE,
    (char32_t)0x10BF,  (char32_t)0x10C0,  (char32_t)0x10C1,  (char32_t)0x10C2,  (char32_t)0x10C3,
    (char32_t)0x10C4,  (char32_t)0x10C5,  (char32_t)0x10C7,  (char32_t)0x10CD,  (char32_t)0x13F8,
    (char32_t)0x13F9,  (char32_t)0x13FA,  (char32_t)0x13FB,  (char32_t)0x13FC,  (char32_t)0x13FD,
    (char32_t)0x1C80,  (char32_t)0x1C81,  (char32_t)0x1C82,  (char32_t)0x1C83,  (char32_t)0x1C84,
    (char32_t)0x1C85,  (char32_t)0x1C86,  (char32_t)0x1C87,  (char32_t)0x1C88,  (char32_t)0x1C90,
    (char32_t)0x1C91,  (char32_t)0x1C92,  (char32_t)0x1C93,  (char32_t)0x1C94,  (char32_t)0x1C95,
    (char32_t)0x1C96,  (char32_t)0x1C97,  (char32_t)0x1C98,  (char32_t)0x1C99,  (char32_t)0x1C9A,
    (char32_t)0x1C9B,  (char32_t)0x1C9C,  (char32_t)0x1C9D,  (char32_t)0x1C9E,  (char32_t)0x1C9F,
    (char32_t)0x1CA0,  (char32_t)0x1CA1,  (char32_t)0x1CA2,  (char32_t)0x1CA3,  (char32_t)0x1CA4,
    (char32_t)0x1CA5,  (char32_t)0x1CA6,  (char32_t)0x1CA7,  (char32_t)0x1CA8,  (char32_t)0x1CA9,
    (char32_t)0x1CAA,  (char32_t)0x1CAB,  (char32_t)0x1CAC,  (char32_t)0x1CAD,  (char32_t)0x1CAE,
    (char32_t)0x1CAF,  (char32_t)0x1CB0,  (char32_t)0x1CB1,  (char32_t)0x1CB2,  (char32_t)0x1CB3,
    (char32_t)0x1CB4,  (char32_t)0x1CB5,  (char32_t)0x1CB6,  (char32_t)0x1CB7,  (char32_t)0x1CB8,
    (char32_t)0x1CB9,  (char32_t)0x1CBA,  (char32_t)0x1CBD,  (char32_t)0x1CBE,  (char32_t)0x1CBF,
    (char32_t)0x1E00,  (char32_t)0x1E02,  (char32_t)0x1E04,  (char32_t)0x1E06,  (char32_t)0x1E08,
    (char32_t)0x1E0A,  (char32_t)0x1E0C,  (char32_t)0x1E0E,  (char32_t)0x1E10,  (char32_t)0x1E12,
    (char32_t)0x1E14,  (char32_t)0x1E16,  (char32_t)0x1E18,  (char32_t)0x1E1A,  (char32_t)0x1E1C,
    (char32_t)0x1E1E,  (char32_t)0x1E20,  (char32_t)0x1E22,  (char32_t)0x1E24,  (char32_t)0x1E26,
    (char32_t)0x1E28,  (char32_t)0x1E2A,  (char32_t)0x1E2C,  (char32_t)0x1E2E,  (char32_t)0x1E30,
    (char32_t)0x1E32,  (char32_t)0x1E34,  (char32_t)0x1E36,  (char32_t)0x1E38,  (char32_t)0x1E3A,
    (char32_t)0x1E3C,  (char32_t)0x1E3E,  (char32_t)0x1E40,  (char32_t)0x1E42,  (char32_t)0x1E44,
    (char32_t)0x1E46,  (char32_t)0x1E48,  (char32_t)0x1E4A,  (char32_t)0x1E4C,  (char32_t)0x1E4E,
    (char32_t)0x1E50,  (char32_t)0x1E52,  (char32_t)0x1E54,  (char32_t)0x1E56,  (char32_t)0x1E58,
    (char32_t)0x1E5A,  (char32_t)0x1E5C,  (char32_t)0x1E5E,  (char32_t)0x1E60,  (char32_t)0x1E62,
    (char32_t)0x1E64,  (char32_t)0x1E66,  (char32_t)0x1E68,  (char32_t)0x1E6A,  (char32_t)0x1E6C,
    (char32_t)0x1E6E,  (char32_t)0x1E70,  (char32_t)0x1E72,  (char32_t)0x1E74,  (char32_t)0x1E76,
    (char32_t)0x1E78,  (char32_t)0x1E7A,  (char32_t)0x1E7C,  (char32_t)0x1E7E,  (char32_t)0x1E80,
    (char32_t)0x1E82,  (char32_t)0x1E84,  (char32_t)0x1E86,  (char32_t)0x1E88,  (char32_t)0x1E8A,
    (char32_t)0x1E8C,  (char32_t)0x1E8E,  (char32_t)0x1E90,  (char32_t)0x1E92,  (char32_t)0x1E94,
    (char32_t)0x1E9B,  (char32_t)0x1E9E,  (char32_t)0x1EA0,  (char32_t)0x1EA2,  (char32_t)0x1EA4,
    (char32_t)0x1EA6,  (char32_t)0x1EA8,  (char32_t)0x1EAA,  (char32_t)0x1EAC,  (char32_t)0x1EAE,
    (char32_t)0x1EB0,  (char32_t)0x1EB2,  (char32_t)0x1EB4,  (char32_t)0x1EB6,  (char32_t)0x1EB8,
    (char32_t)0x1EBA,  (char32_t)0x1EBC,  (char32_t)0x1EBE,  (char32_t)0x1EC0,  (char32_t)0x1EC2,
    (char32_t)0x1EC4,  (char32_t)0x1EC6,  (char32_t)0x1EC8,  (char32_t)0x1ECA,  (char32_t)0x1ECC,
    (char32_t)0x1ECE,  (char32_t)0x1ED0,  (char32_t)0x1ED2,  (char32_t)0x1ED4,  (char32_t)0x1ED6,
    (char32_t)0x1ED8,  (char32_t)0x1EDA,  (char32_t)0x1EDC,  (char32_t)0x1EDE,  (char32_t)0x1EE0,
    (char32_t)0x1EE2,  (char32_t)0x1EE4,  (char32_t)0x1EE6,  (char32_t)0x1EE8,  (char32_t)0x1EEA,
    (char32_t)0x1EEC,  (char32_t)0x1EEE,  (char32_t)0x1EF0,  (char32_t)0x1EF2,  (char32_t)0x1EF4,
    (char32_t)0x1EF6,  (char32_t)0x1EF8,  (char32_t)0x1EFA,  (char32_t)0x1EFC,  (char32_t)0x1EFE,
    (char32_t)0x1F08,  (char32_t)0x1F09,  (char32_t)0x1F0A,  (char32_t)0x1F0B,  (char32_t)0x1F0C,
    (char32_t)0x1F0D,  (char32_t)0x1F0E,  (char32_t)0x1F0F,  (char32_t)0x1F18,  (char32_t)0x1F19,
    (char32_t)0x1F1A,  (char32_t)0x1F1B,  (char32_t)0x1F1C,  (char32_t)0x1F1D,  (char32_t)0x1F28,
    (char32_t)0x1F29,  (char32_t)0x1F2A,  (char32_t)0x1F2B,  (char32_t)0x1F2C,  (char32_t)0x1F2D,
    (char32_t)0x1F2E,  (char32_t)0x1F2F,  (char32_t)0x1F38,  (char32_t)0x1F39,  (char32_t)0x1F3A,
    (char32_t)0x1F3B,  (char32_t)0x1F3C,  (char32_t)0x1F3D,  (char32_t)0x1F3E,  (char32_t)0x1F3F,
    (char32_t)0x1F48,  (char32_t)0x1F49,  (char32_t)0x1F4A,  (char32_t)0x1F4B,  (char32_t)0x1F4C,
    (char32_t)0x1F4D,  (char32_t)0x1F59,  (char32_t)0x1F5B,  (char32_t)0x1F5D,  (char32_t)0x1F5F,
    (char32_t)0x1F68,  (char32_t)0x1F69,  (char32_t)0x1F6A,  (char32_t)0x1F6B,  (char32_t)0x1F6C,
    (char32_t)0x1F6D,  (char32_t)0x1F6E,  (char32_t)0x1F6F,  (char32_t)0x1F88,  (char32_t)0x1F89,
    (char32_t)0x1F8A,  (char32_t)0x1F8B,  (char32_t)0x1F8C,  (char32_t)0x1F8D,  (char32_t)0x1F8E,
    (char32_t)0x1F8F,  (char32_t)0x1F98,  (char32_t)0x1F99,  (char32_t)0x1F9A,  (char32_t)0x1F9B,
    (char32_t)0x1F9C,  (char32_t)0x1F9D,  (char32_t)0x1F9E,  (char32_t)0x1F9F,  (char32_t)0x1FA8,
    (char32_t)0x1FA9,  (char32_t)0x1FAA,  (char32_t)0x1FAB,  (char32_t)0x1FAC,  (char32_t)0x1FAD,
    (char32_t)0x1FAE,  (char32_t)0x1FAF,  (char32_t)0x1FB8,  (char32_t)0x1FB9,  (char32_t)0x1FBA,
    (char32_t)0x1FBB,  (char32_t)0x1FBC,  (char32_t)0x1FBE,  (char32_t)0x1FC8,  (char32_t)0x1FC9,
    (char32_t)0x1FCA,  (char32_t)0x1FCB,  (char32_t)0x1FCC,  (char32_t)0x1FD8,  (char32_t)0x1FD9,
    (char32_t)0x1FDA,  (char32_t)0x1FDB,  (char32_t)0x1FE8,  (char32_t)0x1FE9,  (char32_t)0x1FEA,
    (char32_t)0x1FEB,  (char32_t)0x1FEC,  (char32_t)0x1FF8,  (char32_t)0x1FF9,  (char32_t)0x1FFA,
    (char32_t)0x1FFB,  (char32_t)0x1FFC,  (char32_t)0x2126,  (char32_t)0x212A,  (char32_t)0x212B,
    (char32_t)0x2132,  (char32_t)0x2160,  (char32_t)0x2161,  (char32_t)0x2162,  (char32_t)0x2163,
    (char32_t)0x2164,  (char32_t)0x2165,  (char32_t)0x2166,  (char32_t)0x2167,  (char32_t)0x2168,
    (char32_t)0x2169,  (char32_t)0x216A,  (char32_t)0x216B,  (char32_t)0x216C,  (char32_t)0x216D,
    (char32_t)0x216E,  (char32_t)0x216F,  (char32_t)0x2183,  (char32_t)0x24B6,  (char32_t)0x24B7,
    (char32_t)0x24B8,  (char32_t)0x24B9,  (char32_t)0x24BA,  (char32_t)0x24BB,  (char32_t)0x24BC,
    (char32_t)0x24BD,  (char32_t)0x24BE,  (char32_t)0x24BF,  (char32_t)0x24C0,  (char32_t)0x24C1,
    (char32_t)0x24C2,  (char32_t)0x24C3,  (char32_t)0x24C4,  (char32_t)0x24C5,  (char32_t)0x24C6,
    (char32_t)0x24C7,  (char32_t)0x24C8,  (char32_t)0x24C9,  (char32_t)0x24CA,  (char32_t)0x24CB,
    (char32_t)0x24CC,  (char32_t)0x24CD,  (char32_t)0x24CE,  (char32_t)0x24CF,  (char32_t)0x2C00,
    (char32_t)0x2C01,  (char32_t)0x2C02,  (char32_t)0x2C03,  (char32_t)0x2C04,  (char32_t)0x2C05,
    (char32_t)0x2C06,  (char32_t)0x2C07,  (char32_t)0x2C08,  (char32_t)0x2C09,  (char32_t)0x2C0A,
    (char32_t)0x2C0B,  (char32_t)0x2C0C,  (char32_t)0x2C0D,  (char32_t)0x2C0E,  (char32_t)0x2C0F,
    (char32_t)0x2C10,  (char32_t)0x2C11,  (char32_t)0x2C12,  (char32_t)0x2C13,  (char32_t)0x2C14,
    (char32_t)0x2C15,  (char32_t)0x2C16,  (char32_t)0x2C17,  (char32_t)0x2C18,  (char32_t)0x2C19,
    (char32_t)0x2C1A,  (char32_t)0x2C1B,  (char32_t)0x2C1C,  (char32_t)0x2C1D,  (char32_t)0x2C1E,
    (char32_t)0x2C1F,  (char32_t)0x2C20,  (char32_t)0x2C21,  (char32_t)0x2C22,  (char32_t)0x2C23,
    (char32_t)0x2C24,  (char32_t)0x2C25,  (char32_t)0x2C26,  (char32_t)0x2C27,  (char32_t)0x2C28,
    (char32_t)0x2C29,  (char32_t)0x2C2A,  (char32_t)0x2C2B,  (char32_t)0x2C2C,  (char32_t)0x2C2D,
    (char32_t)0x2C2E,  (char32_t)0x2C2F,  (char32_t)0x2C60,  (char32_t)0x2C62,  (char32_t)0x2C63,
    (char32_t)0x2C64,  (char32_t)0x2C67,  (char32_t)0x2C69,  (char32_t)0x2C6B,  (char32_t)0x2C6D,
    (char32_t)0x2C6E,  (char32_t)0x2C6F,  (char32_t)0x2C70,  (char32_t)0x2C72,  (char32_t)0x2C75,
    (char32_t)0x2C7E,  (char32_t)0x2C7F,  (char32_t)0x2C80,  (char32_t)0x2C82,  (char32_t)0x2C84,
    (char32_t)0x2C86,  (char32_t)0x2C88,  (char32_t)0x2C8A,  (char32_t)0x2C8C,  (char32_t)0x2C8E,
    (char32_t)0x2C90,  (char32_t)0x2C92,  (char32_t)0x2C94,  (char32_t)0x2C96,  (char32_t)0x2C98,
    (char32_t)0x2C9A,  (char32_t)0x2C9C,  (char32_t)0x2C9E,  (char32_t)0x2CA0,  (char32_t)0x2CA2,
    (char32_t)0x2CA4,  (char32_t)0x2CA6,  (char32_t)0x2CA8,  (char32_t)0x2CAA,  (char32_t)0x2CAC,
    (char32_t)0x2CAE,  (char32_t)0x2CB0,  (char32_t)0x2CB2,  (char32_t)0x2CB4,  (char32_t)0x2CB6,
    (char32_t)0x2CB8,  (char32_t)0x2CBA,  (char32_t)0x2CBC,  (char32_t)0x2CBE,  (char32_t)0x2CC0,
    (char32_t)0x2CC2,  (char32_t)0x2CC4,  (char32_t)0x2CC6,  (char32_t)0x2CC8,  (char32_t)0x2CCA,
    (char32_t)0x2CCC,  (char32_t)0x2CCE,  (char32_t)0x2CD0,  (char32_t)0x2CD2,  (char32_t)0x2CD4,
    (char32_t)0x2CD6,  (char32_t)0x2CD8,  (char32_t)0x2CDA,  (char32_t)0x2CDC,  (char32_t)0x2CDE,
    (char32_t)0x2CE0,  (char32_t)0x2CE2,  (char32_t)0x2CEB,  (char32_t)0x2CED,  (char32_t)0x2CF2,
    (char32_t)0xA640,  (char32_t)0xA642,  (char32_t)0xA644,  (char32_t)0xA646,  (char32_t)0xA648,
    (char32_t)0xA64A,  (char32_t)0xA64C,  (char32_t)0xA64E,  (char32_t)0xA650,  (char32_t)0xA652,
    (char32_t)0xA654,  (char32_t)0xA656,  (char32_t)0xA658,  (char32_t)0xA65A,  (char32_t)0xA65C,
    (char32_t)0xA65E,  (char32_t)0xA660,  (char32_t)0xA662,  (char32_t)0xA664,  (char32_t)0xA666,
    (char32_t)0xA668,  (char32_t)0xA66A,  (char32_t)0xA66C,  (char32_t)0xA680,  (char32_t)0xA682,
    (char32_t)0xA684,  (char32_t)0xA686,  (char32_t)0xA688,  (char32_t)0xA68A,  (char32_t)0xA68C,
    (char32_t)0xA68E,  (char32_t)0xA690,  (char32_t)0xA692,  (char32_t)0xA694,  (char32_t)0xA696,
    (char32_t)0xA698,  (char32_t)0xA69A,  (char32_t)0xA722,  (char32_t)0xA724,  (char32_t)0xA726,
    (char32_t)0xA728,  (char32_t)0xA72A,  (char32_t)0xA72C,  (char32_t)0xA72E,  (char32_t)0xA732,
    (char32_t)0xA734,  (char32_t)0xA736,  (char32_t)0xA738,  (char32_t)0xA73A,  (char32_t)0xA73C,
    (char32_t)0xA73E,  (char32_t)0xA740,  (char32_t)0xA742,  (char32_t)0xA744,  (char32_t)0xA746,
    (char32_t)0xA748,  (char32_t)0xA74A,  (char32_t)0xA74C,  (char32_t)0xA74E,  (char32_t)0xA750,
    (char32_t)0xA752,  (char32_t)0xA754,  (char32_t)0xA756,  (char32_t)0xA758,  (char32_t)0xA75A,
    (char32_t)0xA75C,  (char32_t)0xA75E,  (char32_t)0xA760,  (char32_t)0xA762,  (char32_t)0xA764,
    (char32_t)0xA766,  (char32_t)0xA768,  (char32_t)0xA76A,  (char32_t)0xA76C,  (char32_t)0xA76E,
    (char32_t)0xA779,  (char32_t)0xA77B,  (char32_t)0xA77D,  (char32_t)0xA77E,  (char32_t)0xA780,
    (char32_t)0xA782,  (char32_t)0xA784,  (char32_t)0xA786,  (char32_t)0xA78B,  (char32_t)0xA78D,
    (char32_t)0xA790,  (char32_t)0xA792,  (char32_t)0xA796,  (char32_t)0xA798,  (char32_t)0xA79A,
    (char32_t)0xA79C,  (char32_t)0xA79E,  (char32_t)0xA7A0,  (char32_t)0xA7A2,  (char32_t)0xA7A4,
    (char32_t)0xA7A6,  (char32_t)0xA7A8,  (char32_t)0xA7AA,  (char32_t)0xA7AB,  (char32_t)0xA7AC,
    (char32_t)0xA7AD,  (char32_t)0xA7AE,  (char32_t)0xA7B0,  (char32_t)0xA7B1,  (char32_t)0xA7B2,
    (char32_t)0xA7B3,  (char32_t)0xA7B4,  (char32_t)0xA7B6,  (char32_t)0xA7B8,  (char32_t)0xA7BA,
    (char32_t)0xA7BC,  (char32_t)0xA7BE,  (char32_t)0xA7C0,  (char32_t)0xA7C2,  (char32_t)0xA7C4,
    (char32_t)0xA7C5,  (char32_t)0xA7C6,  (char32_t)0xA7C7,  (char32_t)0xA7C9,  (char32_t)0xA7D0,
    (char32_t)0xA7D6,  (char32_t)0xA7D8,  (char32_t)0xA7F5,  (char32_t)0xAB70,  (char32_t)0xAB71,
    (char32_t)0xAB72,  (char32_t)0xAB73,  (char32_t)0xAB74,  (char32_t)0xAB75,  (char32_t)0xAB76,
    (char32_t)0xAB77,  (char32_t)0xAB78,  (char32_t)0xAB79,  (char32_t)0xAB7A,  (char32_t)0xAB7B,
    (char32_t)0xAB7C,  (char32_t)0xAB7D,  (char32_t)0xAB7E,  (char32_t)0xAB7F,  (char32_t)0xAB80,
    (char32_t)0xAB81,  (char32_t)0xAB82,  (char32_t)0xAB83,  (char32_t)0xAB84,  (char32_t)0xAB85,
    (char32_t)0xAB86,  (char32_t)0xAB87,  (char32_t)0xAB88,  (char32_t)0xAB89,  (char32_t)0xAB8A,
    (char32_t)0xAB8B,  (char32_t)0xAB8C,  (char32_t)0xAB8D,  (char32_t)0xAB8E,  (char32_t)0xAB8F,
    (char32_t)0xAB90,  (char32_t)0xAB91,  (char32_t)0xAB92,  (char32_t)0xAB93,  (char32_t)0xAB94,
    (char32_t)0xAB95,  (char32_t)0xAB96,  (char32_t)0xAB97,  (char32_t)0xAB98,  (char32_t)0xAB99,
    (char32_t)0xAB9A,  (char32_t)0xAB9B,  (char32_t)0xAB9C,  (char32_t)0xAB9D,  (char32_t)0xAB9E,
    (char32_t)0xAB9F,  (char32_t)0xABA0,  (char32_t)0xABA1,  (char32_t)0xABA2,  (char32_t)0xABA3,
    (char32_t)0xABA4,  (char32_t)0xABA5,  (char32_t)0xABA6,  (char32_t)0xABA7,  (char32_t)0xABA8,
    (char32_t)0xABA9,  (char32_t)0xABAA,  (char32_t)0xABAB,  (char32_t)0xABAC,  (char32_t)0xABAD,
    (char32_t)0xABAE,  (char32_t)0xABAF,  (char32_t)0xABB0,  (char32_t)0xABB1,  (char32_t)0xABB2,
    (char32_t)0xABB3,  (char32_t)0xABB4,  (char32_t)0xABB5,  (char32_t)0xABB6,  (char32_t)0xABB7,
    (char32_t)0xABB8,  (char32_t)0xABB9,  (char32_t)0xABBA,  (char32_t)0xABBB,  (char32_t)0xABBC,
    (char32_t)0xABBD,  (char32_t)0xABBE,  (char32_t)0xABBF,  (char32_t)0xFF21,  (char32_t)0xFF22,
    (char32_t)0xFF23,  (char32_t)0xFF24,  (char32_t)0xFF25,  (char32_t)0xFF26,  (char32_t)0xFF27,
    (char32_t)0xFF28,  (char32_t)0xFF29,  (char32_t)0xFF2A,  (char32_t)0xFF2B,  (char32_t)0xFF2C,
    (char32_t)0xFF2D,  (char32_t)0xFF2E,  (char32_t)0xFF2F,  (char32_t)0xFF30,  (char32_t)0xFF31,
    (char32_t)0xFF32,  (char32_t)0xFF33,  (char32_t)0xFF34,  (char32_t)0xFF35,  (char32_t)0xFF36,
    (char32_t)0xFF37,  (char32_t)0xFF38,  (char32_t)0xFF39,  (char32_t)0xFF3A,  (char32_t)0x10400,
    (char32_t)0x10401, (char32_t)0x10402, (char32_t)0x10403, (char32_t)0x10404, (char32_t)0x10405,
    (char32_t)0x10406, (char32_t)0x10407, (char32_t)0x10408, (char32_t)0x10409, (char32_t)0x1040A,
    (char32_t)0x1040B, (char32_t)0x1040C, (char32_t)0x1040D, (char32_t)0x1040E, (char32_t)0x1040F,
    (char32_t)0x10410, (char32_t)0x10411, (char32_t)0x10412, (char32_t)0x10413, (char32_t)0x10414,
    (char32_t)0x10415, (char32_t)0x10416, (char32_t)0x10417, (char32_t)0x10418, (char32_t)0x10419,
    (char32_t)0x1041A, (char32_t)0x1041B, (char32_t)0x1041C, (char32_t)0x1041D, (char32_t)0x1041E,
    (char32_t)0x1041F, (char32_t)0x10420, (char32_t)0x10421, (char32_t)0x10422, (char32_t)0x10423,
    (char32_t)0x10424, (char32_t)0x10425, (char32_t)0x10426, (char32_t)0x10427, (char32_t)0x104B0,
    (char32_t)0x104B1, (char32_t)0x104B2, (char32_t)0x104B3, (char32_t)0x104B4, (char32_t)0x104B5,
    (char32_t)0x104B6, (char32_t)0x104B7, (char32_t)0x104B8, (char32_t)0x104B9, (char32_t)0x104BA,
    (char32_t)0x104BB, (char32_t)0x104BC, (char32_t)0x104BD, (char32_t)0x104BE, (char32_t)0x104BF,
    (char32_t)0x104C0, (char32_t)0x104C1, (char32_t)0x104C2, (char32_t)0x104C3, (char32_t)0x104C4,
    (char32_t)0x104C5, (char32_t)0x104C6, (char32_t)0x104C7, (char32_t)0x104C8, (char32_t)0x104C9,
    (char32_t)0x104CA, (char32_t)0x104CB, (char32_t)0x104CC, (char32_t)0x104CD, (char32_t)0x104CE,
    (char32_t)0x104CF, (char32_t)0x104D0, (char32_t)0x104D1, (char32_t)0x104D2, (char32_t)0x104D3,
    (char32_t)0x10570, (char32_t)0x10571, (char32_t)0x10572, (char32_t)0x10573, (char32_t)0x10574,
    (char32_t)0x10575, (char32_t)0x10576, (char32_t)0x10577, (char32_t)0x10578, (char32_t)0x10579,
    (char32_t)0x1057A, (char32_t)0x1057C, (char32_t)0x1057D, (char32_t)0x1057E, (char32_t)0x1057F,
    (char32_t)0x10580, (char32_t)0x10581, (char32_t)0x10582, (char32_t)0x10583, (char32_t)0x10584,
    (char32_t)0x10585, (char32_t)0x10586, (char32_t)0x10587, (char32_t)0x10588, (char32_t)0x10589,
    (char32_t)0x1058A, (char32_t)0x1058C, (char32_t)0x1058D, (char32_t)0x1058E, (char32_t)0x1058F,
    (char32_t)0x10590, (char32_t)0x10591, (char32_t)0x10592, (char32_t)0x10594, (char32_t)0x10595,
    (char32_t)0x10C80, (char32_t)0x10C81, (char32_t)0x10C82, (char32_t)0x10C83, (char32_t)0x10C84,
    (char32_t)0x10C85, (char32_t)0x10C86, (char32_t)0x10C87, (char32_t)0x10C88, (char32_t)0x10C89,
    (char32_t)0x10C8A, (char32_t)0x10C8B, (char32_t)0x10C8C, (char32_t)0x10C8D, (char32_t)0x10C8E,
    (char32_t)0x10C8F, (char32_t)0x10C90, (char32_t)0x10C91, (char32_t)0x10C92, (char32_t)0x10C93,
    (char32_t)0x10C94, (char32_t)0x10C95, (char32_t)0x10C96, (char32_t)0x10C97, (char32_t)0x10C98,
    (char32_t)0x10C99, (char32_t)0x10C9A, (char32_t)0x10C9B, (char32_t)0x10C9C, (char32_t)0x10C9D,
    (char32_t)0x10C9E, (char32_t)0x10C9F, (char32_t)0x10CA0, (char32_t)0x10CA1, (char32_t)0x10CA2,
    (char32_t)0x10CA3, (char32_t)0x10CA4, (char32_t)0x10CA5, (char32_t)0x10CA6, (char32_t)0x10CA7,
    (char32_t)0x10CA8, (char32_t)0x10CA9, (char32_t)0x10CAA, (char32_t)0x10CAB, (char32_t)0x10CAC,
    (char32_t)0x10CAD, (char32_t)0x10CAE, (char32_t)0x10CAF, (char32_t)0x10CB0, (char32_t)0x10CB1,
    (char32_t)0x10CB2, (char32_t)0x118A0, (char32_t)0x118A1, (char32_t)0x118A2, (char32_t)0x118A3,
    (char32_t)0x118A4, (char32_t)0x118A5, (char32_t)0x118A6, (char32_t)0x118A7, (char32_t)0x118A8,
    (char32_t)0x118A9, (char32_t)0x118AA, (char32_t)0x118AB, (char32_t)0x118AC, (char32_t)0x118AD,
    (char32_t)0x118AE, (char32_t)0x118AF, (char32_t)0x118B0, (char32_t)0x118B1, (char32_t)0x118B2,
    (char32_t)0x118B3, (char32_t)0x118B4, (char32_t)0x118B5, (char32_t)0x118B6, (char32_t)0x118B7,
    (char32_t)0x118B8, (char32_t)0x118B9, (char32_t)0x118BA, (char32_t)0x118BB, (char32_t)0x118BC,
    (char32_t)0x118BD, (char32_t)0x118BE, (char32_t)0x118BF, (char32_t)0x16E40, (char32_t)0x16E41,
    (char32_t)0x16E42, (char32_t)0x16E43, (char32_t)0x16E44, (char32_t)0x16E45, (char32_t)0x16E46,
    (char32_t)0x16E47, (char32_t)0x16E48, (char32_t)0x16E49, (char32_t)0x16E4A, (char32_t)0x16E4B,
    (char32_t)0x16E4C, (char32_t)0x16E4D, (char32_t)0x16E4E, (char32_t)0x16E4F, (char32_t)0x16E50,
    (char32_t)0x16E51, (char32_t)0x16E52, (char32_t)0x16E53, (char32_t)0x16E54, (char32_t)0x16E55,
    (char32_t)0x16E56, (char32_t)0x16E57, (char32_t)0x16E58, (char32_t)0x16E59, (char32_t)0x16E5A,
    (char32_t)0x16E5B, (char32_t)0x16E5C, (char32_t)0x16E5D, (char32_t)0x16E5E, (char32_t)0x16E5F,
    (char32_t)0x1E900, (char32_t)0x1E901, (char32_t)0x1E902, (char32_t)0x1E903, (char32_t)0x1E904,
    (char32_t)0x1E905, (char32_t)0x1E906, (char32_t)0x1E907, (char32_t)0x1E908, (char32_t)0x1E909,
    (char32_t)0x1E90A, (char32_t)0x1E90B, (char32_t)0x1E90C, (char32_t)0x1E90D, (char32_t)0x1E90E,
    (char32_t)0x1E90F, (char32_t)0x1E910, (char32_t)0x1E911, (char32_t)0x1E912, (char32_t)0x1E913,
    (char32_t)0x1E914, (char32_t)0x1E915, (char32_t)0x1E916, (char32_t)0x1E917, (char32_t)0x1E918,
    (char32_t)0x1E919, (char32_t)0x1E91A, (char32_t)0x1E91B, (char32_t)0x1E91C, (char32_t)0x1E91D,
    (char32_t)0x1E91E, (char32_t)0x1E91F, (char32_t)0x1E920, (char32_t)0x1E921};

static const char32_t unicode_fold_lower[] = {
    (char32_t)0x0061,  (char32_t)0x0062,  (char32_t)0x0063,  (char32_t)0x0064,  (char32_t)0x0065,
    (char32_t)0x0066,  (char32_t)0x0067,  (char32_t)0x0068,  (char32_t)0x0069,  (char32_t)0x006A,
    (char32_t)0x006B,  (char32_t)0x006C,  (char32_t)0x006D,  (char32_t)0x006E,  (char32_t)0x006F,
    (char32_t)0x0070,  (char32_t)0x0071,  (char32_t)0x0072,  (char32_t)0x0073,  (char32_t)0x0074,
    (char32_t)0x0075,  (char32_t)0x0076,  (char32_t)0x0077,  (char32_t)0x0078,  (char32_t)0x0079,
    (char32_t)0x007A,  (char32_t)0x03BC,  (char32_t)0x00E0,  (char32_t)0x00E1,  (char32_t)0x00E2,
    (char32_t)0x00E3,  (char32_t)0x00E4,  (char32_t)0x00E5,  (char32_t)0x00E6,  (char32_t)0x00E7,
    (char32_t)0x00E8,  (char32_t)0x00E9,  (char32_t)0x00EA,  (char32_t)0x00EB,  (char32_t)0x00EC,
    (char32_t)0x00ED,  (char32_t)0x00EE,  (char32_t)0x00EF,  (char32_t)0x00F0,  (char32_t)0x00F1,
    (char32_t)0x00F2,  (char32_t)0x00F3,  (char32_t)0x00F4,  (char32_t)0x00F5,  (char32_t)0x00F6,
    (char32_t)0x00F8,  (char32_t)0x00F9,  (char32_t)0x00FA,  (char32_t)0x00FB,  (char32_t)0x00FC,
    (char32_t)0x00FD,  (char32_t)0x00FE,  (char32_t)0x0101,  (char32_t)0x0103,  (char32_t)0x0105,
    (char32_t)0x0107,  (char32_t)0x0109,  (char32_t)0x010B,  (char32_t)0x010D,  (char32_t)0x010F,
    (char32_t)0x0111,  (char32_t)0x0113,  (char32_t)0x0115,  (char32_t)0x0117,  (char32_t)0x0119,
    (char32_t)0x011B,  (char32_t)0x011D,  (char32_t)0x011F,  (char32_t)0x0121,  (char32_t)0x0123,
    (char32_t)0x0125,  (char32_t)0x0127,  (char32_t)0x0129,  (char32_t)0x012B,  (char32_t)0x012D,
    (char32_t)0x012F,  (char32_t)0x0133,  (char32_t)0x0135,  (char32_t)0x0137,  (char32_t)0x013A,
    (char32_t)0x013C,  (char32_t)0x013E,  (char32_t)0x0140,  (char32_t)0x0142,  (char32_t)0x0144,
    (char32_t)0x0146,  (char32_t)0x0148,  (char32_t)0x014B,  (char32_t)0x014D,  (char32_t)0x014F,
    (char32_t)0x0151,  (char32_t)0x0153,  (char32_t)0x0155,  (char32_t)0x0157,  (char32_t)0x0159,
    (char32_t)0x015B,  (char32_t)0x015D,  (char32_t)0x015F,  (char32_t)0x0161,  (char32_t)0x0163,
    (char32_t)0x0165,  (char32_t)0x0167,  (char32_t)0x0169,  (char32_t)0x016B,  (char32_t)0x016D,
    (char32_t)0x016F,  (char32_t)0x0171,  (char32_t)0x0173,  (char32_t)0x0175,  (char32_t)0x0177,
    (char32_t)0x00FF,  (char32_t)0x017A,  (char32_t)0x017C,  (char32_t)0x017E,  (char32_t)0x0073,
    (char32_t)0x0253,  (char32_t)0x0183,  (char32_t)0x0185,  (char32_t)0x0254,  (char32_t)0x0188,
    (char32_t)0x0256,  (char32_t)0x0257,  (char32_t)0x018C,  (char32_t)0x01DD,  (char32_t)0x0259,
    (char32_t)0x025B,  (char32_t)0x0192,  (char32_t)0x0260,  (char32_t)0x0263,  (char32_t)0x0269,
    (char32_t)0x0268,  (char32_t)0x0199,  (char32_t)0x026F,  (char32_t)0x0272,  (char32_t)0x0275,
    (char32_t)0x01A1,  (char32_t)0x01A3,  (char32_t)0x01A5,  (char32_t)0x0280,  (char32_t)0x01A8,
    (char32_t)0x0283,  (char32_t)0x01AD,  (char32_t)0x0288,  (char32_t)0x01B0,  (char32_t)0x028A,
    (char32_t)0x028B,  (char32_t)0x01B4,  (char32_t)0x01B6,  (char32_t)0x0292,  (char32_t)0x01B9,
    (char32_t)0x01BD,  (char32_t)0x01C6,  (char32_t)0x01C6,  (char32_t)0x01C9,  (char32_t)0x01C9,
    (char32_t)0x01CC,  (char32_t)0x01CC,  (char32_t)0x01CE,  (char32_t)0x01D0,  (char32_t)0x01D2,
    (char32_t)0x01D4,  (char32_t)0x01D6,  (char32_t)0x01D8,  (char32_t)0x01DA,  (char32_t)0x01DC,
    (char32_t)0x01DF,  (char32_t)0x01E1,  (char32_t)0x01E3,  (char32_t)0x01E5,  (char32_t)0x01E7,
    (char32_t)0x01E9,  (char32_t)0x01EB,  (char32_t)0x01ED,  (char32_t)0x01EF,  (char32_t)0x01F3,
    (char32_t)0x01F3,  (char32_t)0x01F5,  (char32_t)0x0195,  (char32_t)0x01BF,  (char32_t)0x01F9,
    (char32_t)0x01FB,  (char32_t)0x01FD,  (char32_t)0x01FF,  (char32_t)0x0201,  (char32_t)0x0203,
    (char32_t)0x0205,  (char32_t)0x0207,  (char32_t)0x0209,  (char32_t)0x020B,  (char32_t)0x020D,
    (char32_t)0x020F,  (char32_t)0x0211,  (char32_t)0x0213,  (char32_t)0x0215,  (char32_t)0x0217,
    (char32_t)0x0219,  (char32_t)0x021B,  (char32_t)0x021D,  (char32_t)0x021F,  (char32_t)0x019E,
    (char32_t)0x0223,  (char32_t)0x0225,  (char32_t)0x0227,  (char32_t)0x0229,  (char32_t)0x022B,
    (char32_t)0x022D,  (char32_t)0x022F,  (char32_t)0x0231,  (char32_t)0x0233,  (char32_t)0x2C65,
    (char32_t)0x023C,  (char32_t)0x019A,  (char32_t)0x2C66,  (char32_t)0x0242,  (char32_t)0x0180,
    (char32_t)0x0289,  (char32_t)0x028C,  (char32_t)0x0247,  (char32_t)0x0249,  (char32_t)0x024B,
    (char32_t)0x024D,  (char32_t)0x024F,  (char32_t)0x03B9,  (char32_t)0x0371,  (char32_t)0x0373,
    (char32_t)0x0377,  (char32_t)0x03F3,  (char32_t)0x03AC,  (char32_t)0x03AD,  (char32_t)0x03AE,
    (char32_t)0x03AF,  (char32_t)0x03CC,  (char32_t)0x03CD,  (char32_t)0x03CE,  (char32_t)0x03B1,
    (char32_t)0x03B2,  (char32_t)0x03B3,  (char32_t)0x03B4,  (char32_t)0x03B5,  (char32_t)0x03B6,
    (char32_t)0x03B7,  (char32_t)0x03B8,  (char32_t)0x03B9,  (char32_t)0x03BA,  (char32_t)0x03BB,
    (char32_t)0x03BC,  (char32_t)0x03BD,  (char32_t)0x03BE,  (char32_t)0x03BF,  (char32_t)0x03C0,
    (char32_t)0x03C1,  (char32_t)0x03C3,  (char32_t)0x03C4,  (char32_t)0x03C5,  (char32_t)0x03C6,
    (char32_t)0x03C7,  (char32_t)0x03C8,  (char32_t)0x03C9,  (char32_t)0x03CA,  (char32_t)0x03CB,
    (char32_t)0x03C3,  (char32_t)0x03D7,  (char32_t)0x03B2,  (char32_t)0x03B8,  (char32_t)0x03C6,
    (char32_t)0x03C0,  (char32_t)0x03D9,  (char32_t)0x03DB,  (char32_t)0x03DD,  (char32_t)0x03DF,
    (char32_t)0x03E1,  (char32_t)0x03E3,  (char32_t)0x03E5,  (char32_t)0x03E7,  (char32_t)0x03E9,
    (char32_t)0x03EB,  (char32_t)0x03ED,  (char32_t)0x03EF,  (char32_t)0x03BA,  (char32_t)0x03C1,
    (char32_t)0x03B8,  (char32_t)0x03B5,  (char32_t)0x03F8,  (char32_t)0x03F2,  (char32_t)0x03FB,
    (char32_t)0x037B,  (char32_t)0x037C,  (char32_t)0x037D,  (char32_t)0x0450,  (char32_t)0x0451,
    (char32_t)0x0452,  (char32_t)0x0453,  (char32_t)0x0454,  (char32_t)0x0455,  (char32_t)0x0456,
    (char32_t)0x0457,  (char32_t)0x0458,  (char32_t)0x0459,  (char32_t)0x045A,  (char32_t)0x045B,
    (char32_t)0x045C,  (char32_t)0x045D,  (char32_t)0x045E,  (char32_t)0x045F,  (char32_t)0x0430,
    (char32_t)0x0431,  (char32_t)0x0432,  (char32_t)0x0433,  (char32_t)0x0434,  (char32_t)0x0435,
    (char32_t)0x0436,  (char32_t)0x0437,  (char32_t)0x0438,  (char32_t)0x0439,  (char32_t)0x043A,
    (char32_t)0x043B,  (char32_t)0x043C,  (char32_t)0x043D,  (char32_t)0x043E,  (char32_t)0x043F,
    (char32_t)0x0440,  (char32_t)0x0441,  (char32_t)0x0442,  (char32_t)0x0443,  (char32_t)0x0444,
    (char32_t)0x0445,  (char32_t)0x0446,  (char32_t)0x0447,  (char32_t)0x0448,  (char32_t)0x0449,
    (char32_t)0x044A,  (char32_t)0x044B,  (char32_t)0x044C,  (char32_t)0x044D,  (char32_t)0x044E,
    (char32_t)0x044F,  (char32_t)0x0461,  (char32_t)0x0463,  (char32_t)0x0465,  (char32_t)0x0467,
    (char32_t)0x0469,  (char32_t)0x046B,  (char32_t)0x046D,  (char32_t)0x046F,  (char32_t)0x0471,
    (char32_t)0x0473,  (char32_t)0x0475,  (char32_t)0x0477,  (char32_t)0x0479,  (char32_t)0x047B,
    (char32_t)0x047D,  (char32_t)0x047F,  (char32_t)0x0481,  (char32_t)0x048B,  (char32_t)0x048D,
    (char32_t)0x048F,  (char32_t)0x0491,  (char32_t)0x0493,  (char32_t)0x0495,  (char32_t)0x0497,
    (char32_t)0x0499,  (char32_t)0x049B,  (char32_t)0x049D,  (char32_t)0x049F,  (char32_t)0x04A1,
    (char32_t)0x04A3,  (char32_t)0x04A5,  (char32_t)0x04A7,  (char32_t)0x04A9,  (char32_t)0x04AB,
    (char32_t)0x04AD,  (char32_t)0x04AF,  (char32_t)0x04B1,  (char32_t)0x04B3,  (char32_t)0x04B5,
    (char32_t)0x04B7,  (char32_t)0x04B9,  (char32_t)0x04BB,  (char32_t)0x04BD,  (char32_t)0x04BF,
    (char32_t)0x04CF,  (char32_t)0x04C2,  (char32_t)0x04C4,  (char32_t)0x04C6,  (char32_t)0x04C8,
    (char32_t)0x04CA,  (char32_t)0x04CC,  (char32_t)0x04CE,  (char32_t)0x04D1,  (char32_t)0x04D3,
    (char32_t)0x04D5,  (char32_t)0x04D7,  (char32_t)0x04D9,  (char32_t)0x04DB,  (char32_t)0x04DD,
    (char32_t)0x04DF,  (char32_t)0x04E1,  (char32_t)0x04E3,  (char32_t)0x04E5,  (char32_t)0x04E7,
    (char32_t)0x04E9,  (char32_t)0x04EB,  (char32_t)0x04ED,  (char32_t)0x04EF,  (char32_t)0x04F1,
    (char32_t)0x04F3,  (char32_t)0x04F5,  (char32_t)0x04F7,  (char32_t)0x04F9,  (char32_t)0x04FB,
    (char32_t)0x04FD,  (char32_t)0x04FF,  (char32_t)0x0501,  (char32_t)0x0503,  (char32_t)0x0505,
    (char32_t)0x0507,  (char32_t)0x0509,  (char32_t)0x050B,  (char32_t)0x050D,  (char32_t)0x050F,
    (char32_t)0x0511,  (char32_t)0x0513,  (char32_t)0x0515,  (char32_t)0x0517,  (char32_t)0x0519,
    (char32_t)0x051B,  (char32_t)0x051D,  (char32_t)0x051F,  (char32_t)0x0521,  (char32_t)0x0523,
    (char32_t)0x0525,  (char32_t)0x0527,  (char32_t)0x0529,  (char32_t)0x052B,  (char32_t)0x052D,
    (char32_t)0x052F,  (char32_t)0x0561,  (char32_t)0x0562,  (char32_t)0x0563,  (char32_t)0x0564,
    (char32_t)0x0565,  (char32_t)0x0566,  (char32_t)0x0567,  (char32_t)0x0568,  (char32_t)0x0569,
    (char32_t)0x056A,  (char32_t)0x056B,  (char32_t)0x056C,  (char32_t)0x056D,  (char32_t)0x056E,
    (char32_t)0x056F,  (char32_t)0x0570,  (char32_t)0x0571,  (char32_t)0x0572,  (char32_t)0x0573,
    (char32_t)0x0574,  (char32_t)0x0575,  (char32_t)0x0576,  (char32_t)0x0577,  (char32_t)0x0578,
    (char32_t)0x0579,  (char32_t)0x057A,  (char32_t)0x057B,  (char32_t)0x057C,  (char32_t)0x057D,
    (char32_t)0x057E,  (char32_t)0x057F,  (char32_t)0x0580,  (char32_t)0x0581,  (char32_t)0x0582,
    (char32_t)0x0583,  (char32_t)0x0584,  (char32_t)0x0585,  (char32_t)0x0586,  (char32_t)0x2D00,
    (char32_t)0x2D01,  (char32_t)0x2D02,  (char32_t)0x2D03,  (char32_t)0x2D04,  (char32_t)0x2D05,
    (char32_t)0x2D06,  (char32_t)0x2D07,  (char32_t)0x2D08,  (char32_t)0x2D09,  (char32_t)0x2D0A,
    (char32_t)0x2D0B,  (char32_t)0x2D0C,  (char32_t)0x2D0D,  (char32_t)0x2D0E,  (char32_t)0x2D0F,
    (char32_t)0x2D10,  (char32_t)0x2D11,  (char32_t)0x2D12,  (char32_t)0x2D13,  (char32_t)0x2D14,
    (char32_t)0x2D15,  (char32_t)0x2D16,  (char32_t)0x2D17,  (char32_t)0x2D18,  (char32_t)0x2D19,
    (char32_t)0x2D1A,  (char32_t)0x2D1B,  (char32_t)0x2D1C,  (char32_t)0x2D1D,  (char32_t)0x2D1E,
    (char32_t)0x2D1F,  (char32_t)0x2D20,  (char32_t)0x2D21,  (char32_t)0x2D22,  (char32_t)0x2D23,
    (char32_t)0x2D24,  (char32_t)0x2D25,  (char32_t)0x2D27,  (char32_t)0x2D2D,  (char32_t)0x13F0,
    (char32_t)0x13F1,  (char32_t)0x13F2,  (char32_t)0x13F3,  (char32_t)0x13F4,  (char32_t)0x13F5,
    (char32_t)0x0432,  (char32_t)0x0434,  (char32_t)0x043E,  (char32_t)0x0441,  (char32_t)0x0442,
    (char32_t)0x0442,  (char32_t)0x044A,  (char32_t)0x0463,  (char32_t)0xA64B,  (char32_t)0x10D0,
    (char32_t)0x10D1,  (char32_t)0x10D2,  (char32_t)0x10D3,  (char32_t)0x10D4,  (char32_t)0x10D5,
    (char32_t)0x10D6,  (char32_t)0x10D7,  (char32_t)0x10D8,  (char32_t)0x10D9,  (char32_t)0x10DA,
    (char32_t)0x10DB,  (char32_t)0x10DC,  (char32_t)0x10DD,  (char32_t)0x10DE,  (char32_t)0x10DF,
    (char32_t)0x10E0,  (char32_t)0x10E1,  (char32_t)0x10E2,  (char32_t)0x10E3,  (char32_t)0x10E4,
    (char32_t)0x10E5,  (char32_t)0x10E6,  (char32_t)0x10E7,  (char32_t)0x10E8,  (char32_t)0x10E9,
    (char32_t)0x10EA,  (char32_t)0x10EB,  (char32_t)0x10EC,  (char32_t)0x10ED,  (char32_t)0x10EE,
    (char32_t)0x10EF,  (char32_t)0x10F0,  (char32_t)0x10F1,  (char32_t)0x10F2,  (char32_t)0x10F3,
    (char32_t)0x10F4,  (char32_t)0x10F5,  (char32_t)0x10F6,  (char32_t)0x10F7,  (char32_t)0x10F8,
    (char32_t)0x10F9,  (char32_t)0x10FA,  (char32_t)0x10FD,  (char32_t)0x10FE,  (char32_t)0x10FF,
    (char32_t)0x1E01,  (char32_t)0x1E03,  (char32_t)0x1E05,  (char32_t)0x1E07,  (char32_t)0x1E09,
    (char32_t)0x1E0B,  (char32_t)0x1E0D,  (char32_t)0x1E0F,  (char32_t)0x1E11,  (char32_t)0x1E13,
    (char32_t)0x1E15,  (char32_t)0x1E17,  (char32_t)0x1E19,  (char32_t)0x1E1B,  (char32_t)0x1E1D,
    (char32_t)0x1E1F,  (char32_t)0x1E21,  (char32_t)0x1E23,  (char32_t)0x1E25,  (char32_t)0x1E27,
    (char32_t)0x1E29,  (char32_t)0x1E2B,  (char32_t)0x1E2D,  (char32_t)0x1E2F,  (char32_t)0x1E31,
    (char32_t)0x1E33,  (char32_t)0x1E35,  (char32_t)0x1E37,  (char32_t)0x1E39,  (char32_t)0x1E3B,
    (char32_t)0x1E3D,  (char32_t)0x1E3F,  (char32_t)0x1E41,  (char32_t)0x1E43,  (char32_t)0x1E45,
    (char32_t)0x1E47,  (char32_t)0x1E49,  (char32_t)0x1E4B,  (char32_t)0x1E4D,  (char32_t)0x1E4F,
    (char32_t)0x1E51,  (char32_t)0x1E53,  (char32_t)0x1E55,  (char32_t)0x1E57,  (char32_t)0x1E59,
    (char32_t)0x1E5B,  (char32_t)0x1E5D,  (char32_t)0x1E5F,  (char32_t)0x1E61,  (char32_t)0x1E63,
    (char32_t)0x1E65,  (char32_t)0x1E67,  (char32_t)0x1E69,  (char32_t)0x1E6B,  (char32_t)0x1E6D,
    (char32_t)0x1E6F,  (char32_t)0x1E71,  (char32_t)0x1E73,  (char32_t)0x1E75,  (char32_t)0x1E77,
    (char32_t)0x1E79,  (char32_t)0x1E7B,  (char32_t)0x1E7D,  (char32_t)0x1E7F,  (char32_t)0x1E81,
    (char32_t)0x1E83,  (char32_t)0x1E85,  (char32_t)0x1E87,  (char32_t)0x1E89,  (char32_t)0x1E8B,
    (char32_t)0x1E8D,  (char32_t)0x1E8F,  (char32_t)0x1E91,  (char32_t)0x1E93,  (char32_t)0x1E95,
    (char32_t)0x1E61,  (char32_t)0x00DF,  (char32_t)0x1EA1,  (char32_t)0x1EA3,  (char32_t)0x1EA5,
    (char32_t)0x1EA7,  (char32_t)0x1EA9,  (char32_t)0x1EAB,  (char32_t)0x1EAD,  (char32_t)0x1EAF,
    (char32_t)0x1EB1,  (char32_t)0x1EB3,  (char32_t)0x1EB5,  (char32_t)0x1EB7,  (char32_t)0x1EB9,
    (char32_t)0x1EBB,  (char32_t)0x1EBD,  (char32_t)0x1EBF,  (char32_t)0x1EC1,  (char32_t)0x1EC3,
    (char32_t)0x1EC5,  (char32_t)0x1EC7,  (char32_t)0x1EC9,  (char32_t)0x1ECB,  (char32_t)0x1ECD,
    (char32_t)0x1ECF,  (char32_t)0x1ED1,  (char32_t)0x1ED3,  (char32_t)0x1ED5,  (char32_t)0x1ED7,
    (char32_t)0x1ED9,  (char32_t)0x1EDB,  (char32_t)0x1EDD,  (char32_t)0x1EDF,  (char32_t)0x1EE1,
    (char32_t)0x1EE3,  (char32_t)0x1EE5,  (char32_t)0x1EE7,  (char32_t)0x1EE9,  (char32_t)0x1EEB,
    (char32_t)0x1EED,  (char32_t)0x1EEF,  (char32_t)0x1EF1,  (char32_t)0x1EF3,  (char32_t)0x1EF5,
    (char32_t)0x1EF7,  (char32_t)0x1EF9,  (char32_t)0x1EFB,  (char32_t)0x1EFD,  (char32_t)0x1EFF,
    (char32_t)0x1F00,  (char32_t)0x1F01,  (char32_t)0x1F02,  (char32_t)0x1F03,  (char32_t)0x1F04,
    (char32_t)0x1F05,  (char32_t)0x1F06,  (char32_t)0x1F07,  (char32_t)0x1F10,  (char32_t)0x1F11,
    (char32_t)0x1F12,  (char32_t)0x1F13,  (char32_t)0x1F14,  (char32_t)0x1F15,  (char32_t)0x1F20,
    (char32_t)0x1F21,  (char32_t)0x1F22,  (char32_t)0x1F23,  (char32_t)0x1F24,  (char32_t)0x1F25,
    (char32_t)0x1F26,  (char32_t)0x1F27,  (char32_t)0x1F30,  (char32_t)0x1F31,  (char32_t)0x1F32,
    (char32_t)0x1F33,  (char32_t)0x1F34,  (char32_t)0x1F35,  (char32_t)0x1F36,  (char32_t)0x1F37,
    (char32_t)0x1F40,  (char32_t)0x1F41,  (char32_t)0x1F42,  (char32_t)0x1F43,  (char32_t)0x1F44,
    (char32_t)0x1F45,  (char32_t)0x1F51,  (char32_t)0x1F53,  (char32_t)0x1F55,  (char32_t)0x1F57,
    (char32_t)0x1F60,  (char32_t)0x1F61,  (char32_t)0x1F62,  (char32_t)0x1F63,  (char32_t)0x1F64,
    (char32_t)0x1F65,  (char32_t)0x1F66,  (char32_t)0x1F67,  (char32_t)0x1F80,  (char32_t)0x1F81,
    (char32_t)0x1F82,  (char32_t)0x1F83,  (char32_t)0x1F84,  (char32_t)0x1F85,  (char32_t)0x1F86,
    (char32_t)0x1F87,  (char32_t)0x1F90,  (char32_t)0x1F91,  (char32_t)0x1F92,  (char32_t)0x1F93,
    (char32_t)0x1F94,  (char32_t)0x1F95,  (char32_t)0x1F96,  (char32_t)0x1F97,  (char32_t)0x1FA0,
    (char32_t)0x1FA1,  (char32_t)0x1FA2,  (char32_t)0x1FA3,  (char32_t)0x1FA4,  (char32_t)0x1FA5,
    (char32_t)0x1FA6,  (char32_t)0x1FA7,  (char32_t)0x1FB0,  (char32_t)0x1FB1,  (char32_t)0x1F70,
    (char32_t)0x1F71,  (char32_t)0x1FB3,  (char32_t)0x03B9,  (char32_t)0x1F72,  (char32_t)0x1F73,
    (char32_t)0x1F74,  (char32_t)0x1F75,  (char32_t)0x1FC3,  (char32_t)0x1FD0,  (char32_t)0x1FD1,
    (char32_t)0x1F76,  (char32_t)0x1F77,  (char32_t)0x1FE0,  (char32_t)0x1FE1,  (char32_t)0x1F7A,
    (char32_t)0x1F7B,  (char32_t)0x1FE5,  (char32_t)0x1F78,  (char32_t)0x1F79,  (char32_t)0x1F7C,
    (char32_t)0x1F7D,  (char32_t)0x1FF3,  (char32_t)0x03C9,  (char32_t)0x006B,  (char32_t)0x00E5,
    (char32_t)0x214E,  (char32_t)0x2170,  (char32_t)0x2171,  (char32_t)0x2172,  (char32_t)0x2173,
    (char32_t)0x2174,  (char32_t)0x2175,  (char32_t)0x2176,  (char32_t)0x2177,  (char32_t)0x2178,
    (char32_t)0x2179,  (char32_t)0x217A,  (char32_t)0x217B,  (char32_t)0x217C,  (char32_t)0x217D,
    (char32_t)0x217E,  (char32_t)0x217F,  (char32_t)0x2184,  (char32_t)0x24D0,  (char32_t)0x24D1,
    (char32_t)0x24D2,  (char32_t)0x24D3,  (char32_t)0x24D4,  (char32_t)0x24D5,  (char32_t)0x24D6,
    (char32_t)0x24D7,  (char32_t)0x24D8,  (char32_t)0x24D9,  (char32_t)0x24DA,  (char32_t)0x24DB,
    (char32_t)0x24DC,  (char32_t)0x24DD,  (char32_t)0x24DE,  (char32_t)0x24DF,  (char32_t)0x24E0,
    (char32_t)0x24E1,  (char32_t)0x24E2,  (char32_t)0x24E3,  (char32_t)0x24E4,  (char32_t)0x24E5,
    (char32_t)0x24E6,  (char32_t)0x24E7,  (char32_t)0x24E8,  (char32_t)0x24E9,  (char32_t)0x2C30,
    (char32_t)0x2C31,  (char32_t)0x2C32,  (char32_t)0x2C33,  (char32_t)0x2C34,  (char32_t)0x2C35,
    (char32_t)0x2C36,  (char32_t)0x2C37,  (char32_t)0x2C38,  (char32_t)0x2C39,  (char32_t)0x2C3A,
    (char32_t)0x2C3B,  (char32_t)0x2C3C,  (char32_t)0x2C3D,  (char32_t)0x2C3E,  (char32_t)0x2C3F,
    (char32_t)0x2C40,  (char32_t)0x2C41,  (char32_t)0x2C42,  (char32_t)0x2C43,  (char32_t)0x2C44,
    (char32_t)0x2C45,  (char32_t)0x2C46,  (char32_t)0x2C47,  (char32_t)0x2C48,  (char32_t)0x2C49,
    (char32_t)0x2C4A,  (char32_t)0x2C4B,  (char32_t)0x2C4C,  (char32_t)0x2C4D,  (char32_t)0x2C4E,
    (char32_t)0x2C4F,  (char32_t)0x2C50,  (char32_t)0x2C51,  (char32_t)0x2C52,  (char32_t)0x2C53,
    (char32_t)0x2C54,  (char32_t)0x2C55,  (char32_t)0x2C56,  (char32_t)0x2C57,  (char32_t)0x2C58,
    (char32_t)0x2C59,  (char32_t)0x2C5A,  (char32_t)0x2C5B,  (char32_t)0x2C5C,  (char32_t)0x2C5D,
    (char32_t)0x2C5E,  (char32_t)0x2C5F,  (char32_t)0x2C61,  (char32_t)0x026B,  (char32_t)0x1D7D,
    (char32_t)0x027D,  (char32_t)0x2C68,  (char32_t)0x2C6A,  (char32_t)0x2C6C,  (char32_t)0x0251,
    (char32_t)0x0271,  (char32_t)0x0250,  (char32_t)0x0252,  (char32_t)0x2C73,  (char32_t)0x2C76,
    (char32_t)0x023F,  (char32_t)0x0240,  (char32_t)0x2C81,  (char32_t)0x2C83,  (char32_t)0x2C85,
    (char32_t)0x2C87,  (char32_t)0x2C89,  (char32_t)0x2C8B,  (char32_t)0x2C8D,  (char32_t)0x2C8F,
    (char32_t)0x2C91,  (char32_t)0x2C93,  (char32_t)0x2C95,  (char32_t)0x2C97,  (char32_t)0x2C99,
    (char32_t)0x2C9B,  (char32_t)0x2C9D,  (char32_t)0x2C9F,  (char32_t)0x2CA1,  (char32_t)0x2CA3,
    (char32_t)0x2CA5,  (char32_t)0x2CA7,  (char32_t)0x2CA9,  (char32_t)0x2CAB,  (char32_t)0x2CAD,
    (char32_t)0x2CAF,  (char32_t)0x2CB1,  (char32_t)0x2CB3,  (char32_t)0x2CB5,  (char32_t)0x2CB7,
    (char32_t)0x2CB9,  (char32_t)0x2CBB,  (char32_t)0x2CBD,  (char32_t)0x2CBF,  (char32_t)0x2CC1,
    (char32_t)0x2CC3,  (char32_t)0x2CC5,  (char32_t)0x2CC7,  (char32_t)0x2CC9,  (char32_t)0x2CCB,
    (char32_t)0x2CCD,  (char32_t)0x2CCF,  (char32_t)0x2CD1,  (char32_t)0x2CD3,  (char32_t)0x2CD5,
    (char32_t)0x2CD7,  (char32_t)0x2CD9,  (char32_t)0x2CDB,  (char32_t)0x2CDD,  (char32_t)0x2CDF,
    (char32_t)0x2CE1,  (char32_t)0x2CE3,  (char32_t)0x2CEC,  (char32_t)0x2CEE,  (char32_t)0x2CF3,
    (char32_t)0xA641,  (char32_t)0xA643,  (char32_t)0xA645,  (char32_t)0xA647,  (char32_t)0xA649,
    (char32_t)0xA64B,  (char32_t)0xA64D,  (char32_t)0xA64F,  (char32_t)0xA651,  (char32_t)0xA653,
    (char32_t)0xA655,  (char32_t)0xA657,  (char32_t)0xA659,  (char32_t)0xA65B,  (char32_t)0xA65D,
    (char32_t)0xA65F,  (char32_t)0xA661,  (char32_t)0xA663,  (char32_t)0xA665,  (char32_t)0xA667,
    (char32_t)0xA669,  (char32_t)0xA66B,  (char32_t)0xA66D,  (char32_t)0xA681,  (char32_t)0xA683,
    (char32_t)0xA685,  (char32_t)0xA687,  (char32_t)0xA689,  (char32_t)0xA68B,  (char32_t)0xA68D,
    (char32_t)0xA68F,  (char32_t)0xA691,  (char32_t)0xA693,  (char32_t)0xA695,  (char32_t)0xA697,
    (char32_t)0xA699,  (char32_t)0xA69B,  (char32_t)0xA723,  (char32_t)0xA725,  (char32_t)0xA727,
    (char32_t)0xA729,  (char32_t)0xA72B,  (char32_t)0xA72D,  (char32_t)0xA72F,  (char32_t)0xA733,
    (char32_t)0xA735,  (char32_t)0xA737,  (char32_t)0xA739,  (char32_t)0xA73B,  (char32_t)0xA73D,
    (char32_t)0xA73F,  (char32_t)0xA741,  (char32_t)0xA743,  (char32_t)0xA745,  (char32_t)0xA747,
    (char32_t)0xA749,  (char32_t)0xA74B,  (char32_t)0xA74D,  (char32_t)0xA74F,  (char32_t)0xA751,
    (char32_t)0xA753,  (char32_t)0xA755,  (char32_t)0xA757,  (char32_t)0xA759,  (char32_t)0xA75B,
    (char32_t)0xA75D,  (char32_t)0xA75F,  (char32_t)0xA761,  (char32_t)0xA763,  (char32_t)0xA765,
    (char32_t)0xA767,  (char32_t)0xA769,  (char32_t)0xA76B,  (char32_t)0xA76D,  (char32_t)0xA76F,
    (char32_t)0xA77A,  (char32_t)0xA77C,  (char32_t)0x1D79,  (char32_t)0xA77F,  (char32_t)0xA781,
    (char32_t)0xA783,  (char32_t)0xA785,  (char32_t)0xA787,  (char32_t)0xA78C,  (char32_t)0x0265,
    (char32_t)0xA791,  (char32_t)0xA793,  (char32_t)0xA797,  (char32_t)0xA799,  (char32_t)0xA79B,
    (char32_t)0xA79D,  (char32_t)0xA79F,  (char32_t)0xA7A1,  (char32_t)0xA7A3,  (char32_t)0xA7A5,
    (char32_t)0xA7A7,  (char32_t)0xA7A9,  (char32_t)0x0266,  (char32_t)0x025C,  (char32_t)0x0261,
    (char32_t)0x026C,  (char32_t)0x026A,  (char32_t)0x029E,  (char32_t)0x0287,  (char32_t)0x029D,
    (char32_t)0xAB53,  (char32_t)0xA7B5,  (char32_t)0xA7B7,  (char32_t)0xA7B9,  (char32_t)0xA7BB,
    (char32_t)0xA7BD,  (char32_t)0xA7BF,  (char32_t)0xA7C1,  (char32_t)0xA7C3,  (char32_t)0xA794,
    (char32_t)0x0282,  (char32_t)0x1D8E,  (char32_t)0xA7C8,  (char32_t)0xA7CA,  (char32_t)0xA7D1,
    (char32_t)0xA7D7,  (char32_t)0xA7D9,  (char32_t)0xA7F6,  (char32_t)0x13A0,  (char32_t)0x13A1,
    (char32_t)0x13A2,  (char32_t)0x13A3,  (char32_t)0x13A4,  (char32_t)0x13A5,  (char32_t)0x13A6,
    (char32_t)0x13A7,  (char32_t)0x13A8,  (char32_t)0x13A9,  (char32_t)0x13AA,  (char32_t)0x13AB,
    (char32_t)0x13AC,  (char32_t)0x13AD,  (char32_t)0x13AE,  (char32_t)0x13AF,  (char32_t)0x13B0,
    (char32_t)0x13B1,  (char32_t)0x13B2,  (char32_t)0x13B3,  (char32_t)0x13B4,  (char32_t)0x13B5,
    (char32_t)0x13B6,  (char32_t)0x13B7,  (char32_t)0x13B8,  (char32_t)0x13B9,  (char32_t)0x13BA,
    (char32_t)0x13BB,  (char32_t)0x13BC,  (char32_t)0x13BD,  (char32_t)0x13BE,  (char32_t)0x13BF,
    (char32_t)0x13C0,  (char32_t)0x13C1,  (char32_t)0x13C2,  (char32_t)0x13C3,  (char32_t)0x13C4,
    (char32_t)0x13C5,  (char32_t)0x13C6,  (char32_t)0x13C7,  (char32_t)0x13C8,  (char32_t)0x13C9,
    (char32_t)0x13CA,  (char32_t)0x13CB,  (char32_t)0x13CC,  (char32_t)0x13CD,  (char32_t)0x13CE,
    (char32_t)0x13CF,  (char32_t)0x13D0,  (char32_t)0x13D1,  (char32_t)0x13D2,  (char32_t)0x13D3,
    (char32_t)0x13D4,  (char32_t)0x13D5,  (char32_t)0x13D6,  (char32_t)0x13D7,  (char32_t)0x13D8,
    (char32_t)0x13D9,  (char32_t)0x13DA,  (char32_t)0x13DB,  (char32_t)0x13DC,  (char32_t)0x13DD,
    (char32_t)0x13DE,  (char32_t)0x13DF,  (char32_t)0x13E0,  (char32_t)0x13E1,  (char32_t)0x13E2,
    (char32_t)0x13E3,  (char32_t)0x13E4,  (char32_t)0x13E5,  (char32_t)0x13E6,  (char32_t)0x13E7,
    (char32_t)0x13E8,  (char32_t)0x13E9,  (char32_t)0x13EA,  (char32_t)0x13EB,  (char32_t)0x13EC,
    (char32_t)0x13ED,  (char32_t)0x13EE,  (char32_t)0x13EF,  (char32_t)0xFF41,  (char32_t)0xFF42,
    (char32_t)0xFF43,  (char32_t)0xFF44,  (char32_t)0xFF45,  (char32_t)0xFF46,  (char32_t)0xFF47,
    (char32_t)0xFF48,  (char32_t)0xFF49,  (char32_t)0xFF4A,  (char32_t)0xFF4B,  (char32_t)0xFF4C,
    (char32_t)0xFF4D,  (char32_t)0xFF4E,  (char32_t)0xFF4F,  (char32_t)0xFF50,  (char32_t)0xFF51,
    (char32_t)0xFF52,  (char32_t)0xFF53,  (char32_t)0xFF54,  (char32_t)0xFF55,  (char32_t)0xFF56,
    (char32_t)0xFF57,  (char32_t)0xFF58,  (char32_t)0xFF59,  (char32_t)0xFF5A,  (char32_t)0x10428,
    (char32_t)0x10429, (char32_t)0x1042A, (char32_t)0x1042B, (char32_t)0x1042C, (char32_t)0x1042D,
    (char32_t)0x1042E, (char32_t)0x1042F, (char32_t)0x10430, (char32_t)0x10431, (char32_t)0x10432,
    (char32_t)0x10433, (char32_t)0x10434, (char32_t)0x10435, (char32_t)0x10436, (char32_t)0x10437,
    (char32_t)0x10438, (char32_t)0x10439, (char32_t)0x1043A, (char32_t)0x1043B, (char32_t)0x1043C,
    (char32_t)0x1043D, (char32_t)0x1043E, (char32_t)0x1043F, (char32_t)0x10440, (char32_t)0x10441,
    (char32_t)0x10442, (char32_t)0x10443, (char32_t)0x10444, (char32_t)0x10445, (char32_t)0x10446,
    (char32_t)0x10447, (char32_t)0x10448, (char32_t)0x10449, (char32_t)0x1044A, (char32_t)0x1044B,
    (char32_t)0x1044C, (char32_t)0x1044D, (char32_t)0x1044E, (char32_t)0x1044F, (char32_t)0x104D8,
    (char32_t)0x104D9, (char32_t)0x104DA, (char32_t)0x104DB, (char32_t)0x104DC, (char32_t)0x104DD,
    (char32_t)0x104DE, (char32_t)0x104DF, (char32_t)0x104E0, (char32_t)0x104E1, (char32_t)0x104E2,
    (char32_t)0x104E3, (char32_t)0x104E4, (char32_t)0x104E5, (char32_t)0x104E6, (char32_t)0x104E7,
    (char32_t)0x104E8, (char32_t)0x104E9, (char32_t)0x104EA, (char32_t)0x104EB, (char32_t)0x104EC,
    (char32_t)0x104ED, (char32_t)0x104EE, (char32_t)0x104EF, (char32_t)0x104F0, (char32_t)0x104F1,
    (char32_t)0x104F2, (char32_t)0x104F3, (char32_t)0x104F4, (char32_t)0x104F5, (char32_t)0x104F6,
    (char32_t)0x104F7, (char32_t)0x104F8, (char32_t)0x104F9, (char32_t)0x104FA, (char32_t)0x104FB,
    (char32_t)0x10597, (char32_t)0x10598, (char32_t)0x10599, (char32_t)0x1059A, (char32_t)0x1059B,
    (char32_t)0x1059C, (char32_t)0x1059D, (char32_t)0x1059E, (char32_t)0x1059F, (char32_t)0x105A0,
    (char32_t)0x105A1, (char32_t)0x105A3, (char32_t)0x105A4, (char32_t)0x105A5, (char32_t)0x105A6,
    (char32_t)0x105A7, (char32_t)0x105A8, (char32_t)0x105A9, (char32_t)0x105AA, (char32_t)0x105AB,
    (char32_t)0x105AC, (char32_t)0x105AD, (char32_t)0x105AE, (char32_t)0x105AF, (char32_t)0x105B0,
    (char32_t)0x105B1, (char32_t)0x105B3, (char32_t)0x105B4, (char32_t)0x105B5, (char32_t)0x105B6,
    (char32_t)0x105B7, (char32_t)0x105B8, (char32_t)0x105B9, (char32_t)0x105BB, (char32_t)0x105BC,
    (char32_t)0x10CC0, (char32_t)0x10CC1, (char32_t)0x10CC2, (char32_t)0x10CC3, (char32_t)0x10CC4,
    (char32_t)0x10CC5, (char32_t)0x10CC6, (char32_t)0x10CC7, (char32_t)0x10CC8, (char32_t)0x10CC9,
    (char32_t)0x10CCA, (char32_t)0x10CCB, (char32_t)0x10CCC, (char32_t)0x10CCD, (char32_t)0x10CCE,
    (char32_t)0x10CCF, (char32_t)0x10CD0, (char32_t)0x10CD1, (char32_t)0x10CD2, (char32_t)0x10CD3,
    (char32_t)0x10CD4, (char32_t)0x10CD5, (char32_t)0x10CD6, (char32_t)0x10CD7, (char32_t)0x10CD8,
    (char32_t)0x10CD9, (char32_t)0x10CDA, (char32_t)0x10CDB, (char32_t)0x10CDC, (char32_t)0x10CDD,
    (char32_t)0x10CDE, (char32_t)0x10CDF, (char32_t)0x10CE0, (char32_t)0x10CE1, (char32_t)0x10CE2,
    (char32_t)0x10CE3, (char32_t)0x10CE4, (char32_t)0x10CE5, (char32_t)0x10CE6, (char32_t)0x10CE7,
    (char32_t)0x10CE8, (char32_t)0x10CE9, (char32_t)0x10CEA, (char32_t)0x10CEB, (char32_t)0x10CEC,
    (char32_t)0x10CED, (char32_t)0x10CEE, (char32_t)0x10CEF, (char32_t)0x10CF0, (char32_t)0x10CF1,
    (char32_t)0x10CF2, (char32_t)0x118C0, (char32_t)0x118C1, (char32_t)0x118C2, (char32_t)0x118C3,
    (char32_t)0x118C4, (char32_t)0x118C5, (char32_t)0x118C6, (char32_t)0x118C7, (char32_t)0x118C8,
    (char32_t)0x118C9, (char32_t)0x118CA, (char32_t)0x118CB, (char32_t)0x118CC, (char32_t)0x118CD,
    (char32_t)0x118CE, (char32_t)0x118CF, (char32_t)0x118D0, (char32_t)0x118D1, (char32_t)0x118D2,
    (char32_t)0x118D3, (char32_t)0x118D4, (char32_t)0x118D5, (char32_t)0x118D6, (char32_t)0x118D7,
    (char32_t)0x118D8, (char32_t)0x118D9, (char32_t)0x118DA, (char32_t)0x118DB, (char32_t)0x118DC,
    (char32_t)0x118DD, (char32_t)0x118DE, (char32_t)0x118DF, (char32_t)0x16E60, (char32_t)0x16E61,
    (char32_t)0x16E62, (char32_t)0x16E63, (char32_t)0x16E64, (char32_t)0x16E65, (char32_t)0x16E66,
    (char32_t)0x16E67, (char32_t)0x16E68, (char32_t)0x16E69, (char32_t)0x16E6A, (char32_t)0x16E6B,
    (char32_t)0x16E6C, (char32_t)0x16E6D, (char32_t)0x16E6E, (char32_t)0x16E6F, (char32_t)0x16E70,
    (char32_t)0x16E71, (char32_t)0x16E72, (char32_t)0x16E73, (char32_t)0x16E74, (char32_t)0x16E75,
    (char32_t)0x16E76, (char32_t)0x16E77, (char32_t)0x16E78, (char32_t)0x16E79, (char32_t)0x16E7A,
    (char32_t)0x16E7B, (char32_t)0x16E7C, (char32_t)0x16E7D, (char32_t)0x16E7E, (char32_t)0x16E7F,
    (char32_t)0x1E922, (char32_t)0x1E923, (char32_t)0x1E924, (char32_t)0x1E925, (char32_t)0x1E926,
    (char32_t)0x1E927, (char32_t)0x1E928, (char32_t)0x1E929, (char32_t)0x1E92A, (char32_t)0x1E92B,
    (char32_t)0x1E92C, (char32_t)0x1E92D, (char32_t)0x1E92E, (char32_t)0x1E92F, (char32_t)0x1E930,
    (char32_t)0x1E931, (char32_t)0x1E932, (char32_t)0x1E933, (char32_t)0x1E934, (char32_t)0x1E935,
    (char32_t)0x1E936, (char32_t)0x1E937, (char32_t)0x1E938, (char32_t)0x1E939, (char32_t)0x1E93A,
    (char32_t)0x1E93B, (char32_t)0x1E93C, (char32_t)0x1E93D, (char32_t)0x1E93E, (char32_t)0x1E93F,
    (char32_t)0x1E940, (char32_t)0x1E941, (char32_t)0x1E942, (char32_t)0x1E943};

class charMapping
{
public:
  char32_t input;
  char32_t output;
};

std::string To_UTF8(const std::u32string& s)
{
  // TODO: Need destructor

  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.to_bytes(s);
}

static void dump_second_level_table(std::string label,
                                    char32_t upper_index,
                                    int table_length,
                                    const char32_t second_level_table[])
{

  std::cout << std::hex << std::noshowbase // manually show the 0x prefix
            << std::internal // fill between the prefix and the number
            << std::setfill('0'); // fill with 0s

  std::cout << "static const char32_t " << label << "_0x" << std::setw(5) << upper_index
            << "[] =" << std::endl;
  std::cout << "{" << std::endl;

  if (table_length == 0)
    return;

  // Write # of elements in table on first line
  std::cout << "  U'\\x" << std::setw(5) << std::hex << second_level_table[0] << "',";

  int items_on_row = 100;
  for (int i = 1; i <= table_length; i++)
  {
    if (i > 1)
    {
      std::cout << ",  ";
    }
    if (items_on_row > 6)
    {
      std::cout << std::endl;
      items_on_row = 0;
    }
    if (items_on_row == 0)
      std::cout << "  "; // indent

    std::cout << "U'\\x" << std::setw(5) << std::hex << second_level_table[i] << "'";

    items_on_row++;
  }
  if (items_on_row != 0)
    std::cout << std::endl;

  std::cout << "};" << std::endl << std::endl;
}

static void dump_first_level_table(std::string label,
                                   int table_length,
                                   const uint16_t firstLevelTable[])
{
  std::cout << std::noshowbase // manually show the 0x prefix
            << std::internal // fill between the prefix and the number
            << std::setfill('0'); // fill with 0s

  std::cout << "static const char32_t* " << label << "_INDEX [] =" << std::endl;
  std::cout << "{";

  int items_on_row = 8;
  bool firstComma = true;
  for (int i = 0; i < table_length; i++)
  {
    if (not firstComma)
    {
      std::cout << ",  ";
    }
    if (items_on_row > 5)
    {
      std::cout << std::endl << "  ";
      items_on_row = 0;
    }
    if ((i != 0) and (firstLevelTable[i] == 0))
    {
      std::cout << "0x0";
    }
    else
    {
      std::cout << label << "_0x" << std::setw(5) << std::hex << (i);
    }
    items_on_row++;
    firstComma = false;
  }
  std::cout << std::endl << "};" << std::endl << std::endl;
}

static void doTable(std::string label, const std::vector<charMapping> charMap, char32_t maxChar)
{
  char32_t c = 0x0;
  int idx = 0;
  char32_t start;
  char32_t end;
  char32_t previous_inputCodepoint_high_bytes = 0xFFFFFFFF;
  char32_t second_level_table[257] = {}; // 256 + size
  int second_level_table_index = 1; // first element reserved for length
  int endOfSecondTable = 0;
  int first_level_table_index = 0;
  uint16_t firstLevelTable[0x200] = {};

  std::cout << std::showbase // show the 0x prefix
            << std::internal // fill between the prefix and the number
            << std::setfill('0'); // fill with 0s

  while (true)
  {
    char32_t inputCodepoint = charMap[idx].input;
    char32_t inputCodepoint_high_bytes = c >> 8;
    if (c == maxChar + 1)
    {
      inputCodepoint_high_bytes = previous_inputCodepoint_high_bytes + 1;
    }
    if (inputCodepoint_high_bytes != previous_inputCodepoint_high_bytes)
    {
      first_level_table_index = previous_inputCodepoint_high_bytes;
      firstLevelTable[first_level_table_index] = 0; // Mark as empty, correct later
      if (endOfSecondTable > 1)
      {
        // Don't include tables that have no upper/lower code-points

        second_level_table[0] = endOfSecondTable;
        dump_second_level_table(label, previous_inputCodepoint_high_bytes, endOfSecondTable,
                                second_level_table);
        firstLevelTable[first_level_table_index] = previous_inputCodepoint_high_bytes;
        if (c >= maxChar)
          break;
      }
      second_level_table_index = 1; // First element reserved for length
      endOfSecondTable = second_level_table_index;
      previous_inputCodepoint_high_bytes = inputCodepoint_high_bytes;
    }
    if (c == inputCodepoint)
    {
      endOfSecondTable = second_level_table_index;
      second_level_table[second_level_table_index++] = charMap[idx].output;
      idx++;
    }
    else
    {
      second_level_table[second_level_table_index++] = 0x0;
    }

    c++;
  }
  dump_first_level_table(label, first_level_table_index + 1, firstLevelTable);
}

bool compare(const charMapping& left, const charMapping& right)
{
  bool less = left.input < right.input;
  bool failed = false;
  if ((left.input == 0) or (right.input == 0))
  {
    failed = true;
  }
  return less;
}

int main()
{
  std::vector<charMapping> ToLowers((sizeof(unicode_fold_upper) / sizeof(char32_t)));

  for (int i = 0; i < ToLowers.size(); i++)
  {
    ToLowers[i].input = unicode_fold_upper[i];
    ToLowers[i].output = unicode_fold_lower[i];
  }

  sort(ToLowers.begin(), ToLowers.end(), compare);

  doTable("FOLDCASE", ToLowers,
          unicode_fold_upper[sizeof(unicode_fold_upper) / sizeof(char32_t) - 1]);
}
