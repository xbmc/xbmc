/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
//-----------------------------------------------------------------------
//
//  File:      StringUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to use J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------


#include "StringUtils.h"
#include "CharsetConverter.h"
#include "utils/fstrcmp.h"
#include "Util.h"
#include "LangInfo.h"
#include <locale>
#include <functional>

#include <assert.h>
#include <math.h>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <algorithm>
#include "utils/RegExp.h" // don't move or std functions end up in PCRE namespace

#define FORMAT_BLOCK_SIZE 512 // # of bytes for initial allocation for printf

using namespace std;

const char* ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";

/* empty string for use in returns by ref */
const std::string StringUtils::Empty = "";
std::string StringUtils::m_lastUUID = "";

//	Copyright (c) Leigh Brasington 2012.  All rights reserved.
//  This code may be used and reproduced without written permission.
//  http://www.leighb.com/tounicupper.htm
//
//	The tables were constructed from
//	http://publib.boulder.ibm.com/infocenter/iseries/v7r1m0/index.jsp?topic=%2Fnls%2Frbagslowtoupmaptable.htm

static wchar_t unicode_lowers[] = {
  (wchar_t)0x0061, (wchar_t)0x0062, (wchar_t)0x0063, (wchar_t)0x0064, (wchar_t)0x0065, (wchar_t)0x0066, (wchar_t)0x0067, (wchar_t)0x0068, (wchar_t)0x0069,
  (wchar_t)0x006A, (wchar_t)0x006B, (wchar_t)0x006C, (wchar_t)0x006D, (wchar_t)0x006E, (wchar_t)0x006F, (wchar_t)0x0070, (wchar_t)0x0071, (wchar_t)0x0072,
  (wchar_t)0x0073, (wchar_t)0x0074, (wchar_t)0x0075, (wchar_t)0x0076, (wchar_t)0x0077, (wchar_t)0x0078, (wchar_t)0x0079, (wchar_t)0x007A, (wchar_t)0x00E0,
  (wchar_t)0x00E1, (wchar_t)0x00E2, (wchar_t)0x00E3, (wchar_t)0x00E4, (wchar_t)0x00E5, (wchar_t)0x00E6, (wchar_t)0x00E7, (wchar_t)0x00E8, (wchar_t)0x00E9,
  (wchar_t)0x00EA, (wchar_t)0x00EB, (wchar_t)0x00EC, (wchar_t)0x00ED, (wchar_t)0x00EE, (wchar_t)0x00EF, (wchar_t)0x00F0, (wchar_t)0x00F1, (wchar_t)0x00F2,
  (wchar_t)0x00F3, (wchar_t)0x00F4, (wchar_t)0x00F5, (wchar_t)0x00F6, (wchar_t)0x00F8, (wchar_t)0x00F9, (wchar_t)0x00FA, (wchar_t)0x00FB, (wchar_t)0x00FC,
  (wchar_t)0x00FD, (wchar_t)0x00FE, (wchar_t)0x00FF, (wchar_t)0x0101, (wchar_t)0x0103, (wchar_t)0x0105, (wchar_t)0x0107, (wchar_t)0x0109, (wchar_t)0x010B,
  (wchar_t)0x010D, (wchar_t)0x010F, (wchar_t)0x0111, (wchar_t)0x0113, (wchar_t)0x0115, (wchar_t)0x0117, (wchar_t)0x0119, (wchar_t)0x011B, (wchar_t)0x011D,
  (wchar_t)0x011F, (wchar_t)0x0121, (wchar_t)0x0123, (wchar_t)0x0125, (wchar_t)0x0127, (wchar_t)0x0129, (wchar_t)0x012B, (wchar_t)0x012D, (wchar_t)0x012F,
  (wchar_t)0x0131, (wchar_t)0x0133, (wchar_t)0x0135, (wchar_t)0x0137, (wchar_t)0x013A, (wchar_t)0x013C, (wchar_t)0x013E, (wchar_t)0x0140, (wchar_t)0x0142,
  (wchar_t)0x0144, (wchar_t)0x0146, (wchar_t)0x0148, (wchar_t)0x014B, (wchar_t)0x014D, (wchar_t)0x014F, (wchar_t)0x0151, (wchar_t)0x0153, (wchar_t)0x0155,
  (wchar_t)0x0157, (wchar_t)0x0159, (wchar_t)0x015B, (wchar_t)0x015D, (wchar_t)0x015F, (wchar_t)0x0161, (wchar_t)0x0163, (wchar_t)0x0165, (wchar_t)0x0167,
  (wchar_t)0x0169, (wchar_t)0x016B, (wchar_t)0x016D, (wchar_t)0x016F, (wchar_t)0x0171, (wchar_t)0x0173, (wchar_t)0x0175, (wchar_t)0x0177, (wchar_t)0x017A,
  (wchar_t)0x017C, (wchar_t)0x017E, (wchar_t)0x0183, (wchar_t)0x0185, (wchar_t)0x0188, (wchar_t)0x018C, (wchar_t)0x0192, (wchar_t)0x0199, (wchar_t)0x01A1,
  (wchar_t)0x01A3, (wchar_t)0x01A5, (wchar_t)0x01A8, (wchar_t)0x01AD, (wchar_t)0x01B0, (wchar_t)0x01B4, (wchar_t)0x01B6, (wchar_t)0x01B9, (wchar_t)0x01BD,
  (wchar_t)0x01C6, (wchar_t)0x01C9, (wchar_t)0x01CC, (wchar_t)0x01CE, (wchar_t)0x01D0, (wchar_t)0x01D2, (wchar_t)0x01D4, (wchar_t)0x01D6, (wchar_t)0x01D8,
  (wchar_t)0x01DA, (wchar_t)0x01DC, (wchar_t)0x01DF, (wchar_t)0x01E1, (wchar_t)0x01E3, (wchar_t)0x01E5, (wchar_t)0x01E7, (wchar_t)0x01E9, (wchar_t)0x01EB,
  (wchar_t)0x01ED, (wchar_t)0x01EF, (wchar_t)0x01F3, (wchar_t)0x01F5, (wchar_t)0x01FB, (wchar_t)0x01FD, (wchar_t)0x01FF, (wchar_t)0x0201, (wchar_t)0x0203,
  (wchar_t)0x0205, (wchar_t)0x0207, (wchar_t)0x0209, (wchar_t)0x020B, (wchar_t)0x020D, (wchar_t)0x020F, (wchar_t)0x0211, (wchar_t)0x0213, (wchar_t)0x0215,
  (wchar_t)0x0217, (wchar_t)0x0253, (wchar_t)0x0254, (wchar_t)0x0257, (wchar_t)0x0258, (wchar_t)0x0259, (wchar_t)0x025B, (wchar_t)0x0260, (wchar_t)0x0263,
  (wchar_t)0x0268, (wchar_t)0x0269, (wchar_t)0x026F, (wchar_t)0x0272, (wchar_t)0x0275, (wchar_t)0x0283, (wchar_t)0x0288, (wchar_t)0x028A, (wchar_t)0x028B,
  (wchar_t)0x0292, (wchar_t)0x03AC, (wchar_t)0x03AD, (wchar_t)0x03AE, (wchar_t)0x03AF, (wchar_t)0x03B1, (wchar_t)0x03B2, (wchar_t)0x03B3, (wchar_t)0x03B4,
  (wchar_t)0x03B5, (wchar_t)0x03B6, (wchar_t)0x03B7, (wchar_t)0x03B8, (wchar_t)0x03B9, (wchar_t)0x03BA, (wchar_t)0x03BB, (wchar_t)0x03BC, (wchar_t)0x03BD,
  (wchar_t)0x03BE, (wchar_t)0x03BF, (wchar_t)0x03C0, (wchar_t)0x03C1, (wchar_t)0x03C3, (wchar_t)0x03C4, (wchar_t)0x03C5, (wchar_t)0x03C6, (wchar_t)0x03C7,
  (wchar_t)0x03C8, (wchar_t)0x03C9, (wchar_t)0x03CA, (wchar_t)0x03CB, (wchar_t)0x03CC, (wchar_t)0x03CD, (wchar_t)0x03CE, (wchar_t)0x03E3, (wchar_t)0x03E5,
  (wchar_t)0x03E7, (wchar_t)0x03E9, (wchar_t)0x03EB, (wchar_t)0x03ED, (wchar_t)0x03EF, (wchar_t)0x0430, (wchar_t)0x0431, (wchar_t)0x0432, (wchar_t)0x0433,
  (wchar_t)0x0434, (wchar_t)0x0435, (wchar_t)0x0436, (wchar_t)0x0437, (wchar_t)0x0438, (wchar_t)0x0439, (wchar_t)0x043A, (wchar_t)0x043B, (wchar_t)0x043C,
  (wchar_t)0x043D, (wchar_t)0x043E, (wchar_t)0x043F, (wchar_t)0x0440, (wchar_t)0x0441, (wchar_t)0x0442, (wchar_t)0x0443, (wchar_t)0x0444, (wchar_t)0x0445,
  (wchar_t)0x0446, (wchar_t)0x0447, (wchar_t)0x0448, (wchar_t)0x0449, (wchar_t)0x044A, (wchar_t)0x044B, (wchar_t)0x044C, (wchar_t)0x044D, (wchar_t)0x044E,
  (wchar_t)0x044F, (wchar_t)0x0451, (wchar_t)0x0452, (wchar_t)0x0453, (wchar_t)0x0454, (wchar_t)0x0455, (wchar_t)0x0456, (wchar_t)0x0457, (wchar_t)0x0458,
  (wchar_t)0x0459, (wchar_t)0x045A, (wchar_t)0x045B, (wchar_t)0x045C, (wchar_t)0x045E, (wchar_t)0x045F, (wchar_t)0x0461, (wchar_t)0x0463, (wchar_t)0x0465,
  (wchar_t)0x0467, (wchar_t)0x0469, (wchar_t)0x046B, (wchar_t)0x046D, (wchar_t)0x046F, (wchar_t)0x0471, (wchar_t)0x0473, (wchar_t)0x0475, (wchar_t)0x0477,
  (wchar_t)0x0479, (wchar_t)0x047B, (wchar_t)0x047D, (wchar_t)0x047F, (wchar_t)0x0481, (wchar_t)0x0491, (wchar_t)0x0493, (wchar_t)0x0495, (wchar_t)0x0497,
  (wchar_t)0x0499, (wchar_t)0x049B, (wchar_t)0x049D, (wchar_t)0x049F, (wchar_t)0x04A1, (wchar_t)0x04A3, (wchar_t)0x04A5, (wchar_t)0x04A7, (wchar_t)0x04A9,
  (wchar_t)0x04AB, (wchar_t)0x04AD, (wchar_t)0x04AF, (wchar_t)0x04B1, (wchar_t)0x04B3, (wchar_t)0x04B5, (wchar_t)0x04B7, (wchar_t)0x04B9, (wchar_t)0x04BB,
  (wchar_t)0x04BD, (wchar_t)0x04BF, (wchar_t)0x04C2, (wchar_t)0x04C4, (wchar_t)0x04C8, (wchar_t)0x04CC, (wchar_t)0x04D1, (wchar_t)0x04D3, (wchar_t)0x04D5,
  (wchar_t)0x04D7, (wchar_t)0x04D9, (wchar_t)0x04DB, (wchar_t)0x04DD, (wchar_t)0x04DF, (wchar_t)0x04E1, (wchar_t)0x04E3, (wchar_t)0x04E5, (wchar_t)0x04E7,
  (wchar_t)0x04E9, (wchar_t)0x04EB, (wchar_t)0x04EF, (wchar_t)0x04F1, (wchar_t)0x04F3, (wchar_t)0x04F5, (wchar_t)0x04F9, (wchar_t)0x0561, (wchar_t)0x0562,
  (wchar_t)0x0563, (wchar_t)0x0564, (wchar_t)0x0565, (wchar_t)0x0566, (wchar_t)0x0567, (wchar_t)0x0568, (wchar_t)0x0569, (wchar_t)0x056A, (wchar_t)0x056B,
  (wchar_t)0x056C, (wchar_t)0x056D, (wchar_t)0x056E, (wchar_t)0x056F, (wchar_t)0x0570, (wchar_t)0x0571, (wchar_t)0x0572, (wchar_t)0x0573, (wchar_t)0x0574,
  (wchar_t)0x0575, (wchar_t)0x0576, (wchar_t)0x0577, (wchar_t)0x0578, (wchar_t)0x0579, (wchar_t)0x057A, (wchar_t)0x057B, (wchar_t)0x057C, (wchar_t)0x057D,
  (wchar_t)0x057E, (wchar_t)0x057F, (wchar_t)0x0580, (wchar_t)0x0581, (wchar_t)0x0582, (wchar_t)0x0583, (wchar_t)0x0584, (wchar_t)0x0585, (wchar_t)0x0586,
  (wchar_t)0x10D0, (wchar_t)0x10D1, (wchar_t)0x10D2, (wchar_t)0x10D3, (wchar_t)0x10D4, (wchar_t)0x10D5, (wchar_t)0x10D6, (wchar_t)0x10D7, (wchar_t)0x10D8,
  (wchar_t)0x10D9, (wchar_t)0x10DA, (wchar_t)0x10DB, (wchar_t)0x10DC, (wchar_t)0x10DD, (wchar_t)0x10DE, (wchar_t)0x10DF, (wchar_t)0x10E0, (wchar_t)0x10E1,
  (wchar_t)0x10E2, (wchar_t)0x10E3, (wchar_t)0x10E4, (wchar_t)0x10E5, (wchar_t)0x10E6, (wchar_t)0x10E7, (wchar_t)0x10E8, (wchar_t)0x10E9, (wchar_t)0x10EA,
  (wchar_t)0x10EB, (wchar_t)0x10EC, (wchar_t)0x10ED, (wchar_t)0x10EE, (wchar_t)0x10EF, (wchar_t)0x10F0, (wchar_t)0x10F1, (wchar_t)0x10F2, (wchar_t)0x10F3,
  (wchar_t)0x10F4, (wchar_t)0x10F5, (wchar_t)0x1E01, (wchar_t)0x1E03, (wchar_t)0x1E05, (wchar_t)0x1E07, (wchar_t)0x1E09, (wchar_t)0x1E0B, (wchar_t)0x1E0D,
  (wchar_t)0x1E0F, (wchar_t)0x1E11, (wchar_t)0x1E13, (wchar_t)0x1E15, (wchar_t)0x1E17, (wchar_t)0x1E19, (wchar_t)0x1E1B, (wchar_t)0x1E1D, (wchar_t)0x1E1F,
  (wchar_t)0x1E21, (wchar_t)0x1E23, (wchar_t)0x1E25, (wchar_t)0x1E27, (wchar_t)0x1E29, (wchar_t)0x1E2B, (wchar_t)0x1E2D, (wchar_t)0x1E2F, (wchar_t)0x1E31,
  (wchar_t)0x1E33, (wchar_t)0x1E35, (wchar_t)0x1E37, (wchar_t)0x1E39, (wchar_t)0x1E3B, (wchar_t)0x1E3D, (wchar_t)0x1E3F, (wchar_t)0x1E41, (wchar_t)0x1E43,
  (wchar_t)0x1E45, (wchar_t)0x1E47, (wchar_t)0x1E49, (wchar_t)0x1E4B, (wchar_t)0x1E4D, (wchar_t)0x1E4F, (wchar_t)0x1E51, (wchar_t)0x1E53, (wchar_t)0x1E55,
  (wchar_t)0x1E57, (wchar_t)0x1E59, (wchar_t)0x1E5B, (wchar_t)0x1E5D, (wchar_t)0x1E5F, (wchar_t)0x1E61, (wchar_t)0x1E63, (wchar_t)0x1E65, (wchar_t)0x1E67,
  (wchar_t)0x1E69, (wchar_t)0x1E6B, (wchar_t)0x1E6D, (wchar_t)0x1E6F, (wchar_t)0x1E71, (wchar_t)0x1E73, (wchar_t)0x1E75, (wchar_t)0x1E77, (wchar_t)0x1E79,
  (wchar_t)0x1E7B, (wchar_t)0x1E7D, (wchar_t)0x1E7F, (wchar_t)0x1E81, (wchar_t)0x1E83, (wchar_t)0x1E85, (wchar_t)0x1E87, (wchar_t)0x1E89, (wchar_t)0x1E8B,
  (wchar_t)0x1E8D, (wchar_t)0x1E8F, (wchar_t)0x1E91, (wchar_t)0x1E93, (wchar_t)0x1E95, (wchar_t)0x1EA1, (wchar_t)0x1EA3, (wchar_t)0x1EA5, (wchar_t)0x1EA7,
  (wchar_t)0x1EA9, (wchar_t)0x1EAB, (wchar_t)0x1EAD, (wchar_t)0x1EAF, (wchar_t)0x1EB1, (wchar_t)0x1EB3, (wchar_t)0x1EB5, (wchar_t)0x1EB7, (wchar_t)0x1EB9,
  (wchar_t)0x1EBB, (wchar_t)0x1EBD, (wchar_t)0x1EBF, (wchar_t)0x1EC1, (wchar_t)0x1EC3, (wchar_t)0x1EC5, (wchar_t)0x1EC7, (wchar_t)0x1EC9, (wchar_t)0x1ECB,
  (wchar_t)0x1ECD, (wchar_t)0x1ECF, (wchar_t)0x1ED1, (wchar_t)0x1ED3, (wchar_t)0x1ED5, (wchar_t)0x1ED7, (wchar_t)0x1ED9, (wchar_t)0x1EDB, (wchar_t)0x1EDD,
  (wchar_t)0x1EDF, (wchar_t)0x1EE1, (wchar_t)0x1EE3, (wchar_t)0x1EE5, (wchar_t)0x1EE7, (wchar_t)0x1EE9, (wchar_t)0x1EEB, (wchar_t)0x1EED, (wchar_t)0x1EEF,
  (wchar_t)0x1EF1, (wchar_t)0x1EF3, (wchar_t)0x1EF5, (wchar_t)0x1EF7, (wchar_t)0x1EF9, (wchar_t)0x1F00, (wchar_t)0x1F01, (wchar_t)0x1F02, (wchar_t)0x1F03,
  (wchar_t)0x1F04, (wchar_t)0x1F05, (wchar_t)0x1F06, (wchar_t)0x1F07, (wchar_t)0x1F10, (wchar_t)0x1F11, (wchar_t)0x1F12, (wchar_t)0x1F13, (wchar_t)0x1F14,
  (wchar_t)0x1F15, (wchar_t)0x1F20, (wchar_t)0x1F21, (wchar_t)0x1F22, (wchar_t)0x1F23, (wchar_t)0x1F24, (wchar_t)0x1F25, (wchar_t)0x1F26, (wchar_t)0x1F27,
  (wchar_t)0x1F30, (wchar_t)0x1F31, (wchar_t)0x1F32, (wchar_t)0x1F33, (wchar_t)0x1F34, (wchar_t)0x1F35, (wchar_t)0x1F36, (wchar_t)0x1F37, (wchar_t)0x1F40,
  (wchar_t)0x1F41, (wchar_t)0x1F42, (wchar_t)0x1F43, (wchar_t)0x1F44, (wchar_t)0x1F45, (wchar_t)0x1F51, (wchar_t)0x1F53, (wchar_t)0x1F55, (wchar_t)0x1F57,
  (wchar_t)0x1F60, (wchar_t)0x1F61, (wchar_t)0x1F62, (wchar_t)0x1F63, (wchar_t)0x1F64, (wchar_t)0x1F65, (wchar_t)0x1F66, (wchar_t)0x1F67, (wchar_t)0x1F80,
  (wchar_t)0x1F81, (wchar_t)0x1F82, (wchar_t)0x1F83, (wchar_t)0x1F84, (wchar_t)0x1F85, (wchar_t)0x1F86, (wchar_t)0x1F87, (wchar_t)0x1F90, (wchar_t)0x1F91,
  (wchar_t)0x1F92, (wchar_t)0x1F93, (wchar_t)0x1F94, (wchar_t)0x1F95, (wchar_t)0x1F96, (wchar_t)0x1F97, (wchar_t)0x1FA0, (wchar_t)0x1FA1, (wchar_t)0x1FA2,
  (wchar_t)0x1FA3, (wchar_t)0x1FA4, (wchar_t)0x1FA5, (wchar_t)0x1FA6, (wchar_t)0x1FA7, (wchar_t)0x1FB0, (wchar_t)0x1FB1, (wchar_t)0x1FD0, (wchar_t)0x1FD1,
  (wchar_t)0x1FE0, (wchar_t)0x1FE1, (wchar_t)0x24D0, (wchar_t)0x24D1, (wchar_t)0x24D2, (wchar_t)0x24D3, (wchar_t)0x24D4, (wchar_t)0x24D5, (wchar_t)0x24D6,
  (wchar_t)0x24D7, (wchar_t)0x24D8, (wchar_t)0x24D9, (wchar_t)0x24DA, (wchar_t)0x24DB, (wchar_t)0x24DC, (wchar_t)0x24DD, (wchar_t)0x24DE, (wchar_t)0x24DF,
  (wchar_t)0x24E0, (wchar_t)0x24E1, (wchar_t)0x24E2, (wchar_t)0x24E3, (wchar_t)0x24E4, (wchar_t)0x24E5, (wchar_t)0x24E6, (wchar_t)0x24E7, (wchar_t)0x24E8,
  (wchar_t)0x24E9, (wchar_t)0xFF41, (wchar_t)0xFF42, (wchar_t)0xFF43, (wchar_t)0xFF44, (wchar_t)0xFF45, (wchar_t)0xFF46, (wchar_t)0xFF47, (wchar_t)0xFF48,
  (wchar_t)0xFF49, (wchar_t)0xFF4A, (wchar_t)0xFF4B, (wchar_t)0xFF4C, (wchar_t)0xFF4D, (wchar_t)0xFF4E, (wchar_t)0xFF4F, (wchar_t)0xFF50, (wchar_t)0xFF51,
  (wchar_t)0xFF52, (wchar_t)0xFF53, (wchar_t)0xFF54, (wchar_t)0xFF55, (wchar_t)0xFF56, (wchar_t)0xFF57, (wchar_t)0xFF58, (wchar_t)0xFF59, (wchar_t)0xFF5A
};

static const wchar_t unicode_uppers[] = {
  (wchar_t)0x0041, (wchar_t)0x0042, (wchar_t)0x0043, (wchar_t)0x0044, (wchar_t)0x0045, (wchar_t)0x0046, (wchar_t)0x0047, (wchar_t)0x0048, (wchar_t)0x0049,
  (wchar_t)0x004A, (wchar_t)0x004B, (wchar_t)0x004C, (wchar_t)0x004D, (wchar_t)0x004E, (wchar_t)0x004F, (wchar_t)0x0050, (wchar_t)0x0051, (wchar_t)0x0052,
  (wchar_t)0x0053, (wchar_t)0x0054, (wchar_t)0x0055, (wchar_t)0x0056, (wchar_t)0x0057, (wchar_t)0x0058, (wchar_t)0x0059, (wchar_t)0x005A, (wchar_t)0x00C0,
  (wchar_t)0x00C1, (wchar_t)0x00C2, (wchar_t)0x00C3, (wchar_t)0x00C4, (wchar_t)0x00C5, (wchar_t)0x00C6, (wchar_t)0x00C7, (wchar_t)0x00C8, (wchar_t)0x00C9,
  (wchar_t)0x00CA, (wchar_t)0x00CB, (wchar_t)0x00CC, (wchar_t)0x00CD, (wchar_t)0x00CE, (wchar_t)0x00CF, (wchar_t)0x00D0, (wchar_t)0x00D1, (wchar_t)0x00D2,
  (wchar_t)0x00D3, (wchar_t)0x00D4, (wchar_t)0x00D5, (wchar_t)0x00D6, (wchar_t)0x00D8, (wchar_t)0x00D9, (wchar_t)0x00DA, (wchar_t)0x00DB, (wchar_t)0x00DC,
  (wchar_t)0x00DD, (wchar_t)0x00DE, (wchar_t)0x0178, (wchar_t)0x0100, (wchar_t)0x0102, (wchar_t)0x0104, (wchar_t)0x0106, (wchar_t)0x0108, (wchar_t)0x010A,
  (wchar_t)0x010C, (wchar_t)0x010E, (wchar_t)0x0110, (wchar_t)0x0112, (wchar_t)0x0114, (wchar_t)0x0116, (wchar_t)0x0118, (wchar_t)0x011A, (wchar_t)0x011C,
  (wchar_t)0x011E, (wchar_t)0x0120, (wchar_t)0x0122, (wchar_t)0x0124, (wchar_t)0x0126, (wchar_t)0x0128, (wchar_t)0x012A, (wchar_t)0x012C, (wchar_t)0x012E,
  (wchar_t)0x0049, (wchar_t)0x0132, (wchar_t)0x0134, (wchar_t)0x0136, (wchar_t)0x0139, (wchar_t)0x013B, (wchar_t)0x013D, (wchar_t)0x013F, (wchar_t)0x0141,
  (wchar_t)0x0143, (wchar_t)0x0145, (wchar_t)0x0147, (wchar_t)0x014A, (wchar_t)0x014C, (wchar_t)0x014E, (wchar_t)0x0150, (wchar_t)0x0152, (wchar_t)0x0154,
  (wchar_t)0x0156, (wchar_t)0x0158, (wchar_t)0x015A, (wchar_t)0x015C, (wchar_t)0x015E, (wchar_t)0x0160, (wchar_t)0x0162, (wchar_t)0x0164, (wchar_t)0x0166,
  (wchar_t)0x0168, (wchar_t)0x016A, (wchar_t)0x016C, (wchar_t)0x016E, (wchar_t)0x0170, (wchar_t)0x0172, (wchar_t)0x0174, (wchar_t)0x0176, (wchar_t)0x0179,
  (wchar_t)0x017B, (wchar_t)0x017D, (wchar_t)0x0182, (wchar_t)0x0184, (wchar_t)0x0187, (wchar_t)0x018B, (wchar_t)0x0191, (wchar_t)0x0198, (wchar_t)0x01A0,
  (wchar_t)0x01A2, (wchar_t)0x01A4, (wchar_t)0x01A7, (wchar_t)0x01AC, (wchar_t)0x01AF, (wchar_t)0x01B3, (wchar_t)0x01B5, (wchar_t)0x01B8, (wchar_t)0x01BC,
  (wchar_t)0x01C4, (wchar_t)0x01C7, (wchar_t)0x01CA, (wchar_t)0x01CD, (wchar_t)0x01CF, (wchar_t)0x01D1, (wchar_t)0x01D3, (wchar_t)0x01D5, (wchar_t)0x01D7,
  (wchar_t)0x01D9, (wchar_t)0x01DB, (wchar_t)0x01DE, (wchar_t)0x01E0, (wchar_t)0x01E2, (wchar_t)0x01E4, (wchar_t)0x01E6, (wchar_t)0x01E8, (wchar_t)0x01EA,
  (wchar_t)0x01EC, (wchar_t)0x01EE, (wchar_t)0x01F1, (wchar_t)0x01F4, (wchar_t)0x01FA, (wchar_t)0x01FC, (wchar_t)0x01FE, (wchar_t)0x0200, (wchar_t)0x0202,
  (wchar_t)0x0204, (wchar_t)0x0206, (wchar_t)0x0208, (wchar_t)0x020A, (wchar_t)0x020C, (wchar_t)0x020E, (wchar_t)0x0210, (wchar_t)0x0212, (wchar_t)0x0214,
  (wchar_t)0x0216, (wchar_t)0x0181, (wchar_t)0x0186, (wchar_t)0x018A, (wchar_t)0x018E, (wchar_t)0x018F, (wchar_t)0x0190, (wchar_t)0x0193, (wchar_t)0x0194,
  (wchar_t)0x0197, (wchar_t)0x0196, (wchar_t)0x019C, (wchar_t)0x019D, (wchar_t)0x019F, (wchar_t)0x01A9, (wchar_t)0x01AE, (wchar_t)0x01B1, (wchar_t)0x01B2,
  (wchar_t)0x01B7, (wchar_t)0x0386, (wchar_t)0x0388, (wchar_t)0x0389, (wchar_t)0x038A, (wchar_t)0x0391, (wchar_t)0x0392, (wchar_t)0x0393, (wchar_t)0x0394,
  (wchar_t)0x0395, (wchar_t)0x0396, (wchar_t)0x0397, (wchar_t)0x0398, (wchar_t)0x0399, (wchar_t)0x039A, (wchar_t)0x039B, (wchar_t)0x039C, (wchar_t)0x039D,
  (wchar_t)0x039E, (wchar_t)0x039F, (wchar_t)0x03A0, (wchar_t)0x03A1, (wchar_t)0x03A3, (wchar_t)0x03A4, (wchar_t)0x03A5, (wchar_t)0x03A6, (wchar_t)0x03A7,
  (wchar_t)0x03A8, (wchar_t)0x03A9, (wchar_t)0x03AA, (wchar_t)0x03AB, (wchar_t)0x038C, (wchar_t)0x038E, (wchar_t)0x038F, (wchar_t)0x03E2, (wchar_t)0x03E4,
  (wchar_t)0x03E6, (wchar_t)0x03E8, (wchar_t)0x03EA, (wchar_t)0x03EC, (wchar_t)0x03EE, (wchar_t)0x0410, (wchar_t)0x0411, (wchar_t)0x0412, (wchar_t)0x0413,
  (wchar_t)0x0414, (wchar_t)0x0415, (wchar_t)0x0416, (wchar_t)0x0417, (wchar_t)0x0418, (wchar_t)0x0419, (wchar_t)0x041A, (wchar_t)0x041B, (wchar_t)0x041C,
  (wchar_t)0x041D, (wchar_t)0x041E, (wchar_t)0x041F, (wchar_t)0x0420, (wchar_t)0x0421, (wchar_t)0x0422, (wchar_t)0x0423, (wchar_t)0x0424, (wchar_t)0x0425,
  (wchar_t)0x0426, (wchar_t)0x0427, (wchar_t)0x0428, (wchar_t)0x0429, (wchar_t)0x042A, (wchar_t)0x042B, (wchar_t)0x042C, (wchar_t)0x042D, (wchar_t)0x042E,
  (wchar_t)0x042F, (wchar_t)0x0401, (wchar_t)0x0402, (wchar_t)0x0403, (wchar_t)0x0404, (wchar_t)0x0405, (wchar_t)0x0406, (wchar_t)0x0407, (wchar_t)0x0408,
  (wchar_t)0x0409, (wchar_t)0x040A, (wchar_t)0x040B, (wchar_t)0x040C, (wchar_t)0x040E, (wchar_t)0x040F, (wchar_t)0x0460, (wchar_t)0x0462, (wchar_t)0x0464,
  (wchar_t)0x0466, (wchar_t)0x0468, (wchar_t)0x046A, (wchar_t)0x046C, (wchar_t)0x046E, (wchar_t)0x0470, (wchar_t)0x0472, (wchar_t)0x0474, (wchar_t)0x0476,
  (wchar_t)0x0478, (wchar_t)0x047A, (wchar_t)0x047C, (wchar_t)0x047E, (wchar_t)0x0480, (wchar_t)0x0490, (wchar_t)0x0492, (wchar_t)0x0494, (wchar_t)0x0496,
  (wchar_t)0x0498, (wchar_t)0x049A, (wchar_t)0x049C, (wchar_t)0x049E, (wchar_t)0x04A0, (wchar_t)0x04A2, (wchar_t)0x04A4, (wchar_t)0x04A6, (wchar_t)0x04A8,
  (wchar_t)0x04AA, (wchar_t)0x04AC, (wchar_t)0x04AE, (wchar_t)0x04B0, (wchar_t)0x04B2, (wchar_t)0x04B4, (wchar_t)0x04B6, (wchar_t)0x04B8, (wchar_t)0x04BA,
  (wchar_t)0x04BC, (wchar_t)0x04BE, (wchar_t)0x04C1, (wchar_t)0x04C3, (wchar_t)0x04C7, (wchar_t)0x04CB, (wchar_t)0x04D0, (wchar_t)0x04D2, (wchar_t)0x04D4,
  (wchar_t)0x04D6, (wchar_t)0x04D8, (wchar_t)0x04DA, (wchar_t)0x04DC, (wchar_t)0x04DE, (wchar_t)0x04E0, (wchar_t)0x04E2, (wchar_t)0x04E4, (wchar_t)0x04E6,
  (wchar_t)0x04E8, (wchar_t)0x04EA, (wchar_t)0x04EE, (wchar_t)0x04F0, (wchar_t)0x04F2, (wchar_t)0x04F4, (wchar_t)0x04F8, (wchar_t)0x0531, (wchar_t)0x0532,
  (wchar_t)0x0533, (wchar_t)0x0534, (wchar_t)0x0535, (wchar_t)0x0536, (wchar_t)0x0537, (wchar_t)0x0538, (wchar_t)0x0539, (wchar_t)0x053A, (wchar_t)0x053B,
  (wchar_t)0x053C, (wchar_t)0x053D, (wchar_t)0x053E, (wchar_t)0x053F, (wchar_t)0x0540, (wchar_t)0x0541, (wchar_t)0x0542, (wchar_t)0x0543, (wchar_t)0x0544,
  (wchar_t)0x0545, (wchar_t)0x0546, (wchar_t)0x0547, (wchar_t)0x0548, (wchar_t)0x0549, (wchar_t)0x054A, (wchar_t)0x054B, (wchar_t)0x054C, (wchar_t)0x054D,
  (wchar_t)0x054E, (wchar_t)0x054F, (wchar_t)0x0550, (wchar_t)0x0551, (wchar_t)0x0552, (wchar_t)0x0553, (wchar_t)0x0554, (wchar_t)0x0555, (wchar_t)0x0556,
  (wchar_t)0x10A0, (wchar_t)0x10A1, (wchar_t)0x10A2, (wchar_t)0x10A3, (wchar_t)0x10A4, (wchar_t)0x10A5, (wchar_t)0x10A6, (wchar_t)0x10A7, (wchar_t)0x10A8,
  (wchar_t)0x10A9, (wchar_t)0x10AA, (wchar_t)0x10AB, (wchar_t)0x10AC, (wchar_t)0x10AD, (wchar_t)0x10AE, (wchar_t)0x10AF, (wchar_t)0x10B0, (wchar_t)0x10B1,
  (wchar_t)0x10B2, (wchar_t)0x10B3, (wchar_t)0x10B4, (wchar_t)0x10B5, (wchar_t)0x10B6, (wchar_t)0x10B7, (wchar_t)0x10B8, (wchar_t)0x10B9, (wchar_t)0x10BA,
  (wchar_t)0x10BB, (wchar_t)0x10BC, (wchar_t)0x10BD, (wchar_t)0x10BE, (wchar_t)0x10BF, (wchar_t)0x10C0, (wchar_t)0x10C1, (wchar_t)0x10C2, (wchar_t)0x10C3,
  (wchar_t)0x10C4, (wchar_t)0x10C5, (wchar_t)0x1E00, (wchar_t)0x1E02, (wchar_t)0x1E04, (wchar_t)0x1E06, (wchar_t)0x1E08, (wchar_t)0x1E0A, (wchar_t)0x1E0C,
  (wchar_t)0x1E0E, (wchar_t)0x1E10, (wchar_t)0x1E12, (wchar_t)0x1E14, (wchar_t)0x1E16, (wchar_t)0x1E18, (wchar_t)0x1E1A, (wchar_t)0x1E1C, (wchar_t)0x1E1E,
  (wchar_t)0x1E20, (wchar_t)0x1E22, (wchar_t)0x1E24, (wchar_t)0x1E26, (wchar_t)0x1E28, (wchar_t)0x1E2A, (wchar_t)0x1E2C, (wchar_t)0x1E2E, (wchar_t)0x1E30,
  (wchar_t)0x1E32, (wchar_t)0x1E34, (wchar_t)0x1E36, (wchar_t)0x1E38, (wchar_t)0x1E3A, (wchar_t)0x1E3C, (wchar_t)0x1E3E, (wchar_t)0x1E40, (wchar_t)0x1E42,
  (wchar_t)0x1E44, (wchar_t)0x1E46, (wchar_t)0x1E48, (wchar_t)0x1E4A, (wchar_t)0x1E4C, (wchar_t)0x1E4E, (wchar_t)0x1E50, (wchar_t)0x1E52, (wchar_t)0x1E54,
  (wchar_t)0x1E56, (wchar_t)0x1E58, (wchar_t)0x1E5A, (wchar_t)0x1E5C, (wchar_t)0x1E5E, (wchar_t)0x1E60, (wchar_t)0x1E62, (wchar_t)0x1E64, (wchar_t)0x1E66,
  (wchar_t)0x1E68, (wchar_t)0x1E6A, (wchar_t)0x1E6C, (wchar_t)0x1E6E, (wchar_t)0x1E70, (wchar_t)0x1E72, (wchar_t)0x1E74, (wchar_t)0x1E76, (wchar_t)0x1E78,
  (wchar_t)0x1E7A, (wchar_t)0x1E7C, (wchar_t)0x1E7E, (wchar_t)0x1E80, (wchar_t)0x1E82, (wchar_t)0x1E84, (wchar_t)0x1E86, (wchar_t)0x1E88, (wchar_t)0x1E8A,
  (wchar_t)0x1E8C, (wchar_t)0x1E8E, (wchar_t)0x1E90, (wchar_t)0x1E92, (wchar_t)0x1E94, (wchar_t)0x1EA0, (wchar_t)0x1EA2, (wchar_t)0x1EA4, (wchar_t)0x1EA6,
  (wchar_t)0x1EA8, (wchar_t)0x1EAA, (wchar_t)0x1EAC, (wchar_t)0x1EAE, (wchar_t)0x1EB0, (wchar_t)0x1EB2, (wchar_t)0x1EB4, (wchar_t)0x1EB6, (wchar_t)0x1EB8,
  (wchar_t)0x1EBA, (wchar_t)0x1EBC, (wchar_t)0x1EBE, (wchar_t)0x1EC0, (wchar_t)0x1EC2, (wchar_t)0x1EC4, (wchar_t)0x1EC6, (wchar_t)0x1EC8, (wchar_t)0x1ECA,
  (wchar_t)0x1ECC, (wchar_t)0x1ECE, (wchar_t)0x1ED0, (wchar_t)0x1ED2, (wchar_t)0x1ED4, (wchar_t)0x1ED6, (wchar_t)0x1ED8, (wchar_t)0x1EDA, (wchar_t)0x1EDC,
  (wchar_t)0x1EDE, (wchar_t)0x1EE0, (wchar_t)0x1EE2, (wchar_t)0x1EE4, (wchar_t)0x1EE6, (wchar_t)0x1EE8, (wchar_t)0x1EEA, (wchar_t)0x1EEC, (wchar_t)0x1EEE,
  (wchar_t)0x1EF0, (wchar_t)0x1EF2, (wchar_t)0x1EF4, (wchar_t)0x1EF6, (wchar_t)0x1EF8, (wchar_t)0x1F08, (wchar_t)0x1F09, (wchar_t)0x1F0A, (wchar_t)0x1F0B,
  (wchar_t)0x1F0C, (wchar_t)0x1F0D, (wchar_t)0x1F0E, (wchar_t)0x1F0F, (wchar_t)0x1F18, (wchar_t)0x1F19, (wchar_t)0x1F1A, (wchar_t)0x1F1B, (wchar_t)0x1F1C,
  (wchar_t)0x1F1D, (wchar_t)0x1F28, (wchar_t)0x1F29, (wchar_t)0x1F2A, (wchar_t)0x1F2B, (wchar_t)0x1F2C, (wchar_t)0x1F2D, (wchar_t)0x1F2E, (wchar_t)0x1F2F,
  (wchar_t)0x1F38, (wchar_t)0x1F39, (wchar_t)0x1F3A, (wchar_t)0x1F3B, (wchar_t)0x1F3C, (wchar_t)0x1F3D, (wchar_t)0x1F3E, (wchar_t)0x1F3F, (wchar_t)0x1F48,
  (wchar_t)0x1F49, (wchar_t)0x1F4A, (wchar_t)0x1F4B, (wchar_t)0x1F4C, (wchar_t)0x1F4D, (wchar_t)0x1F59, (wchar_t)0x1F5B, (wchar_t)0x1F5D, (wchar_t)0x1F5F,
  (wchar_t)0x1F68, (wchar_t)0x1F69, (wchar_t)0x1F6A, (wchar_t)0x1F6B, (wchar_t)0x1F6C, (wchar_t)0x1F6D, (wchar_t)0x1F6E, (wchar_t)0x1F6F, (wchar_t)0x1F88,
  (wchar_t)0x1F89, (wchar_t)0x1F8A, (wchar_t)0x1F8B, (wchar_t)0x1F8C, (wchar_t)0x1F8D, (wchar_t)0x1F8E, (wchar_t)0x1F8F, (wchar_t)0x1F98, (wchar_t)0x1F99,
  (wchar_t)0x1F9A, (wchar_t)0x1F9B, (wchar_t)0x1F9C, (wchar_t)0x1F9D, (wchar_t)0x1F9E, (wchar_t)0x1F9F, (wchar_t)0x1FA8, (wchar_t)0x1FA9, (wchar_t)0x1FAA,
  (wchar_t)0x1FAB, (wchar_t)0x1FAC, (wchar_t)0x1FAD, (wchar_t)0x1FAE, (wchar_t)0x1FAF, (wchar_t)0x1FB8, (wchar_t)0x1FB9, (wchar_t)0x1FD8, (wchar_t)0x1FD9,
  (wchar_t)0x1FE8, (wchar_t)0x1FE9, (wchar_t)0x24B6, (wchar_t)0x24B7, (wchar_t)0x24B8, (wchar_t)0x24B9, (wchar_t)0x24BA, (wchar_t)0x24BB, (wchar_t)0x24BC,
  (wchar_t)0x24BD, (wchar_t)0x24BE, (wchar_t)0x24BF, (wchar_t)0x24C0, (wchar_t)0x24C1, (wchar_t)0x24C2, (wchar_t)0x24C3, (wchar_t)0x24C4, (wchar_t)0x24C5,
  (wchar_t)0x24C6, (wchar_t)0x24C7, (wchar_t)0x24C8, (wchar_t)0x24C9, (wchar_t)0x24CA, (wchar_t)0x24CB, (wchar_t)0x24CC, (wchar_t)0x24CD, (wchar_t)0x24CE,
  (wchar_t)0x24CF, (wchar_t)0xFF21, (wchar_t)0xFF22, (wchar_t)0xFF23, (wchar_t)0xFF24, (wchar_t)0xFF25, (wchar_t)0xFF26, (wchar_t)0xFF27, (wchar_t)0xFF28,
  (wchar_t)0xFF29, (wchar_t)0xFF2A, (wchar_t)0xFF2B, (wchar_t)0xFF2C, (wchar_t)0xFF2D, (wchar_t)0xFF2E, (wchar_t)0xFF2F, (wchar_t)0xFF30, (wchar_t)0xFF31,
  (wchar_t)0xFF32, (wchar_t)0xFF33, (wchar_t)0xFF34, (wchar_t)0xFF35, (wchar_t)0xFF36, (wchar_t)0xFF37, (wchar_t)0xFF38, (wchar_t)0xFF39, (wchar_t)0xFF3A
};

string StringUtils::Format(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  string str = FormatV(fmt, args);
  va_end(args);

  return str;
}

string StringUtils::FormatV(const char *fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return "";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  while (1) 
  {
    char *cstr = reinterpret_cast<char*>(malloc(sizeof(char) * size));
    if (!cstr)
      return "";

    va_copy(argCopy, args);
    int nActual = vsnprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);

    if (nActual > -1 && nActual < size) // We got a valid result
    {
      std::string str(cstr, nActual);
      free(cstr);
      return str;
    }
    free(cstr);
#ifndef TARGET_WINDOWS
    if (nActual > -1)                   // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else                                // Let's try to double the size (glibc 2.0)
      size *= 2;
#else  // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return "";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
  }

  return ""; // unreachable
}

wstring StringUtils::Format(const wchar_t *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  wstring str = FormatV(fmt, args);
  va_end(args);
  
  return str;
}

wstring StringUtils::FormatV(const wchar_t *fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return L"";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;
  
  while (1)
  {
    wchar_t *cstr = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * size));
    if (!cstr)
      return L"";

    va_copy(argCopy, args);
    int nActual = vswprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);
    
    if (nActual > -1 && nActual < size) // We got a valid result
    {
      std::wstring str(cstr, nActual);
      free(cstr);
      return str;
    }
    free(cstr);

#ifndef TARGET_WINDOWS
    if (nActual > -1)                   // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else                                // Let's try to double the size (glibc 2.0)
      size *= 2;
#else  // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscwprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return L"";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
  }
  
  return L"";
}

int compareWchar (const void* a, const void* b)
{
  if (*(wchar_t*)a <  *(wchar_t*)b)
    return -1;
  else if (*(wchar_t*)a >  *(wchar_t*)b)
    return 1;
  return 0;
}

wchar_t tolowerUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_uppers, sizeof(unicode_uppers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_lowers + (p - unicode_uppers));

  return c;
}

wchar_t toupperUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_lowers, sizeof(unicode_lowers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_uppers + (p - unicode_lowers));

  return c;
}

void StringUtils::ToUpper(string &str)
{
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void StringUtils::ToUpper(wstring &str)
{
  transform(str.begin(), str.end(), str.begin(), toupperUnicode);
}

void StringUtils::ToLower(string &str)
{
  transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void StringUtils::ToLower(wstring &str)
{
  transform(str.begin(), str.end(), str.begin(), tolowerUnicode);
}

void StringUtils::ToCapitalize(string &str)
{
  std::wstring wstr;
  g_charsetConverter.utf8ToW(str, wstr);
  ToCapitalize(wstr);
  g_charsetConverter.wToUTF8(wstr, str);
}

void StringUtils::ToCapitalize(std::wstring &str)
{
  const std::locale& loc = g_langInfo.GetSystemLocale();
  bool isFirstLetter = true;
  for (std::wstring::iterator it = str.begin(); it < str.end(); ++it)
  {
    // capitalize after spaces and punctuation characters (except apostrophes)
    if (std::isspace(*it, loc) || (std::ispunct(*it, loc) && *it != '\''))
      isFirstLetter = true;
    else if (isFirstLetter)
    {
      *it = std::toupper(*it, loc);
      isFirstLetter = false;
    }
  }
}

bool StringUtils::EqualsNoCase(const std::string &str1, const std::string &str2)
{
  return EqualsNoCase(str1.c_str(), str2.c_str());
}

bool StringUtils::EqualsNoCase(const std::string &str1, const char *s2)
{
  return EqualsNoCase(str1.c_str(), s2);
}

bool StringUtils::EqualsNoCase(const char *s1, const char *s2)
{
  char c2; // we need only one char outside the loop
  do
  {
    const char c1 = *s1++; // const local variable should help compiler to optimize
    c2 = *s2++;
    if (c1 != c2 && ::tolower(c1) != ::tolower(c2)) // This includes the possibility that one of the characters is the null-terminator, which implies a string mismatch.
      return false;
  } while (c2 != '\0'); // At this point, we know c1 == c2, so there's no need to test them both.
  return true;
}

int StringUtils::CompareNoCase(const std::string &str1, const std::string &str2)
{
  return CompareNoCase(str1.c_str(), str2.c_str());
}

int StringUtils::CompareNoCase(const char *s1, const char *s2)
{
  char c2; // we need only one char outside the loop
  do
  {
    const char c1 = *s1++; // const local variable should help compiler to optimize
    c2 = *s2++;
    if (c1 != c2 && ::tolower(c1) != ::tolower(c2)) // This includes the possibility that one of the characters is the null-terminator, which implies a string mismatch.
      return ::tolower(c1) - ::tolower(c2);
  } while (c2 != '\0'); // At this point, we know c1 == c2, so there's no need to test them both.
  return 0;
}

string StringUtils::Left(const string &str, size_t count)
{
  count = max((size_t)0, min(count, str.size()));
  return str.substr(0, count);
}

string StringUtils::Mid(const string &str, size_t first, size_t count /* = string::npos */)
{
  if (first + count > str.size())
    count = str.size() - first;
  
  if (first > str.size())
    return string();
  
  assert(first + count <= str.size());
  
  return str.substr(first, count);
}

string StringUtils::Right(const string &str, size_t count)
{
  count = max((size_t)0, min(count, str.size()));
  return str.substr(str.size() - count);
}

std::string& StringUtils::Trim(std::string &str)
{
  TrimLeft(str);
  return TrimRight(str);
}

std::string& StringUtils::Trim(std::string &str, const char* const chars)
{
  TrimLeft(str, chars);
  return TrimRight(str, chars);
}

// hack to check only first byte of UTF-8 character
// without this hack "TrimX" functions failed on Win32 and OS X with UTF-8 strings
static int isspace_c(char c)
{
  return (c & 0x80) == 0 && ::isspace(c);
}

std::string& StringUtils::TrimLeft(std::string &str)
{
  str.erase(str.begin(), ::find_if(str.begin(), str.end(), ::not1(::ptr_fun(isspace_c))));
  return str;
}

std::string& StringUtils::TrimLeft(std::string &str, const char* const chars)
{
  size_t nidx = str.find_first_not_of(chars);
  str.erase(0, nidx);
  return str;
}

std::string& StringUtils::TrimRight(std::string &str)
{
  str.erase(::find_if(str.rbegin(), str.rend(), ::not1(::ptr_fun(isspace_c))).base(), str.end());
  return str;
}

std::string& StringUtils::TrimRight(std::string &str, const char* const chars)
{
  size_t nidx = str.find_last_not_of(chars);
  str.erase(str.npos == nidx ? 0 : ++nidx);
  return str;
}

std::string& StringUtils::RemoveDuplicatedSpacesAndTabs(std::string& str)
{
  std::string::iterator it = str.begin();
  bool onSpace = false;
  while(it != str.end())
  {
    if (*it == '\t')
      *it = ' ';

    if (*it == ' ')
    {
      if (onSpace)
      {
        it = str.erase(it);
        continue;
      }
      else
        onSpace = true;
    }
    else
      onSpace = false;

    ++it;
  }
  return str;
}

int StringUtils::Replace(string &str, char oldChar, char newChar)
{
  int replacedChars = 0;
  for (string::iterator it = str.begin(); it != str.end(); ++it)
  {
    if (*it == oldChar)
    {
      *it = newChar;
      replacedChars++;
    }
  }
  
  return replacedChars;
}

int StringUtils::Replace(std::string &str, const std::string &oldStr, const std::string &newStr)
{
  if (oldStr.empty())
    return 0;

  int replacedChars = 0;
  size_t index = 0;
  
  while (index < str.size() && (index = str.find(oldStr, index)) != string::npos)
  {
    str.replace(index, oldStr.size(), newStr);
    index += newStr.size();
    replacedChars++;
  }
  
  return replacedChars;
}

int StringUtils::Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr)
{
  if (oldStr.empty())
    return 0;

  int replacedChars = 0;
  size_t index = 0;

  while (index < str.size() && (index = str.find(oldStr, index)) != string::npos)
  {
    str.replace(index, oldStr.size(), newStr);
    index += newStr.size();
    replacedChars++;
  }

  return replacedChars;
}

bool StringUtils::StartsWith(const std::string &str1, const std::string &str2)
{
  return str1.compare(0, str2.size(), str2) == 0;
}

bool StringUtils::StartsWith(const std::string &str1, const char *s2)
{
  return StartsWith(str1.c_str(), s2);
}

bool StringUtils::StartsWith(const char *s1, const char *s2)
{
  while (*s2 != '\0')
  {
    if (*s1 != *s2)
      return false;
    s1++;
    s2++;
  }
  return true;
}

bool StringUtils::StartsWithNoCase(const std::string &str1, const std::string &str2)
{
  return StartsWithNoCase(str1.c_str(), str2.c_str());
}

bool StringUtils::StartsWithNoCase(const std::string &str1, const char *s2)
{
  return StartsWithNoCase(str1.c_str(), s2);
}

bool StringUtils::StartsWithNoCase(const char *s1, const char *s2)
{
  while (*s2 != '\0')
  {
    if (::tolower(*s1) != ::tolower(*s2))
      return false;
    s1++;
    s2++;
  }
  return true;
}

bool StringUtils::EndsWith(const std::string &str1, const std::string &str2)
{
  if (str1.size() < str2.size())
    return false;
  return str1.compare(str1.size() - str2.size(), str2.size(), str2) == 0;
}

bool StringUtils::EndsWith(const std::string &str1, const char *s2)
{
  size_t len2 = strlen(s2);
  if (str1.size() < len2)
    return false;
  return str1.compare(str1.size() - len2, len2, s2) == 0;
}

bool StringUtils::EndsWithNoCase(const std::string &str1, const std::string &str2)
{
  if (str1.size() < str2.size())
    return false;
  const char *s1 = str1.c_str() + str1.size() - str2.size();
  const char *s2 = str2.c_str();
  while (*s2 != '\0')
  {
    if (::tolower(*s1) != ::tolower(*s2))
      return false;
    s1++;
    s2++;
  }
  return true;
}

bool StringUtils::EndsWithNoCase(const std::string &str1, const char *s2)
{
  size_t len2 = strlen(s2);
  if (str1.size() < len2)
    return false;
  const char *s1 = str1.c_str() + str1.size() - len2;
  while (*s2 != '\0')
  {
    if (::tolower(*s1) != ::tolower(*s2))
      return false;
    s1++;
    s2++;
  }
  return true;
}

std::string StringUtils::Join(const vector<string> &strings, const std::string& delimiter)
{
  std::string result;
  for(vector<string>::const_iterator it = strings.begin(); it != strings.end(); ++it )
    result += (*it) + delimiter;
  
  if (!result.empty())
    result.erase(result.size() - delimiter.size());
  return result;
}

vector<string> StringUtils::Split(const std::string& input, const std::string& delimiter, unsigned int iMaxStrings /* = 0 */)
{
  std::vector<std::string> results;
  if (input.empty())
    return results;
  if (delimiter.empty())
  {
    results.push_back(input);
    return results;
  }

  const size_t delimLen = delimiter.length();
  size_t nextDelim;
  size_t textPos = 0;
  do
  {
    if (--iMaxStrings == 0)
    {
      results.push_back(input.substr(textPos));
      break;
    }
    nextDelim = input.find(delimiter, textPos);
    results.push_back(input.substr(textPos, nextDelim - textPos));
    textPos = nextDelim + delimLen;
  } while (nextDelim != std::string::npos);

  return results;
}

std::vector<std::string> StringUtils::Split(const std::string& input, const char delimiter, size_t iMaxStrings /*= 0*/)
{
  std::vector<std::string> results;
  if (input.empty())
    return results;

  size_t nextDelim;
  size_t textPos = 0;
  do
  {
    if (--iMaxStrings == 0)
    {
      results.push_back(input.substr(textPos));
      break;
    }
    nextDelim = input.find(delimiter, textPos);
    results.push_back(input.substr(textPos, nextDelim - textPos));
    textPos = nextDelim + 1;
  } while (nextDelim != std::string::npos);

  return results;
}


// returns the number of occurrences of strFind in strInput.
int StringUtils::FindNumber(const std::string& strInput, const std::string &strFind)
{
  size_t pos = strInput.find(strFind, 0);
  int numfound = 0;
  while (pos != std::string::npos)
  {
    numfound++;
    pos = strInput.find(strFind, pos + 1);
  }
  return numfound;
}

// Compares separately the numeric and alphabetic parts of a string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical (essentially calculates left - right)
int64_t StringUtils::AlphaNumericCompare(const wchar_t *left, const wchar_t *right)
{
  wchar_t *l = (wchar_t *)left;
  wchar_t *r = (wchar_t *)right;
  wchar_t *ld, *rd;
  wchar_t lc, rc;
  int64_t lnum, rnum;
  const collate<wchar_t>& coll = use_facet< collate<wchar_t> >(g_langInfo.GetSystemLocale());
  int cmp_res = 0;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= L'0' && *l <= L'9' && *r >= L'0' && *r <= L'9')
    {
      ld = l;
      lnum = 0;
      while (*ld >= L'0' && *ld <= L'9' && ld < l + 15)
      { // compare only up to 15 digits
        lnum *= 10;
        lnum += *ld++ - '0';
      }
      rd = r;
      rnum = 0;
      while (*rd >= L'0' && *rd <= L'9' && rd < r + 15)
      { // compare only up to 15 digits
        rnum *= 10;
        rnum += *rd++ - L'0';
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return lnum - rnum;
      }
      l = ld;
      r = rd;
      continue;
    }
    // do case less comparison
    lc = *l;
    if (lc >= L'A' && lc <= L'Z')
      lc += L'a'-L'A';
    rc = *r;
    if (rc >= L'A' && rc <= L'Z')
      rc += L'a'- L'A';

    // ok, do a normal comparison, taking current locale into account. Add special case stuff (eg '(' characters)) in here later
    if ((cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1)) != 0)
    {
      return cmp_res;
    }
    l++; r++;
  }
  if (*r)
  { // r is longer
    return -1;
  }
  else if (*l)
  { // l is longer
    return 1;
  }
  return 0; // files are the same
}

int StringUtils::DateStringToYYYYMMDD(const std::string &dateString)
{
  vector<string> days = StringUtils::Split(dateString, '-');
  if (days.size() == 1)
    return atoi(days[0].c_str());
  else if (days.size() == 2)
    return atoi(days[0].c_str())*100+atoi(days[1].c_str());
  else if (days.size() == 3)
    return atoi(days[0].c_str())*10000+atoi(days[1].c_str())*100+atoi(days[2].c_str());
  else
    return -1;
}

long StringUtils::TimeStringToSeconds(const std::string &timeString)
{
  std::string strCopy(timeString);
  StringUtils::Trim(strCopy);
  if(StringUtils::EndsWithNoCase(strCopy, " min"))
  {
    // this is imdb format of "XXX min"
    return 60 * atoi(strCopy.c_str());
  }
  else
  {
    vector<string> secs = StringUtils::Split(strCopy, ':');
    int timeInSecs = 0;
    for (unsigned int i = 0; i < 3 && i < secs.size(); i++)
    {
      timeInSecs *= 60;
      timeInSecs += atoi(secs[i].c_str());
    }
    return timeInSecs;
  }
}

std::string StringUtils::SecondsToTimeString(long lSeconds, TIME_FORMAT format)
{
  int hh = lSeconds / 3600;
  lSeconds = lSeconds % 3600;
  int mm = lSeconds / 60;
  int ss = lSeconds % 60;

  if (format == TIME_FORMAT_GUESS)
    format = (hh >= 1) ? TIME_FORMAT_HH_MM_SS : TIME_FORMAT_MM_SS;
  std::string strHMS;
  if (format & TIME_FORMAT_HH)
    strHMS += StringUtils::Format("%02.2i", hh);
  else if (format & TIME_FORMAT_H)
    strHMS += StringUtils::Format("%i", hh);
  if (format & TIME_FORMAT_MM)
    strHMS += StringUtils::Format(strHMS.empty() ? "%02.2i" : ":%02.2i", mm);
  if (format & TIME_FORMAT_SS)
    strHMS += StringUtils::Format(strHMS.empty() ? "%02.2i" : ":%02.2i", ss);
  return strHMS;
}

bool StringUtils::IsNaturalNumber(const std::string& str)
{
  size_t i = 0, n = 0;
  // allow whitespace,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

bool StringUtils::IsInteger(const std::string& str)
{
  size_t i = 0, n = 0;
  // allow whitespace,-,digits,whitespace
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  if (i < str.size() && str[i] == '-')
    i++;
  while (i < str.size() && isdigit((unsigned char) str[i]))
  {
    i++; n++;
  }
  while (i < str.size() && isspace((unsigned char) str[i]))
    i++;
  return i == str.size() && n > 0;
}

int StringUtils::asciidigitvalue(char chr)
{
  if (!isasciidigit(chr))
    return -1;

  return chr - '0';
}

int StringUtils::asciixdigitvalue(char chr)
{
  int v = asciidigitvalue(chr);
  if (v >= 0)
    return v;
  if (chr >= 'a' && chr <= 'f')
    return chr - 'a' + 10;
  if (chr >= 'A' && chr <= 'F')
    return chr - 'A' + 10;

  return -1;
}


void StringUtils::RemoveCRLF(std::string& strLine)
{
  StringUtils::TrimRight(strLine, "\n\r");
}

std::string StringUtils::SizeToString(int64_t size)
{
  std::string strLabel;
  const char prefixes[] = {' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
  unsigned int i = 0;
  double s = (double)size;
  while (i < ARRAY_SIZE(prefixes) && s >= 1000.0)
  {
    s /= 1024.0;
    i++;
  }

  if (!i)
    strLabel = StringUtils::Format("%.0lf B", s);
  else if (s >= 100.0)
    strLabel = StringUtils::Format("%.1lf %cB", s, prefixes[i]);
  else
    strLabel = StringUtils::Format("%.2lf %cB", s, prefixes[i]);

  return strLabel;
}

// return -1 if not, else return the utf8 char length.
int IsUTF8Letter(const unsigned char *str)
{
  // reference:
  // unicode -> utf8 table: http://www.utf8-chartable.de/
  // latin characters in unicode: http://en.wikipedia.org/wiki/Latin_characters_in_Unicode
  unsigned char ch = str[0];
  if (!ch)
    return -1;
  if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
    return 1;
  if (!(ch & 0x80))
    return -1;
  unsigned char ch2 = str[1];
  if (!ch2)
    return -1;
  // check latin 1 letter table: http://en.wikipedia.org/wiki/C1_Controls_and_Latin-1_Supplement
  if (ch == 0xC3 && ch2 >= 0x80 && ch2 <= 0xBF && ch2 != 0x97 && ch2 != 0xB7)
    return 2;
  // check latin extended A table: http://en.wikipedia.org/wiki/Latin_Extended-A
  if (ch >= 0xC4 && ch <= 0xC7 && ch2 >= 0x80 && ch2 <= 0xBF)
    return 2;
  // check latin extended B table: http://en.wikipedia.org/wiki/Latin_Extended-B
  // and International Phonetic Alphabet: http://en.wikipedia.org/wiki/IPA_Extensions_(Unicode_block)
  if (((ch == 0xC8 || ch == 0xC9) && ch2 >= 0x80 && ch2 <= 0xBF)
      || (ch == 0xCA && ch2 >= 0x80 && ch2 <= 0xAF))
    return 2;
  return -1;
}

size_t StringUtils::FindWords(const char *str, const char *wordLowerCase)
{
  // NOTE: This assumes word is lowercase!
  unsigned char *s = (unsigned char *)str;
  do
  {
    // start with a compare
    unsigned char *c = s;
    unsigned char *w = (unsigned char *)wordLowerCase;
    bool same = true;
    while (same && *c && *w)
    {
      unsigned char lc = *c++;
      if (lc >= 'A' && lc <= 'Z')
        lc += 'a'-'A';

      if (lc != *w++) // different
        same = false;
    }
    if (same && *w == 0)  // only the same if word has been exhausted
      return (const char *)s - str;

    // otherwise, skip current word (composed by latin letters) or number
    int l;
    if (*s >= '0' && *s <= '9')
    {
      ++s;
      while (*s >= '0' && *s <= '9') ++s;
    }
    else if ((l = IsUTF8Letter(s)) > 0)
    {
      s += l;
      while ((l = IsUTF8Letter(s)) > 0) s += l;
    }
    else
      ++s;
    while (*s && *s == ' ') s++;

    // and repeat until we're done
  } while (*s);

  return std::string::npos;
}

// assumes it is called from after the first open bracket is found
int StringUtils::FindEndBracket(const std::string &str, char opener, char closer, int startPos)
{
  int blocks = 1;
  for (unsigned int i = startPos; i < str.size(); i++)
  {
    if (str[i] == opener)
      blocks++;
    else if (str[i] == closer)
    {
      blocks--;
      if (!blocks)
        return i;
    }
  }

  return (int)std::string::npos;
}

void StringUtils::WordToDigits(std::string &word)
{
  static const char word_to_letter[] = "22233344455566677778889999";
  StringUtils::ToLower(word);
  for (unsigned int i = 0; i < word.size(); ++i)
  { // NB: This assumes ascii, which probably needs extending at some  point.
    char letter = word[i];
    if ((letter >= 'a' && letter <= 'z')) // assume contiguous letter range
    {
      word[i] = word_to_letter[letter-'a'];
    }
    else if (letter < '0' || letter > '9') // We want to keep 0-9!
    {
      word[i] = ' ';  // replace everything else with a space
    }
  }
}

std::string StringUtils::CreateUUID()
{
  /* This function generate a DCE 1.1, ISO/IEC 11578:1996 and IETF RFC-4122
  * Version 4 conform local unique UUID based upon random number generation.
  */
  char UuidStrTmp[40];
  char *pUuidStr = UuidStrTmp;
  int i;

  static bool m_uuidInitialized = false;
  if (!m_uuidInitialized)
  {
    /* use current time as the seed for rand()*/
    srand(time(NULL));
    m_uuidInitialized = true;
  }

  /*Data1 - 8 characters.*/
  for(i = 0; i < 8; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data2 - 4 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 4; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data3 - 4 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 4; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data4 - 4 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 4; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  /*Data5 - 12 characters.*/
  *pUuidStr++ = '-';
  for(i = 0; i < 12; i++, pUuidStr++)
    ((*pUuidStr = (rand() % 16)) < 10) ? *pUuidStr += 48 : *pUuidStr += 55;

  *pUuidStr = '\0';

  m_lastUUID = UuidStrTmp;
  return UuidStrTmp;
}

bool StringUtils::ValidateUUID(const std::string &uuid)
{
  CRegExp guidRE;
  guidRE.RegComp(ADDON_GUID_RE);
  return (guidRE.RegFind(uuid.c_str()) == 0);
}

double StringUtils::CompareFuzzy(const std::string &left, const std::string &right)
{
  return (0.5 + fstrcmp(left.c_str(), right.c_str(), 0.0) * (left.length() + right.length())) / 2.0;
}

int StringUtils::FindBestMatch(const std::string &str, const vector<string> &strings, double &matchscore)
{
  int best = -1;
  matchscore = 0;

  int i = 0;
  for (vector<string>::const_iterator it = strings.begin(); it != strings.end(); ++it, i++)
  {
    int maxlength = max(str.length(), it->length());
    double score = StringUtils::CompareFuzzy(str, *it) / maxlength;
    if (score > matchscore)
    {
      matchscore = score;
      best = i;
    }
  }
  return best;
}

bool StringUtils::ContainsKeyword(const std::string &str, const vector<string> &keywords)
{
  for (vector<string>::const_iterator it = keywords.begin(); it != keywords.end(); ++it)
  {
    if (str.find(*it) != str.npos)
      return true;
  }
  return false;
}

size_t StringUtils::utf8_strlen(const char *s)
{
  size_t length = 0;
  while (*s)
  {
    if ((*s++ & 0xC0) != 0x80)
      length++;
  }
  return length;
}

std::string StringUtils::Paramify(const std::string &param)
{
  std::string result = param;
  // escape backspaces
  StringUtils::Replace(result, "\\", "\\\\");
  // escape double quotes
  StringUtils::Replace(result, "\"", "\\\"");

  // add double quotes around the whole string
  return "\"" + result + "\"";
}

std::vector<std::string> StringUtils::Tokenize(const std::string &input, const std::string &delimiters)
{
  std::vector<std::string> tokens;
  Tokenize(input, tokens, delimiters);
  return tokens;
}

void StringUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiters);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find_first_of(delimiters, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiters, nextDelimPos);
  }
}

std::vector<std::string> StringUtils::Tokenize(const std::string &input, const char delimiter)
{
  std::vector<std::string> tokens;
  Tokenize(input, tokens, delimiter);
  return tokens;
}

void StringUtils::Tokenize(const std::string& input, std::vector<std::string>& tokens, const char delimiter)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiter);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find(delimiter, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiter, nextDelimPos);
  }
}
