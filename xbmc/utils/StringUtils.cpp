/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

#ifdef HAVE_NEW_CROSSGUID
#include <crossguid/guid.hpp>
#else
#include <guid.h>
#endif

#if defined(TARGET_ANDROID)
#include <androidjni/JNIThreading.h>
#endif

#include "CharsetConverter.h"
#include "LangInfo.h"
#include "StringUtils.h"
#include "XBDateTime.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <codecvt>
#include <functional>
#include <inttypes.h>
#include <iomanip>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstrcmp.h>
#include <memory.h>

// don't move or std functions end up in PCRE namespace
// clang-format off
#include "utils/RegExp.h"
// clang-format on

#define FORMAT_BLOCK_SIZE 512 // # of bytes for initial allocation for printf

using namespace std::string_literals;

namespace
{
static bool FAIL_ON_ERROR = false;
static const std::string CONVERSION_ERROR("Conversion error");
static const std::wstring WIDE_CONVERSION_ERROR(L"Conversion error- wide");
static const std::u32string U32_CONVERSION_ERROR(U"Conversion error- U32");
/*!
 * \brief Converts a string to a number of a specified type, by using istringstream.
 * \param str The string to convert
 * \param fallback [OPT] The number to return when the conversion fails
 * \return The converted number, otherwise fallback if conversion fails
 */
template<typename T>
T NumberFromSS(std::string_view str, T fallback) noexcept
{
  std::istringstream iss{str.data()};
  T result{fallback};
  iss >> result;
  return result;
}
} // unnamed namespace

static constexpr const char* ADDON_GUID_RE = "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}(\\}){0,1}$";

/* empty string for use in returns by ref */
const std::string StringUtils::Empty = "";

namespace
{
// TODO: Is it best to move everything that should go in anonymous namespace together,
//       or leave some of it where it is (functions that logically fit where they are)?
//

/* Case folding tables are derived from Unicode Inc.
 * Data file, CaseFolding.txt (CaseFolding-14.0.0.txt, 2021-03-08). Copyright follows below.
 *
 * These tables provide for "simple case folding" that is not locale sensitive. They do NOT
 * support "Full Case Folding" which can fold single characters into multiple, or multiple
 * into single, etc. CaseFolding.txt can be found in the ICUC4 source directory:
 * icu/source/data/unidata.
 *
 * The home-grown tool to produce the case folding tables can be found in
 * xbmc/utils/unicode_tools. They are built and run by hand with the table data
 * pasted and formatted here. This only needs to be done if an error or an update
 * to CaseFolding.txt occurs.
 *
 * TODO: The following license agreement will be moved to the appropriate location
 * once that has been worked out.
 *
 * Terms of use:
 *
 * UNICODE, INC. LICENSE AGREEMENT - DATA FILES AND SOFTWARE
 *
 * See Terms of Use <https://www.unicode.org/copyright.html>
 * for definitions of Unicode Inc.’s Data Files and Software.
 *
 * NOTICE TO USER: Carefully read the following legal agreement.
 * BY DOWNLOADING, INSTALLING, COPYING OR OTHERWISE USING UNICODE INC.'S
 * DATA FILES ("DATA FILES"), AND/OR SOFTWARE ("SOFTWARE"),
 * YOU UNEQUIVOCALLY ACCEPT, AND AGREE TO BE BOUND BY, ALL OF THE
 * TERMS AND CONDITIONS OF THIS AGREEMENT.
 * IF YOU DO NOT AGREE, DO NOT DOWNLOAD, INSTALL, COPY, DISTRIBUTE OR USE
 * THE DATA FILES OR SOFTWARE.
 *
 * COPYRIGHT AND PERMISSION NOTICE
 *
 * Copyright © 1991-2022 Unicode, Inc. All rights reserved.
 * Distributed under the Terms of Use in https://www.unicode.org/copyright.html.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of the Unicode data files and any associated documentation
 * (the "Data Files") or Unicode software and any associated documentation
 * (the "Software") to deal in the Data Files or Software
 * without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, and/or sell copies of
 * the Data Files or Software, and to permit persons to whom the Data Files
 * or Software are furnished to do so, provided that either
 * (a) this copyright and permission notice appear with all copies
 * of the Data Files or Software, or
 * (b) this copyright and permission notice appear in associated
 * Documentation.
 *
 * THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS
 * NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL
 * DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THE DATA FILES OR SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in these Data Files or Software without prior
 * written authorization of the copyright holder.
 */

// The tables below are logically indexed by the 32-bit Unicode value of the
// character which is to be case folded. The value found in the table is
// the case folded value, or 0 if no such value exists.
//
// A char32_t contains a 32-bit Unicode codepoint, although only 24 bits is
// used. The array FOLDCASE_INDEX is indexed by the upper 16-bits of the
// the 24-bit codepoint, yielding a pointer to another table indexed by
// the lower 8-bits of the codepoint (FOLDCASE_0x0001, etc.) to get the lower-case equivalent
// for the original codepoint (see FoldCaseChar, below).
//
// Specifically, FOLDCASE_0x...[0] contains the number of elements in the
// array. This helps reduce the size of the table. All other non-zero elements
// contain the upper-case Unicode value for a fold-case codepoint. This
// means that "A", 0x041, the FoldCase value can be found by:
//
//   high_bytes = 0x41 >> 8; => 0
//   char32_t* table = FOLDCASE_INDEX[high_bytes]; => address of FOLDCASE_0000
//   uint16_t low_byte = c & 0xFF; => 0x41
//   char32_t foldedChar = table[low_byte + 1]; => 0x61 'a'
//
// clang-format off

static const char32_t FOLDCASE_0x00000[] =
{
 U'\x000df',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00061',  U'\x00062',  U'\x00063',  U'\x00064',  U'\x00065',
 U'\x00066',  U'\x00067',  U'\x00068',  U'\x00069',  U'\x0006a',  U'\x0006b',  U'\x0006c',
 U'\x0006d',  U'\x0006e',  U'\x0006f',  U'\x00070',  U'\x00071',  U'\x00072',  U'\x00073',
 U'\x00074',  U'\x00075',  U'\x00076',  U'\x00077',  U'\x00078',  U'\x00079',  U'\x0007a',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003bc',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x000e0',  U'\x000e1',  U'\x000e2',  U'\x000e3',
 U'\x000e4',  U'\x000e5',  U'\x000e6',  U'\x000e7',  U'\x000e8',  U'\x000e9',  U'\x000ea',
 U'\x000eb',  U'\x000ec',  U'\x000ed',  U'\x000ee',  U'\x000ef',  U'\x000f0',  U'\x000f1',
 U'\x000f2',  U'\x000f3',  U'\x000f4',  U'\x000f5',  U'\x000f6',  U'\x00000',  U'\x000f8',
 U'\x000f9',  U'\x000fa',  U'\x000fb',  U'\x000fc',  U'\x000fd',  U'\x000fe'
};

static const char32_t FOLDCASE_0x00001[] =
{
 U'\x000ff',
 U'\x00101',  U'\x00000',  U'\x00103',  U'\x00000',  U'\x00105',  U'\x00000',  U'\x00107',
 U'\x00000',  U'\x00109',  U'\x00000',  U'\x0010b',  U'\x00000',  U'\x0010d',  U'\x00000',
 U'\x0010f',  U'\x00000',  U'\x00111',  U'\x00000',  U'\x00113',  U'\x00000',  U'\x00115',
 U'\x00000',  U'\x00117',  U'\x00000',  U'\x00119',  U'\x00000',  U'\x0011b',  U'\x00000',
 U'\x0011d',  U'\x00000',  U'\x0011f',  U'\x00000',  U'\x00121',  U'\x00000',  U'\x00123',
 U'\x00000',  U'\x00125',  U'\x00000',  U'\x00127',  U'\x00000',  U'\x00129',  U'\x00000',
 U'\x0012b',  U'\x00000',  U'\x0012d',  U'\x00000',  U'\x0012f',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00133',  U'\x00000',  U'\x00135',  U'\x00000',  U'\x00137',  U'\x00000',
 U'\x00000',  U'\x0013a',  U'\x00000',  U'\x0013c',  U'\x00000',  U'\x0013e',  U'\x00000',
 U'\x00140',  U'\x00000',  U'\x00142',  U'\x00000',  U'\x00144',  U'\x00000',  U'\x00146',
 U'\x00000',  U'\x00148',  U'\x00000',  U'\x00000',  U'\x0014b',  U'\x00000',  U'\x0014d',
 U'\x00000',  U'\x0014f',  U'\x00000',  U'\x00151',  U'\x00000',  U'\x00153',  U'\x00000',
 U'\x00155',  U'\x00000',  U'\x00157',  U'\x00000',  U'\x00159',  U'\x00000',  U'\x0015b',
 U'\x00000',  U'\x0015d',  U'\x00000',  U'\x0015f',  U'\x00000',  U'\x00161',  U'\x00000',
 U'\x00163',  U'\x00000',  U'\x00165',  U'\x00000',  U'\x00167',  U'\x00000',  U'\x00169',
 U'\x00000',  U'\x0016b',  U'\x00000',  U'\x0016d',  U'\x00000',  U'\x0016f',  U'\x00000',
 U'\x00171',  U'\x00000',  U'\x00173',  U'\x00000',  U'\x00175',  U'\x00000',  U'\x00177',
 U'\x00000',  U'\x000ff',  U'\x0017a',  U'\x00000',  U'\x0017c',  U'\x00000',  U'\x0017e',
 U'\x00000',  U'\x00073',  U'\x00000',  U'\x00253',  U'\x00183',  U'\x00000',  U'\x00185',
 U'\x00000',  U'\x00254',  U'\x00188',  U'\x00000',  U'\x00256',  U'\x00257',  U'\x0018c',
 U'\x00000',  U'\x00000',  U'\x001dd',  U'\x00259',  U'\x0025b',  U'\x00192',  U'\x00000',
 U'\x00260',  U'\x00263',  U'\x00000',  U'\x00269',  U'\x00268',  U'\x00199',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x0026f',  U'\x00272',  U'\x00000',  U'\x00275',  U'\x001a1',
 U'\x00000',  U'\x001a3',  U'\x00000',  U'\x001a5',  U'\x00000',  U'\x00280',  U'\x001a8',
 U'\x00000',  U'\x00283',  U'\x00000',  U'\x00000',  U'\x001ad',  U'\x00000',  U'\x00288',
 U'\x001b0',  U'\x00000',  U'\x0028a',  U'\x0028b',  U'\x001b4',  U'\x00000',  U'\x001b6',
 U'\x00000',  U'\x00292',  U'\x001b9',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x001bd',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x001c6',  U'\x001c6',  U'\x00000',  U'\x001c9',  U'\x001c9',  U'\x00000',  U'\x001cc',
 U'\x001cc',  U'\x00000',  U'\x001ce',  U'\x00000',  U'\x001d0',  U'\x00000',  U'\x001d2',
 U'\x00000',  U'\x001d4',  U'\x00000',  U'\x001d6',  U'\x00000',  U'\x001d8',  U'\x00000',
 U'\x001da',  U'\x00000',  U'\x001dc',  U'\x00000',  U'\x00000',  U'\x001df',  U'\x00000',
 U'\x001e1',  U'\x00000',  U'\x001e3',  U'\x00000',  U'\x001e5',  U'\x00000',  U'\x001e7',
 U'\x00000',  U'\x001e9',  U'\x00000',  U'\x001eb',  U'\x00000',  U'\x001ed',  U'\x00000',
 U'\x001ef',  U'\x00000',  U'\x00000',  U'\x001f3',  U'\x001f3',  U'\x00000',  U'\x001f5',
 U'\x00000',  U'\x00195',  U'\x001bf',  U'\x001f9',  U'\x00000',  U'\x001fb',  U'\x00000',
 U'\x001fd',  U'\x00000',  U'\x001ff'
};

static const char32_t FOLDCASE_0x00002[] =
{
 U'\x0004f',
 U'\x00201',  U'\x00000',  U'\x00203',  U'\x00000',  U'\x00205',  U'\x00000',  U'\x00207',
 U'\x00000',  U'\x00209',  U'\x00000',  U'\x0020b',  U'\x00000',  U'\x0020d',  U'\x00000',
 U'\x0020f',  U'\x00000',  U'\x00211',  U'\x00000',  U'\x00213',  U'\x00000',  U'\x00215',
 U'\x00000',  U'\x00217',  U'\x00000',  U'\x00219',  U'\x00000',  U'\x0021b',  U'\x00000',
 U'\x0021d',  U'\x00000',  U'\x0021f',  U'\x00000',  U'\x0019e',  U'\x00000',  U'\x00223',
 U'\x00000',  U'\x00225',  U'\x00000',  U'\x00227',  U'\x00000',  U'\x00229',  U'\x00000',
 U'\x0022b',  U'\x00000',  U'\x0022d',  U'\x00000',  U'\x0022f',  U'\x00000',  U'\x00231',
 U'\x00000',  U'\x00233',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x02c65',  U'\x0023c',  U'\x00000',  U'\x0019a',  U'\x02c66',
 U'\x00000',  U'\x00000',  U'\x00242',  U'\x00000',  U'\x00180',  U'\x00289',  U'\x0028c',
 U'\x00247',  U'\x00000',  U'\x00249',  U'\x00000',  U'\x0024b',  U'\x00000',  U'\x0024d',
 U'\x00000',  U'\x0024f'
};

static const char32_t FOLDCASE_0x00003[] =
{
 U'\x00100',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003b9',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00371',  U'\x00000',  U'\x00373',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00377',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x003f3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x003ac',  U'\x00000',  U'\x003ad',  U'\x003ae',  U'\x003af',  U'\x00000',
 U'\x003cc',  U'\x00000',  U'\x003cd',  U'\x003ce',  U'\x00000',  U'\x003b1',  U'\x003b2',
 U'\x003b3',  U'\x003b4',  U'\x003b5',  U'\x003b6',  U'\x003b7',  U'\x003b8',  U'\x003b9',
 U'\x003ba',  U'\x003bb',  U'\x003bc',  U'\x003bd',  U'\x003be',  U'\x003bf',  U'\x003c0',
 U'\x003c1',  U'\x00000',  U'\x003c3',  U'\x003c4',  U'\x003c5',  U'\x003c6',  U'\x003c7',
 U'\x003c8',  U'\x003c9',  U'\x003ca',  U'\x003cb',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003c3',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003d7',  U'\x003b2',  U'\x003b8',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x003c6',  U'\x003c0',  U'\x00000',  U'\x003d9',
 U'\x00000',  U'\x003db',  U'\x00000',  U'\x003dd',  U'\x00000',  U'\x003df',  U'\x00000',
 U'\x003e1',  U'\x00000',  U'\x003e3',  U'\x00000',  U'\x003e5',  U'\x00000',  U'\x003e7',
 U'\x00000',  U'\x003e9',  U'\x00000',  U'\x003eb',  U'\x00000',  U'\x003ed',  U'\x00000',
 U'\x003ef',  U'\x00000',  U'\x003ba',  U'\x003c1',  U'\x00000',  U'\x00000',  U'\x003b8',
 U'\x003b5',  U'\x00000',  U'\x003f8',  U'\x00000',  U'\x003f2',  U'\x003fb',  U'\x00000',
 U'\x00000',  U'\x0037b',  U'\x0037c',  U'\x0037d'
};

static const char32_t FOLDCASE_0x00004[] =
{
 U'\x000ff',
 U'\x00450',  U'\x00451',  U'\x00452',  U'\x00453',  U'\x00454',  U'\x00455',  U'\x00456',
 U'\x00457',  U'\x00458',  U'\x00459',  U'\x0045a',  U'\x0045b',  U'\x0045c',  U'\x0045d',
 U'\x0045e',  U'\x0045f',  U'\x00430',  U'\x00431',  U'\x00432',  U'\x00433',  U'\x00434',
 U'\x00435',  U'\x00436',  U'\x00437',  U'\x00438',  U'\x00439',  U'\x0043a',  U'\x0043b',
 U'\x0043c',  U'\x0043d',  U'\x0043e',  U'\x0043f',  U'\x00440',  U'\x00441',  U'\x00442',
 U'\x00443',  U'\x00444',  U'\x00445',  U'\x00446',  U'\x00447',  U'\x00448',  U'\x00449',
 U'\x0044a',  U'\x0044b',  U'\x0044c',  U'\x0044d',  U'\x0044e',  U'\x0044f',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00461',  U'\x00000',
 U'\x00463',  U'\x00000',  U'\x00465',  U'\x00000',  U'\x00467',  U'\x00000',  U'\x00469',
 U'\x00000',  U'\x0046b',  U'\x00000',  U'\x0046d',  U'\x00000',  U'\x0046f',  U'\x00000',
 U'\x00471',  U'\x00000',  U'\x00473',  U'\x00000',  U'\x00475',  U'\x00000',  U'\x00477',
 U'\x00000',  U'\x00479',  U'\x00000',  U'\x0047b',  U'\x00000',  U'\x0047d',  U'\x00000',
 U'\x0047f',  U'\x00000',  U'\x00481',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0048b',  U'\x00000',
 U'\x0048d',  U'\x00000',  U'\x0048f',  U'\x00000',  U'\x00491',  U'\x00000',  U'\x00493',
 U'\x00000',  U'\x00495',  U'\x00000',  U'\x00497',  U'\x00000',  U'\x00499',  U'\x00000',
 U'\x0049b',  U'\x00000',  U'\x0049d',  U'\x00000',  U'\x0049f',  U'\x00000',  U'\x004a1',
 U'\x00000',  U'\x004a3',  U'\x00000',  U'\x004a5',  U'\x00000',  U'\x004a7',  U'\x00000',
 U'\x004a9',  U'\x00000',  U'\x004ab',  U'\x00000',  U'\x004ad',  U'\x00000',  U'\x004af',
 U'\x00000',  U'\x004b1',  U'\x00000',  U'\x004b3',  U'\x00000',  U'\x004b5',  U'\x00000',
 U'\x004b7',  U'\x00000',  U'\x004b9',  U'\x00000',  U'\x004bb',  U'\x00000',  U'\x004bd',
 U'\x00000',  U'\x004bf',  U'\x00000',  U'\x004cf',  U'\x004c2',  U'\x00000',  U'\x004c4',
 U'\x00000',  U'\x004c6',  U'\x00000',  U'\x004c8',  U'\x00000',  U'\x004ca',  U'\x00000',
 U'\x004cc',  U'\x00000',  U'\x004ce',  U'\x00000',  U'\x00000',  U'\x004d1',  U'\x00000',
 U'\x004d3',  U'\x00000',  U'\x004d5',  U'\x00000',  U'\x004d7',  U'\x00000',  U'\x004d9',
 U'\x00000',  U'\x004db',  U'\x00000',  U'\x004dd',  U'\x00000',  U'\x004df',  U'\x00000',
 U'\x004e1',  U'\x00000',  U'\x004e3',  U'\x00000',  U'\x004e5',  U'\x00000',  U'\x004e7',
 U'\x00000',  U'\x004e9',  U'\x00000',  U'\x004eb',  U'\x00000',  U'\x004ed',  U'\x00000',
 U'\x004ef',  U'\x00000',  U'\x004f1',  U'\x00000',  U'\x004f3',  U'\x00000',  U'\x004f5',
 U'\x00000',  U'\x004f7',  U'\x00000',  U'\x004f9',  U'\x00000',  U'\x004fb',  U'\x00000',
 U'\x004fd',  U'\x00000',  U'\x004ff'
};

static const char32_t FOLDCASE_0x00005[] =
{
 U'\x00057',
 U'\x00501',  U'\x00000',  U'\x00503',  U'\x00000',  U'\x00505',  U'\x00000',  U'\x00507',
 U'\x00000',  U'\x00509',  U'\x00000',  U'\x0050b',  U'\x00000',  U'\x0050d',  U'\x00000',
 U'\x0050f',  U'\x00000',  U'\x00511',  U'\x00000',  U'\x00513',  U'\x00000',  U'\x00515',
 U'\x00000',  U'\x00517',  U'\x00000',  U'\x00519',  U'\x00000',  U'\x0051b',  U'\x00000',
 U'\x0051d',  U'\x00000',  U'\x0051f',  U'\x00000',  U'\x00521',  U'\x00000',  U'\x00523',
 U'\x00000',  U'\x00525',  U'\x00000',  U'\x00527',  U'\x00000',  U'\x00529',  U'\x00000',
 U'\x0052b',  U'\x00000',  U'\x0052d',  U'\x00000',  U'\x0052f',  U'\x00000',  U'\x00000',
 U'\x00561',  U'\x00562',  U'\x00563',  U'\x00564',  U'\x00565',  U'\x00566',  U'\x00567',
 U'\x00568',  U'\x00569',  U'\x0056a',  U'\x0056b',  U'\x0056c',  U'\x0056d',  U'\x0056e',
 U'\x0056f',  U'\x00570',  U'\x00571',  U'\x00572',  U'\x00573',  U'\x00574',  U'\x00575',
 U'\x00576',  U'\x00577',  U'\x00578',  U'\x00579',  U'\x0057a',  U'\x0057b',  U'\x0057c',
 U'\x0057d',  U'\x0057e',  U'\x0057f',  U'\x00580',  U'\x00581',  U'\x00582',  U'\x00583',
 U'\x00584',  U'\x00585',  U'\x00586'
};

static const char32_t FOLDCASE_0x00010[] =
{
 U'\x000ce',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02d00',
 U'\x02d01',  U'\x02d02',  U'\x02d03',  U'\x02d04',  U'\x02d05',  U'\x02d06',  U'\x02d07',
 U'\x02d08',  U'\x02d09',  U'\x02d0a',  U'\x02d0b',  U'\x02d0c',  U'\x02d0d',  U'\x02d0e',
 U'\x02d0f',  U'\x02d10',  U'\x02d11',  U'\x02d12',  U'\x02d13',  U'\x02d14',  U'\x02d15',
 U'\x02d16',  U'\x02d17',  U'\x02d18',  U'\x02d19',  U'\x02d1a',  U'\x02d1b',  U'\x02d1c',
 U'\x02d1d',  U'\x02d1e',  U'\x02d1f',  U'\x02d20',  U'\x02d21',  U'\x02d22',  U'\x02d23',
 U'\x02d24',  U'\x02d25',  U'\x00000',  U'\x02d27',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x02d2d'
};

static const char32_t FOLDCASE_0x00013[] =
{
 U'\x000fe',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x013f0',  U'\x013f1',  U'\x013f2',  U'\x013f3',
 U'\x013f4',  U'\x013f5'
};

static const char32_t FOLDCASE_0x0001c[] =
{
 U'\x000c0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00432',  U'\x00434',  U'\x0043e',  U'\x00441',  U'\x00442',
 U'\x00442',  U'\x0044a',  U'\x00463',  U'\x0a64b',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x010d0',  U'\x010d1',  U'\x010d2',
 U'\x010d3',  U'\x010d4',  U'\x010d5',  U'\x010d6',  U'\x010d7',  U'\x010d8',  U'\x010d9',
 U'\x010da',  U'\x010db',  U'\x010dc',  U'\x010dd',  U'\x010de',  U'\x010df',  U'\x010e0',
 U'\x010e1',  U'\x010e2',  U'\x010e3',  U'\x010e4',  U'\x010e5',  U'\x010e6',  U'\x010e7',
 U'\x010e8',  U'\x010e9',  U'\x010ea',  U'\x010eb',  U'\x010ec',  U'\x010ed',  U'\x010ee',
 U'\x010ef',  U'\x010f0',  U'\x010f1',  U'\x010f2',  U'\x010f3',  U'\x010f4',  U'\x010f5',
 U'\x010f6',  U'\x010f7',  U'\x010f8',  U'\x010f9',  U'\x010fa',  U'\x00000',  U'\x00000',
 U'\x010fd',  U'\x010fe',  U'\x010ff'
};

static const char32_t FOLDCASE_0x0001e[] =
{
 U'\x000ff',
 U'\x01e01',  U'\x00000',  U'\x01e03',  U'\x00000',  U'\x01e05',  U'\x00000',  U'\x01e07',
 U'\x00000',  U'\x01e09',  U'\x00000',  U'\x01e0b',  U'\x00000',  U'\x01e0d',  U'\x00000',
 U'\x01e0f',  U'\x00000',  U'\x01e11',  U'\x00000',  U'\x01e13',  U'\x00000',  U'\x01e15',
 U'\x00000',  U'\x01e17',  U'\x00000',  U'\x01e19',  U'\x00000',  U'\x01e1b',  U'\x00000',
 U'\x01e1d',  U'\x00000',  U'\x01e1f',  U'\x00000',  U'\x01e21',  U'\x00000',  U'\x01e23',
 U'\x00000',  U'\x01e25',  U'\x00000',  U'\x01e27',  U'\x00000',  U'\x01e29',  U'\x00000',
 U'\x01e2b',  U'\x00000',  U'\x01e2d',  U'\x00000',  U'\x01e2f',  U'\x00000',  U'\x01e31',
 U'\x00000',  U'\x01e33',  U'\x00000',  U'\x01e35',  U'\x00000',  U'\x01e37',  U'\x00000',
 U'\x01e39',  U'\x00000',  U'\x01e3b',  U'\x00000',  U'\x01e3d',  U'\x00000',  U'\x01e3f',
 U'\x00000',  U'\x01e41',  U'\x00000',  U'\x01e43',  U'\x00000',  U'\x01e45',  U'\x00000',
 U'\x01e47',  U'\x00000',  U'\x01e49',  U'\x00000',  U'\x01e4b',  U'\x00000',  U'\x01e4d',
 U'\x00000',  U'\x01e4f',  U'\x00000',  U'\x01e51',  U'\x00000',  U'\x01e53',  U'\x00000',
 U'\x01e55',  U'\x00000',  U'\x01e57',  U'\x00000',  U'\x01e59',  U'\x00000',  U'\x01e5b',
 U'\x00000',  U'\x01e5d',  U'\x00000',  U'\x01e5f',  U'\x00000',  U'\x01e61',  U'\x00000',
 U'\x01e63',  U'\x00000',  U'\x01e65',  U'\x00000',  U'\x01e67',  U'\x00000',  U'\x01e69',
 U'\x00000',  U'\x01e6b',  U'\x00000',  U'\x01e6d',  U'\x00000',  U'\x01e6f',  U'\x00000',
 U'\x01e71',  U'\x00000',  U'\x01e73',  U'\x00000',  U'\x01e75',  U'\x00000',  U'\x01e77',
 U'\x00000',  U'\x01e79',  U'\x00000',  U'\x01e7b',  U'\x00000',  U'\x01e7d',  U'\x00000',
 U'\x01e7f',  U'\x00000',  U'\x01e81',  U'\x00000',  U'\x01e83',  U'\x00000',  U'\x01e85',
 U'\x00000',  U'\x01e87',  U'\x00000',  U'\x01e89',  U'\x00000',  U'\x01e8b',  U'\x00000',
 U'\x01e8d',  U'\x00000',  U'\x01e8f',  U'\x00000',  U'\x01e91',  U'\x00000',  U'\x01e93',
 U'\x00000',  U'\x01e95',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x01e61',  U'\x00000',  U'\x00000',  U'\x000df',  U'\x00000',  U'\x01ea1',
 U'\x00000',  U'\x01ea3',  U'\x00000',  U'\x01ea5',  U'\x00000',  U'\x01ea7',  U'\x00000',
 U'\x01ea9',  U'\x00000',  U'\x01eab',  U'\x00000',  U'\x01ead',  U'\x00000',  U'\x01eaf',
 U'\x00000',  U'\x01eb1',  U'\x00000',  U'\x01eb3',  U'\x00000',  U'\x01eb5',  U'\x00000',
 U'\x01eb7',  U'\x00000',  U'\x01eb9',  U'\x00000',  U'\x01ebb',  U'\x00000',  U'\x01ebd',
 U'\x00000',  U'\x01ebf',  U'\x00000',  U'\x01ec1',  U'\x00000',  U'\x01ec3',  U'\x00000',
 U'\x01ec5',  U'\x00000',  U'\x01ec7',  U'\x00000',  U'\x01ec9',  U'\x00000',  U'\x01ecb',
 U'\x00000',  U'\x01ecd',  U'\x00000',  U'\x01ecf',  U'\x00000',  U'\x01ed1',  U'\x00000',
 U'\x01ed3',  U'\x00000',  U'\x01ed5',  U'\x00000',  U'\x01ed7',  U'\x00000',  U'\x01ed9',
 U'\x00000',  U'\x01edb',  U'\x00000',  U'\x01edd',  U'\x00000',  U'\x01edf',  U'\x00000',
 U'\x01ee1',  U'\x00000',  U'\x01ee3',  U'\x00000',  U'\x01ee5',  U'\x00000',  U'\x01ee7',
 U'\x00000',  U'\x01ee9',  U'\x00000',  U'\x01eeb',  U'\x00000',  U'\x01eed',  U'\x00000',
 U'\x01eef',  U'\x00000',  U'\x01ef1',  U'\x00000',  U'\x01ef3',  U'\x00000',  U'\x01ef5',
 U'\x00000',  U'\x01ef7',  U'\x00000',  U'\x01ef9',  U'\x00000',  U'\x01efb',  U'\x00000',
 U'\x01efd',  U'\x00000',  U'\x01eff'
};

static const char32_t FOLDCASE_0x0001f[] =
{
 U'\x000fd',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x01f00',  U'\x01f01',  U'\x01f02',  U'\x01f03',  U'\x01f04',  U'\x01f05',
 U'\x01f06',  U'\x01f07',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f10',  U'\x01f11',  U'\x01f12',  U'\x01f13',
 U'\x01f14',  U'\x01f15',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f20',  U'\x01f21',
 U'\x01f22',  U'\x01f23',  U'\x01f24',  U'\x01f25',  U'\x01f26',  U'\x01f27',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x01f30',  U'\x01f31',  U'\x01f32',  U'\x01f33',  U'\x01f34',  U'\x01f35',  U'\x01f36',
 U'\x01f37',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x01f40',  U'\x01f41',  U'\x01f42',  U'\x01f43',  U'\x01f44',
 U'\x01f45',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f51',  U'\x00000',
 U'\x01f53',  U'\x00000',  U'\x01f55',  U'\x00000',  U'\x01f57',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f60',
 U'\x01f61',  U'\x01f62',  U'\x01f63',  U'\x01f64',  U'\x01f65',  U'\x01f66',  U'\x01f67',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f80',  U'\x01f81',  U'\x01f82',  U'\x01f83',
 U'\x01f84',  U'\x01f85',  U'\x01f86',  U'\x01f87',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f90',  U'\x01f91',
 U'\x01f92',  U'\x01f93',  U'\x01f94',  U'\x01f95',  U'\x01f96',  U'\x01f97',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x01fa0',  U'\x01fa1',  U'\x01fa2',  U'\x01fa3',  U'\x01fa4',  U'\x01fa5',  U'\x01fa6',
 U'\x01fa7',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x01fb0',  U'\x01fb1',  U'\x01f70',  U'\x01f71',  U'\x01fb3',
 U'\x00000',  U'\x003b9',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f72',  U'\x01f73',  U'\x01f74',
 U'\x01f75',  U'\x01fc3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01fd0',
 U'\x01fd1',  U'\x01f76',  U'\x01f77',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x01fe0',  U'\x01fe1',  U'\x01f7a',  U'\x01f7b',  U'\x01fe5',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f78',  U'\x01f79',  U'\x01f7c',  U'\x01f7d',
 U'\x01ff3'
};

static const char32_t FOLDCASE_0x00021[] =
{
 U'\x00084',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x003c9',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x0006b',  U'\x000e5',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x0214e',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02170',  U'\x02171',
 U'\x02172',  U'\x02173',  U'\x02174',  U'\x02175',  U'\x02176',  U'\x02177',  U'\x02178',
 U'\x02179',  U'\x0217a',  U'\x0217b',  U'\x0217c',  U'\x0217d',  U'\x0217e',  U'\x0217f',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02184'
};

static const char32_t FOLDCASE_0x00024[] =
{
 U'\x000d0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x024d0',  U'\x024d1',  U'\x024d2',  U'\x024d3',  U'\x024d4',  U'\x024d5',  U'\x024d6',
 U'\x024d7',  U'\x024d8',  U'\x024d9',  U'\x024da',  U'\x024db',  U'\x024dc',  U'\x024dd',
 U'\x024de',  U'\x024df',  U'\x024e0',  U'\x024e1',  U'\x024e2',  U'\x024e3',  U'\x024e4',
 U'\x024e5',  U'\x024e6',  U'\x024e7',  U'\x024e8',  U'\x024e9'
};

static const char32_t FOLDCASE_0x0002c[] =
{
 U'\x000f3',
 U'\x02c30',  U'\x02c31',  U'\x02c32',  U'\x02c33',  U'\x02c34',  U'\x02c35',  U'\x02c36',
 U'\x02c37',  U'\x02c38',  U'\x02c39',  U'\x02c3a',  U'\x02c3b',  U'\x02c3c',  U'\x02c3d',
 U'\x02c3e',  U'\x02c3f',  U'\x02c40',  U'\x02c41',  U'\x02c42',  U'\x02c43',  U'\x02c44',
 U'\x02c45',  U'\x02c46',  U'\x02c47',  U'\x02c48',  U'\x02c49',  U'\x02c4a',  U'\x02c4b',
 U'\x02c4c',  U'\x02c4d',  U'\x02c4e',  U'\x02c4f',  U'\x02c50',  U'\x02c51',  U'\x02c52',
 U'\x02c53',  U'\x02c54',  U'\x02c55',  U'\x02c56',  U'\x02c57',  U'\x02c58',  U'\x02c59',
 U'\x02c5a',  U'\x02c5b',  U'\x02c5c',  U'\x02c5d',  U'\x02c5e',  U'\x02c5f',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c61',  U'\x00000',
 U'\x0026b',  U'\x01d7d',  U'\x0027d',  U'\x00000',  U'\x00000',  U'\x02c68',  U'\x00000',
 U'\x02c6a',  U'\x00000',  U'\x02c6c',  U'\x00000',  U'\x00251',  U'\x00271',  U'\x00250',
 U'\x00252',  U'\x00000',  U'\x02c73',  U'\x00000',  U'\x00000',  U'\x02c76',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x0023f',  U'\x00240',  U'\x02c81',  U'\x00000',  U'\x02c83',  U'\x00000',  U'\x02c85',
 U'\x00000',  U'\x02c87',  U'\x00000',  U'\x02c89',  U'\x00000',  U'\x02c8b',  U'\x00000',
 U'\x02c8d',  U'\x00000',  U'\x02c8f',  U'\x00000',  U'\x02c91',  U'\x00000',  U'\x02c93',
 U'\x00000',  U'\x02c95',  U'\x00000',  U'\x02c97',  U'\x00000',  U'\x02c99',  U'\x00000',
 U'\x02c9b',  U'\x00000',  U'\x02c9d',  U'\x00000',  U'\x02c9f',  U'\x00000',  U'\x02ca1',
 U'\x00000',  U'\x02ca3',  U'\x00000',  U'\x02ca5',  U'\x00000',  U'\x02ca7',  U'\x00000',
 U'\x02ca9',  U'\x00000',  U'\x02cab',  U'\x00000',  U'\x02cad',  U'\x00000',  U'\x02caf',
 U'\x00000',  U'\x02cb1',  U'\x00000',  U'\x02cb3',  U'\x00000',  U'\x02cb5',  U'\x00000',
 U'\x02cb7',  U'\x00000',  U'\x02cb9',  U'\x00000',  U'\x02cbb',  U'\x00000',  U'\x02cbd',
 U'\x00000',  U'\x02cbf',  U'\x00000',  U'\x02cc1',  U'\x00000',  U'\x02cc3',  U'\x00000',
 U'\x02cc5',  U'\x00000',  U'\x02cc7',  U'\x00000',  U'\x02cc9',  U'\x00000',  U'\x02ccb',
 U'\x00000',  U'\x02ccd',  U'\x00000',  U'\x02ccf',  U'\x00000',  U'\x02cd1',  U'\x00000',
 U'\x02cd3',  U'\x00000',  U'\x02cd5',  U'\x00000',  U'\x02cd7',  U'\x00000',  U'\x02cd9',
 U'\x00000',  U'\x02cdb',  U'\x00000',  U'\x02cdd',  U'\x00000',  U'\x02cdf',  U'\x00000',
 U'\x02ce1',  U'\x00000',  U'\x02ce3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02cec',  U'\x00000',  U'\x02cee',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02cf3'
};

static const char32_t FOLDCASE_0x000a6[] =
{
 U'\x0009b',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x0a641',  U'\x00000',  U'\x0a643',  U'\x00000',  U'\x0a645',  U'\x00000',
 U'\x0a647',  U'\x00000',  U'\x0a649',  U'\x00000',  U'\x0a64b',  U'\x00000',  U'\x0a64d',
 U'\x00000',  U'\x0a64f',  U'\x00000',  U'\x0a651',  U'\x00000',  U'\x0a653',  U'\x00000',
 U'\x0a655',  U'\x00000',  U'\x0a657',  U'\x00000',  U'\x0a659',  U'\x00000',  U'\x0a65b',
 U'\x00000',  U'\x0a65d',  U'\x00000',  U'\x0a65f',  U'\x00000',  U'\x0a661',  U'\x00000',
 U'\x0a663',  U'\x00000',  U'\x0a665',  U'\x00000',  U'\x0a667',  U'\x00000',  U'\x0a669',
 U'\x00000',  U'\x0a66b',  U'\x00000',  U'\x0a66d',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x0a681',  U'\x00000',  U'\x0a683',  U'\x00000',  U'\x0a685',
 U'\x00000',  U'\x0a687',  U'\x00000',  U'\x0a689',  U'\x00000',  U'\x0a68b',  U'\x00000',
 U'\x0a68d',  U'\x00000',  U'\x0a68f',  U'\x00000',  U'\x0a691',  U'\x00000',  U'\x0a693',
 U'\x00000',  U'\x0a695',  U'\x00000',  U'\x0a697',  U'\x00000',  U'\x0a699',  U'\x00000',
 U'\x0a69b'
};

static const char32_t FOLDCASE_0x000a7[] =
{
 U'\x000f6',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a723',
 U'\x00000',  U'\x0a725',  U'\x00000',  U'\x0a727',  U'\x00000',  U'\x0a729',  U'\x00000',
 U'\x0a72b',  U'\x00000',  U'\x0a72d',  U'\x00000',  U'\x0a72f',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x0a733',  U'\x00000',  U'\x0a735',  U'\x00000',  U'\x0a737',  U'\x00000',
 U'\x0a739',  U'\x00000',  U'\x0a73b',  U'\x00000',  U'\x0a73d',  U'\x00000',  U'\x0a73f',
 U'\x00000',  U'\x0a741',  U'\x00000',  U'\x0a743',  U'\x00000',  U'\x0a745',  U'\x00000',
 U'\x0a747',  U'\x00000',  U'\x0a749',  U'\x00000',  U'\x0a74b',  U'\x00000',  U'\x0a74d',
 U'\x00000',  U'\x0a74f',  U'\x00000',  U'\x0a751',  U'\x00000',  U'\x0a753',  U'\x00000',
 U'\x0a755',  U'\x00000',  U'\x0a757',  U'\x00000',  U'\x0a759',  U'\x00000',  U'\x0a75b',
 U'\x00000',  U'\x0a75d',  U'\x00000',  U'\x0a75f',  U'\x00000',  U'\x0a761',  U'\x00000',
 U'\x0a763',  U'\x00000',  U'\x0a765',  U'\x00000',  U'\x0a767',  U'\x00000',  U'\x0a769',
 U'\x00000',  U'\x0a76b',  U'\x00000',  U'\x0a76d',  U'\x00000',  U'\x0a76f',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x0a77a',  U'\x00000',  U'\x0a77c',  U'\x00000',  U'\x01d79',
 U'\x0a77f',  U'\x00000',  U'\x0a781',  U'\x00000',  U'\x0a783',  U'\x00000',  U'\x0a785',
 U'\x00000',  U'\x0a787',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a78c',
 U'\x00000',  U'\x00265',  U'\x00000',  U'\x00000',  U'\x0a791',  U'\x00000',  U'\x0a793',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a797',  U'\x00000',  U'\x0a799',  U'\x00000',
 U'\x0a79b',  U'\x00000',  U'\x0a79d',  U'\x00000',  U'\x0a79f',  U'\x00000',  U'\x0a7a1',
 U'\x00000',  U'\x0a7a3',  U'\x00000',  U'\x0a7a5',  U'\x00000',  U'\x0a7a7',  U'\x00000',
 U'\x0a7a9',  U'\x00000',  U'\x00266',  U'\x0025c',  U'\x00261',  U'\x0026c',  U'\x0026a',
 U'\x00000',  U'\x0029e',  U'\x00287',  U'\x0029d',  U'\x0ab53',  U'\x0a7b5',  U'\x00000',
 U'\x0a7b7',  U'\x00000',  U'\x0a7b9',  U'\x00000',  U'\x0a7bb',  U'\x00000',  U'\x0a7bd',
 U'\x00000',  U'\x0a7bf',  U'\x00000',  U'\x0a7c1',  U'\x00000',  U'\x0a7c3',  U'\x00000',
 U'\x0a794',  U'\x00282',  U'\x01d8e',  U'\x0a7c8',  U'\x00000',  U'\x0a7ca',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d1',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d7',  U'\x00000',  U'\x0a7d9',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x0a7f6'
};

static const char32_t FOLDCASE_0x000ab[] =
{
 U'\x000c0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x013a0',  U'\x013a1',  U'\x013a2',  U'\x013a3',  U'\x013a4',  U'\x013a5',  U'\x013a6',
 U'\x013a7',  U'\x013a8',  U'\x013a9',  U'\x013aa',  U'\x013ab',  U'\x013ac',  U'\x013ad',
 U'\x013ae',  U'\x013af',  U'\x013b0',  U'\x013b1',  U'\x013b2',  U'\x013b3',  U'\x013b4',
 U'\x013b5',  U'\x013b6',  U'\x013b7',  U'\x013b8',  U'\x013b9',  U'\x013ba',  U'\x013bb',
 U'\x013bc',  U'\x013bd',  U'\x013be',  U'\x013bf',  U'\x013c0',  U'\x013c1',  U'\x013c2',
 U'\x013c3',  U'\x013c4',  U'\x013c5',  U'\x013c6',  U'\x013c7',  U'\x013c8',  U'\x013c9',
 U'\x013ca',  U'\x013cb',  U'\x013cc',  U'\x013cd',  U'\x013ce',  U'\x013cf',  U'\x013d0',
 U'\x013d1',  U'\x013d2',  U'\x013d3',  U'\x013d4',  U'\x013d5',  U'\x013d6',  U'\x013d7',
 U'\x013d8',  U'\x013d9',  U'\x013da',  U'\x013db',  U'\x013dc',  U'\x013dd',  U'\x013de',
 U'\x013df',  U'\x013e0',  U'\x013e1',  U'\x013e2',  U'\x013e3',  U'\x013e4',  U'\x013e5',
 U'\x013e6',  U'\x013e7',  U'\x013e8',  U'\x013e9',  U'\x013ea',  U'\x013eb',  U'\x013ec',
 U'\x013ed',  U'\x013ee',  U'\x013ef'
};

static const char32_t FOLDCASE_0x000ff[] =
{
 U'\x0003b',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0ff41',  U'\x0ff42',
 U'\x0ff43',  U'\x0ff44',  U'\x0ff45',  U'\x0ff46',  U'\x0ff47',  U'\x0ff48',  U'\x0ff49',
 U'\x0ff4a',  U'\x0ff4b',  U'\x0ff4c',  U'\x0ff4d',  U'\x0ff4e',  U'\x0ff4f',  U'\x0ff50',
 U'\x0ff51',  U'\x0ff52',  U'\x0ff53',  U'\x0ff54',  U'\x0ff55',  U'\x0ff56',  U'\x0ff57',
 U'\x0ff58',  U'\x0ff59',  U'\x0ff5a'
};

static const char32_t FOLDCASE_0x00104[] =
{
 U'\x000d4',
 U'\x10428',  U'\x10429',  U'\x1042a',  U'\x1042b',  U'\x1042c',  U'\x1042d',  U'\x1042e',
 U'\x1042f',  U'\x10430',  U'\x10431',  U'\x10432',  U'\x10433',  U'\x10434',  U'\x10435',
 U'\x10436',  U'\x10437',  U'\x10438',  U'\x10439',  U'\x1043a',  U'\x1043b',  U'\x1043c',
 U'\x1043d',  U'\x1043e',  U'\x1043f',  U'\x10440',  U'\x10441',  U'\x10442',  U'\x10443',
 U'\x10444',  U'\x10445',  U'\x10446',  U'\x10447',  U'\x10448',  U'\x10449',  U'\x1044a',
 U'\x1044b',  U'\x1044c',  U'\x1044d',  U'\x1044e',  U'\x1044f',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x104d8',  U'\x104d9',  U'\x104da',  U'\x104db',  U'\x104dc',  U'\x104dd',
 U'\x104de',  U'\x104df',  U'\x104e0',  U'\x104e1',  U'\x104e2',  U'\x104e3',  U'\x104e4',
 U'\x104e5',  U'\x104e6',  U'\x104e7',  U'\x104e8',  U'\x104e9',  U'\x104ea',  U'\x104eb',
 U'\x104ec',  U'\x104ed',  U'\x104ee',  U'\x104ef',  U'\x104f0',  U'\x104f1',  U'\x104f2',
 U'\x104f3',  U'\x104f4',  U'\x104f5',  U'\x104f6',  U'\x104f7',  U'\x104f8',  U'\x104f9',
 U'\x104fa',  U'\x104fb'
};

static const char32_t FOLDCASE_0x00105[] =
{
 U'\x00096',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x10597',  U'\x10598',  U'\x10599',  U'\x1059a',  U'\x1059b',  U'\x1059c',  U'\x1059d',
 U'\x1059e',  U'\x1059f',  U'\x105a0',  U'\x105a1',  U'\x00000',  U'\x105a3',  U'\x105a4',
 U'\x105a5',  U'\x105a6',  U'\x105a7',  U'\x105a8',  U'\x105a9',  U'\x105aa',  U'\x105ab',
 U'\x105ac',  U'\x105ad',  U'\x105ae',  U'\x105af',  U'\x105b0',  U'\x105b1',  U'\x00000',
 U'\x105b3',  U'\x105b4',  U'\x105b5',  U'\x105b6',  U'\x105b7',  U'\x105b8',  U'\x105b9',
 U'\x00000',  U'\x105bb',  U'\x105bc'
};

static const char32_t FOLDCASE_0x0010c[] =
{
 U'\x000b3',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x10cc0',  U'\x10cc1',  U'\x10cc2',  U'\x10cc3',  U'\x10cc4',
 U'\x10cc5',  U'\x10cc6',  U'\x10cc7',  U'\x10cc8',  U'\x10cc9',  U'\x10cca',  U'\x10ccb',
 U'\x10ccc',  U'\x10ccd',  U'\x10cce',  U'\x10ccf',  U'\x10cd0',  U'\x10cd1',  U'\x10cd2',
 U'\x10cd3',  U'\x10cd4',  U'\x10cd5',  U'\x10cd6',  U'\x10cd7',  U'\x10cd8',  U'\x10cd9',
 U'\x10cda',  U'\x10cdb',  U'\x10cdc',  U'\x10cdd',  U'\x10cde',  U'\x10cdf',  U'\x10ce0',
 U'\x10ce1',  U'\x10ce2',  U'\x10ce3',  U'\x10ce4',  U'\x10ce5',  U'\x10ce6',  U'\x10ce7',
 U'\x10ce8',  U'\x10ce9',  U'\x10cea',  U'\x10ceb',  U'\x10cec',  U'\x10ced',  U'\x10cee',
 U'\x10cef',  U'\x10cf0',  U'\x10cf1',  U'\x10cf2'
};

static const char32_t FOLDCASE_0x00118[] =
{
 U'\x000c0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x118c0',
 U'\x118c1',  U'\x118c2',  U'\x118c3',  U'\x118c4',  U'\x118c5',  U'\x118c6',  U'\x118c7',
 U'\x118c8',  U'\x118c9',  U'\x118ca',  U'\x118cb',  U'\x118cc',  U'\x118cd',  U'\x118ce',
 U'\x118cf',  U'\x118d0',  U'\x118d1',  U'\x118d2',  U'\x118d3',  U'\x118d4',  U'\x118d5',
 U'\x118d6',  U'\x118d7',  U'\x118d8',  U'\x118d9',  U'\x118da',  U'\x118db',  U'\x118dc',
 U'\x118dd',  U'\x118de',  U'\x118df'
};

static const char32_t FOLDCASE_0x0016e[] =
{
 U'\x00060',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x16e60',  U'\x16e61',  U'\x16e62',  U'\x16e63',  U'\x16e64',  U'\x16e65',
 U'\x16e66',  U'\x16e67',  U'\x16e68',  U'\x16e69',  U'\x16e6a',  U'\x16e6b',  U'\x16e6c',
 U'\x16e6d',  U'\x16e6e',  U'\x16e6f',  U'\x16e70',  U'\x16e71',  U'\x16e72',  U'\x16e73',
 U'\x16e74',  U'\x16e75',  U'\x16e76',  U'\x16e77',  U'\x16e78',  U'\x16e79',  U'\x16e7a',
 U'\x16e7b',  U'\x16e7c',  U'\x16e7d',  U'\x16e7e',  U'\x16e7f'
};

static const char32_t FOLDCASE_0x001e9[] =
{
 U'\x00022',
 U'\x1e922',  U'\x1e923',  U'\x1e924',  U'\x1e925',  U'\x1e926',  U'\x1e927',  U'\x1e928',
 U'\x1e929',  U'\x1e92a',  U'\x1e92b',  U'\x1e92c',  U'\x1e92d',  U'\x1e92e',  U'\x1e92f',
 U'\x1e930',  U'\x1e931',  U'\x1e932',  U'\x1e933',  U'\x1e934',  U'\x1e935',  U'\x1e936',
 U'\x1e937',  U'\x1e938',  U'\x1e939',  U'\x1e93a',  U'\x1e93b',  U'\x1e93c',  U'\x1e93d',
 U'\x1e93e',  U'\x1e93f',  U'\x1e940',  U'\x1e941',  U'\x1e942',  U'\x1e943'
};

static const char32_t* FOLDCASE_INDEX [] =
{
 FOLDCASE_0x00000,  FOLDCASE_0x00001,  FOLDCASE_0x00002,  FOLDCASE_0x00003,  FOLDCASE_0x00004,  FOLDCASE_0x00005,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x00010,  0x0,
 0x0,  FOLDCASE_0x00013,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x0001c,  0x0,
 FOLDCASE_0x0001e,  FOLDCASE_0x0001f,  0x0,  FOLDCASE_0x00021,  0x0,  0x0,
 FOLDCASE_0x00024,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  FOLDCASE_0x0002c,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x000a6,  FOLDCASE_0x000a7,
 0x0,  0x0,  0x0,  FOLDCASE_0x000ab,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  FOLDCASE_0x000ff,  0x0,  0x0,
 0x0,  0x0,  FOLDCASE_0x00104,  FOLDCASE_0x00105,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x0010c,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x00118,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 FOLDCASE_0x0016e,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  FOLDCASE_0x001e9
};

// clang-format on

// clang-format off

// TODO: These tables to be removed after wstring versions of ToUpper/ToLower no
// longer depend upon them (once foldcase changes complete).

static constexpr wchar_t unicode_lowers[] = {
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

std::string ToHex(const std::wstring_view in);

std::string ToHex(const std::u32string_view in);

} // namespace

std::string StringUtils::FormatV(const char *fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return "";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  while (true)
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

std::wstring StringUtils::FormatV(const wchar_t *fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return L"";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  while (true)
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

//
// --------------  Unicode encoding converters --------------
//
// There are two ways to convert between the Unicode encodings:
// iconv or C++ wstring_convert
//
// iconv
//   - handles all of the conversions that we need
//   - on error can either ignore bad code-units or truncate
//     converted string
//   - could modify CharsetConverter to substitute bad code-units
//     with 'replacement character' UxFFFD, which would preserve
//     more of the string
//   - likely more consistent behavior than wstring_convert
//   - CharsetConverter is not available for use before StringUtils
//     is called. Either need means to detect when CharsetConverter
//     is ready, or need to initialize it sooner.
//
// wstring_convert
//   - c++ built-in
//   - does not support wstring to/from char32_t
//   - on error can either throw exception or return 'bad value' string
//   - some conversions are in 'limbo': deprecated, but no
//     replacement defined.
//

std::string StringUtils::ToUtf8(const std::u32string_view str)
{
  bool useIconv = g_charsetConverter.isInitialized();
  if (useIconv)
  {
    std::u32string strTmp(str);
    std::string utf8Str;
    g_charsetConverter.utf32ToUtf8(strTmp, utf8Str, FAIL_ON_ERROR); // omit bad chars
    return utf8Str;
  }
  else
  {
    // Alternative to iconv, which may not be initialized prior to
    // the need for it here.

    // No workaround to codecvt_utf8 destructor needed, as is case for
    // std::codecvt

    // A Conversion error results in std::range_error exception,
    // but a number of unassigned, etc. codepoints don't throw exception.
    // Mileage may vary by platform.

    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    std::string result;
    try
    {
      result = conv.to_bytes(str.data(), str.data() + str.length());
    }
    catch ( std::range_error& e )
    {
      std::string asHex = ToHex(str);
      CLog::Log(LOGWARNING, "Conversion error converting from u32string to utf8: {}", asHex);
      result = CONVERSION_ERROR; // It is difficult to salvage what does convert
    }
    return result;
  }
}

std::string StringUtils::ToUtf8(const std::wstring_view str)
{
  bool useIconv = g_charsetConverter.isInitialized();
  if (useIconv)
  {
    const std::wstring strTmp(str);
    std::string utf8Str;
    g_charsetConverter.wToUTF8(strTmp, utf8Str, FAIL_ON_ERROR); // Omit bad chars
    return utf8Str;
  }
  else
  {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::string result;
    try
    {
      // A Conversion error results in std::range_error exception,
      // but a number of unassigned, etc. codepoints don't throw exception.
      // Mileage may vary by platform.

      result = conv.to_bytes(str.data(), str.data() + str.length());
    }
    catch ( std::range_error& e )
    {
      CLog::Log(LOGWARNING, "Conversion error converting from wstring to utf8: {}", ToHex(str));
      result = CONVERSION_ERROR; // It is difficult to salvage what does convert
    }
    return result;
  }
}

std::u32string StringUtils::ToUtf32(const std::string_view str)
{
  bool useIconv = g_charsetConverter.isInitialized();
  if (useIconv)
  {
    const std::string strTmp(str);
    std::u32string utf32Str;
    g_charsetConverter.utf8ToUtf32(strTmp, utf32Str, FAIL_ON_ERROR); // Omit bad chars
    return utf32Str;
  }
  else
  {
    // Alternative to iconv
    // There are some string operations that occur prior to iconv being initialized.
    // One such case is CPosixTimezone::CPosixTimezone which calls StringUtils::sortstringbyname

    // Will NOT compile on C++20, but no simple replacement, given that CharsetConverter
    // may not be initialized. Could call iconv directly.

    struct destructible_codecvt : public std::codecvt<char32_t, char, std::mbstate_t> {
      using std::codecvt<char32_t, char, std::mbstate_t>::codecvt;
      ~destructible_codecvt() = default;
    };
    std::wstring_convert<destructible_codecvt, char32_t> utf32_converter;
    std::u32string result;
    try
    {
      // A Conversion error results in std::range_error exception,
      // but a number of unassigned, etc. codepoints don't throw exception.
      // Mileage may vary by platform.
      result = utf32_converter.from_bytes(str.data(), str.data() + str.length());
    }
    catch ( std::range_error& e )
    {
      CLog::Log(LOGWARNING, "Conversion error converting from string to u32string: {}", str);
      result = U32_CONVERSION_ERROR; // It is difficult to salvage what does convert
    }
    return result;
  }
}

std::u32string StringUtils::ToUtf32(const std::wstring_view str)
{
  const std::wstring strTmp(str);
  std::u32string utf32Str;
  g_charsetConverter.wToUtf32(strTmp, utf32Str, FAIL_ON_ERROR); // Omit bad chars
  return utf32Str;
}

std::wstring StringUtils::ToWString(const std::string_view str)
{
  bool useIconv = g_charsetConverter.isInitialized();
  if (useIconv)
  {
    const std::string strTmp(str);
    std::wstring wStr;
    g_charsetConverter.utf8ToW(strTmp, wStr, FAIL_ON_ERROR); // Omit bad chars
    return wStr;
  }
  else
  {
    // Alternative to iconv, which may not be initialized prior to the need
    // here.

    // Note that unlike codecvt, codecvt_utf8 has public destructor,
    // so no special code needed.

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv; // (CONVERSION_ERROR, WIDE_CONVERSION_ERROR);
    std::wstring result;
    try
    {
      // A Conversion error results in std::range_error exception,
      // but a number of unassigned, etc. codepoints don't throw exception.
      // Mileage may vary by platform.

      result = conv.from_bytes(str.data(), str.data() + str.length());
    }
    catch ( std::range_error& e )
    {
      CLog::Log(LOGWARNING, "Conversion error converting from utf8 to wstring: {}", str);
      result = WIDE_CONVERSION_ERROR;
    }
    return result;
  }
}

std::wstring StringUtils::ToWString(const std::u32string_view str)
{
  const std::u32string strTmp(str);
  std::wstring wStr;
  g_charsetConverter.utf32ToW(strTmp, wStr, FAIL_ON_ERROR); // Omit bad chars
  return wStr;
}

namespace
{
// TODO: Move FoldCaseChar to Anonymous namespace

/*!
 * \brief Folds the case of the given Unicode (32-bit) character
 *
 * Performs "simple" case folding using data from Unicode Inc.'s ICUC4 (License near top of file).
 * Read the description preceding the tables (FOLDCASE_0000) or
 * from the API documentation for FoldCase.
 */
static constexpr char32_t MAX_FOLD_HIGH_BYTES = (sizeof(FOLDCASE_INDEX) / sizeof(char32_t *)) - 1;

inline char32_t FoldCaseChar(const char32_t c)
{
  const char32_t high_bytes = c >> 8; // DF  -> 0
  if (high_bytes > MAX_FOLD_HIGH_BYTES)
    return c;

  const char32_t* table = FOLDCASE_INDEX[high_bytes];
  if (table == 0)
    return c;

  const uint16_t low_byte = c & 0xFF; // DF
  // First table entry is # entries.
  // Entries are in table[1...n + 1], NOT [0..n]
  if (low_byte >= table[0])
    return c;

  const char32_t foldedChar = table[low_byte + 1];
  if (foldedChar == 0)
    return c;

  return foldedChar;
}

int compareWchar (const void* a, const void* b)
{
  if (*(const wchar_t*)a < *(const wchar_t*)b)
    return -1;
  else if (*(const wchar_t*)a > *(const wchar_t*)b)
    return 1;
  return 0;
}

// TODO: To be removed when ToLower(wstring) is reworked (after CaseFold)

wchar_t tolowerUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_uppers, sizeof(unicode_uppers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_lowers + (p - unicode_uppers));

  return c;
}

// TODO: To be removed when ToLower(wstring) is reworked (after CaseFold)

wchar_t toupperUnicode(const wchar_t& c)
{
  wchar_t* p = (wchar_t*) bsearch (&c, unicode_lowers, sizeof(unicode_lowers) / sizeof(wchar_t), sizeof(wchar_t), compareWchar);
  if (p)
    return *(unicode_uppers + (p - unicode_lowers));

  return c;
}

template<typename Str, typename Fn>
void transformString(const Str& input, Str& output, Fn fn)
{
  std::transform(input.begin(), input.end(), output.begin(), fn);
}
} // namespace

std::string StringUtils::ToUpper(const std::string& str)
{
  std::string result(str.size(), '\0');
  transformString(str, result, ::toupper);
  return result;
}

std::wstring StringUtils::ToUpper(const std::wstring& str)
{
  std::wstring result(str.size(), '\0');
  transformString(str, result, toupperUnicode);
  return result;
}

void StringUtils::ToUpper(std::string &str)
{
  transformString(str, str, ::toupper);
}

void StringUtils::ToUpper(std::wstring &str)
{
  transformString(str, str, toupperUnicode);
}

std::string StringUtils::ToLower(const std::string& str)
{
  std::string result(str.size(), '\0');
  transformString(str, result, ::tolower);
  return result;
}

std::wstring StringUtils::ToLower(const std::wstring& str)
{
  std::wstring result(str.size(), '\0');
  transformString(str, result, tolowerUnicode);
  return result;
}

void StringUtils::ToLower(std::string &str)
{
  transformString(str, str, ::tolower);
}

void StringUtils::ToLower(std::wstring &str)
{
  transformString(str, str, tolowerUnicode);
}

std::u32string StringUtils::FoldCase(const std::u32string_view str)
{
  // Common code to do actual case folding
  //
  // In the multi-lingual world, FoldCase is used instead of ToLower to 'normalize'
  // unique-ids, such as property ids, map keys, etc. In the good-'ol days of ASCII
  // ToLower was fine, but when you have ToLower changing behavior depending upon
  // the current locale, you have a disaster. For the curious, look up "Turkish-I"
  // problem.
  //
  // FoldCase is designed to transform strings so that unimportant differences
  // (letter case, some accents, etc.) are neutralized by monocasing, etc..
  // Full case folding goes further and converts strings into a canonical
  // form (ex: German sharfes-S (looks like a script B) is converted to
  // to "ss").
  //
  // The FoldCase here does not support full case folding, but as long as key ids
  // are not too exotic, then this FoldCase should be fine. (A library, 
  // such as ICUC4 is required for more advanced Folding).
  //
  // Even though Kodi appears to have fairly well behaved unique-ids, it is
  // very easy to create bad ones and they can be hard to detect.
  //  - The "Turkish-I" problem caught everyone by surprise. No one knew
  //    that in a few Turkic locales, changing the case of "i" caused a non-Latin
  //    "i" to appear (I think a "dotless 'i', but maybe a dotted 'I'). The
  //    problem prevented Kodi from starting at all.
  //  - As I write this, there are at least five instances where a translated
  //    value for a device name ("keyboard," etc.) is used as the unique-id.
  //    This was only found by detecting non-ASCII ids AND setting locale to
  //    Russian. The problem might be benign, but a lot of testing or
  //    research is required to be sure.
  //  - Python addons face the same problem. This code does NOT address
  //    that issue.
  //
  // The FoldCase here is based on character data from ICU4C. (The library
  // and data are constantly changing. The current version is from 2021.)
  // The data is in UTF32 format. Since C++ string functions, such as
  // tolower(char) only examines one byte at a time (for UTF8),
  // it is unable to properly process multi-byte characters. Similarly
  // for UTF16, multi-UTF16 code-unit characters are not properly
  // processed (fortunately there are far fewer characters longer than
  // 16-bits than for 8-bit ones). For this reason, both versions of
  // FoldCase (UTF8 and wstring) converts its argument to utf32 to
  // perform the fold and then back to the original UTF8, wstring,
  // etc. after the fold.
  //
  if (str.length() == 0)
    return std::u32string(str);

  // This FoldCase doesn't change string length; more sophisticated libs,
  // such as ICU can.

  std::u32string result;
  for (auto i = str.begin(); i != str.end(); i++)
  {
    char32_t fold_c = FoldCaseChar(*i);
    result.push_back(fold_c);
  }
  return result;
}

std::wstring StringUtils::FoldCase(const std::wstring_view str)
{
  if (str.length() == 0)
    return std::wstring(str);

  // TODO: Consider performance impact and improvements:
  //       1- Creating string_view signatures for CharacterConverter
  //       2- If desperate, use char32_t[] so malloc is not used

  // It may be possible for this to be called prior to g_charsetConverter being initialized.
  // If so, need to seek alternative solution.

  std::u32string_view s32;
  const std::wstring strTmp{str};
  std::u32string utf32Str;
  // g_charsetConverter.wToUtf32(strTmp, utf32Str);
  utf32Str = StringUtils::ToUtf32(str);
  s32 = std::u32string_view(utf32Str);
  std::u32string foldedStr = StringUtils::FoldCase(s32);
  std::wstring result = StringUtils::ToWString(foldedStr);

  return result;
}

std::string StringUtils::FoldCase(const std::string_view str)
{
  // To get same behavior and better accuracy as the wstring version, convert to utf32string.

  std::u32string utf32Str = StringUtils::ToUtf32(str);
  std::u32string foldedStr = StringUtils::FoldCase(utf32Str);
  std::string result = StringUtils::ToUtf8(foldedStr);
  return result;
}

void StringUtils::ToCapitalize(std::string &str)
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
  // before we do the char-by-char comparison, first compare sizes of both strings.
  // This led to a 33% improvement in benchmarking on average. (size() just returns a member of std::string)
  if (str1.size() != str2.size())
    return false;
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

int StringUtils::CompareNoCase(const std::string& str1, const std::string& str2, size_t n /* = 0 */)
{
  return CompareNoCase(str1.c_str(), str2.c_str(), n);
}

int StringUtils::CompareNoCase(const char* s1, const char* s2, size_t n /* = 0 */)
{
  char c2; // we need only one char outside the loop
  size_t index = 0;
  do
  {
    const char c1 = *s1++; // const local variable should help compiler to optimize
    c2 = *s2++;
    index++;
    if (c1 != c2 && ::tolower(c1) != ::tolower(c2)) // This includes the possibility that one of the characters is the null-terminator, which implies a string mismatch.
      return ::tolower(c1) - ::tolower(c2);
  } while (c2 != '\0' &&
           index != n); // At this point, we know c1 == c2, so there's no need to test them both.
  return 0;
}

std::string StringUtils::Left(const std::string &str, size_t count)
{
  count = std::max((size_t)0, std::min(count, str.size()));
  return str.substr(0, count);
}

std::string StringUtils::Mid(const std::string &str, size_t first, size_t count /* = string::npos */)
{
  if (first + count > str.size())
    count = str.size() - first;

  if (first > str.size())
    return std::string();

  assert(first + count <= str.size());

  return str.substr(first, count);
}

std::string StringUtils::Right(const std::string &str, size_t count)
{
  count = std::max((size_t)0, std::min(count, str.size()));
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

namespace {
// hack to check only first byte of UTF-8 character
// without this hack "TrimX" functions failed on Win32 and OS X with UTF-8 strings
static int isspace_c(char c)
{
  return (c & 0x80) == 0 && ::isspace(c);
}
} // namespace

std::string& StringUtils::TrimLeft(std::string &str)
{
  str.erase(str.begin(),
            std::find_if(str.begin(), str.end(), [](char s) { return isspace_c(s) == 0; }));
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
  str.erase(std::find_if(str.rbegin(), str.rend(), [](char s) { return isspace_c(s) == 0; }).base(),
            str.end());
  return str;
}

std::string& StringUtils::TrimRight(std::string &str, const char* const chars)
{
  size_t nidx = str.find_last_not_of(chars);
  str.erase(str.npos == nidx ? 0 : ++nidx);
  return str;
}

int StringUtils::ReturnDigits(const std::string& str)
{
  std::stringstream ss;
  for (const auto& character : str)
  {
    if (isdigit(character))
      ss << character;
  }
  return atoi(ss.str().c_str());
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

int StringUtils::Replace(std::string &str, char oldChar, char newChar)
{
  int replacedChars = 0;
  for (std::string::iterator it = str.begin(); it != str.end(); ++it)
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

  while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos)
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

  while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos)
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

// TODO: Force build. Remove

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

std::vector<std::string> StringUtils::Split(const std::string& input, const std::string& delimiter, unsigned int iMaxStrings)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
  return result;
}

std::vector<std::string> StringUtils::Split(const std::string& input, const char delimiter, size_t iMaxStrings)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
  return result;
}

std::vector<std::string> StringUtils::Split(const std::string& input, const std::vector<std::string>& delimiters)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiters);
  return result;
}

std::vector<std::string> StringUtils::SplitMulti(const std::vector<std::string>& input,
                                                 const std::vector<std::string>& delimiters,
                                                 size_t iMaxStrings /* = 0 */)
{
  if (input.empty())
    return std::vector<std::string>();

  std::vector<std::string> results(input);

  if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
    return results;

  std::vector<std::string> strings1;
  if (iMaxStrings == 0)
  {
    for (size_t di = 0; di < delimiters.size(); di++)
    {
      for (size_t i = 0; i < results.size(); i++)
      {
        std::vector<std::string> substrings = StringUtils::Split(results[i], delimiters[di]);
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      results = strings1;
      strings1.clear();
    }
    return results;
  }

  // Control the number of strings input is split into, keeping the original strings.
  // Note iMaxStrings > input.size()
  int64_t iNew = iMaxStrings - results.size();
  for (size_t di = 0; di < delimiters.size(); di++)
  {
    for (size_t i = 0; i < results.size(); i++)
    {
      if (iNew > 0)
      {
        std::vector<std::string> substrings = StringUtils::Split(results[i], delimiters[di], iNew + 1);
        iNew = iNew - substrings.size() + 1;
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      else
        strings1.push_back(results[i]);
    }
    results = strings1;
    iNew = iMaxStrings - results.size();
    strings1.clear();
    if ((iNew <= 0))
      break;  //Stop trying any more delimiters
  }
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

namespace {
// TODO: Move plane maps  to Anonymous namespace

// Plane maps for MySQL utf8_general_ci (now known as utf8mb3_general_ci) collation
// Derived from https://github.com/MariaDB/server/blob/10.5/strings/ctype-utf8.c

// clang-format off
static const uint16_t plane00[] = {
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
  0x0060, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x039C, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00D7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0053,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00F7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0059
};

static const uint16_t plane01[] = {
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0044, 0x0044,
  0x0110, 0x0110, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0047, 0x0047, 0x0047, 0x0047,
  0x0047, 0x0047, 0x0047, 0x0047, 0x0048, 0x0048, 0x0126, 0x0126, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049,
  0x0049, 0x0049, 0x0132, 0x0132, 0x004A, 0x004A, 0x004B, 0x004B, 0x0138, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x013F,
  0x013F, 0x0141, 0x0141, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x0149, 0x014A, 0x014A, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0152, 0x0152, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053,
  0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0166, 0x0166, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0055, 0x0055, 0x0057, 0x0057, 0x0059, 0x0059, 0x0059, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0053,
  0x0180, 0x0181, 0x0182, 0x0182, 0x0184, 0x0184, 0x0186, 0x0187, 0x0187, 0x0189, 0x018A, 0x018B, 0x018B, 0x018D, 0x018E, 0x018F,
  0x0190, 0x0191, 0x0191, 0x0193, 0x0194, 0x01F6, 0x0196, 0x0197, 0x0198, 0x0198, 0x019A, 0x019B, 0x019C, 0x019D, 0x019E, 0x019F,
  0x004F, 0x004F, 0x01A2, 0x01A2, 0x01A4, 0x01A4, 0x01A6, 0x01A7, 0x01A7, 0x01A9, 0x01AA, 0x01AB, 0x01AC, 0x01AC, 0x01AE, 0x0055,
  0x0055, 0x01B1, 0x01B2, 0x01B3, 0x01B3, 0x01B5, 0x01B5, 0x01B7, 0x01B8, 0x01B8, 0x01BA, 0x01BB, 0x01BC, 0x01BC, 0x01BE, 0x01F7,
  0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C4, 0x01C4, 0x01C4, 0x01C7, 0x01C7, 0x01C7, 0x01CA, 0x01CA, 0x01CA, 0x0041, 0x0041, 0x0049,
  0x0049, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x018E, 0x0041, 0x0041,
  0x0041, 0x0041, 0x00C6, 0x00C6, 0x01E4, 0x01E4, 0x0047, 0x0047, 0x004B, 0x004B, 0x004F, 0x004F, 0x004F, 0x004F, 0x01B7, 0x01B7,
  0x004A, 0x01F1, 0x01F1, 0x01F1, 0x0047, 0x0047, 0x01F6, 0x01F7, 0x004E, 0x004E, 0x0041, 0x0041, 0x00C6, 0x00C6, 0x00D8, 0x00D8
};

static const uint16_t plane02[] = {
  0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x0052, 0x0052, 0x0052, 0x0052, 0x0055, 0x0055, 0x0055, 0x0055, 0x0053, 0x0053, 0x0054, 0x0054, 0x021C, 0x021C, 0x0048, 0x0048,
  0x0220, 0x0221, 0x0222, 0x0222, 0x0224, 0x0224, 0x0041, 0x0041, 0x0045, 0x0045, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0059, 0x0059, 0x0234, 0x0235, 0x0236, 0x0237, 0x0238, 0x0239, 0x023A, 0x023B, 0x023C, 0x023D, 0x023E, 0x023F,
  0x0240, 0x0241, 0x0242, 0x0243, 0x0244, 0x0245, 0x0246, 0x0247, 0x0248, 0x0249, 0x024A, 0x024B, 0x024C, 0x024D, 0x024E, 0x024F,
  0x0250, 0x0251, 0x0252, 0x0181, 0x0186, 0x0255, 0x0189, 0x018A, 0x0258, 0x018F, 0x025A, 0x0190, 0x025C, 0x025D, 0x025E, 0x025F,
  0x0193, 0x0261, 0x0262, 0x0194, 0x0264, 0x0265, 0x0266, 0x0267, 0x0197, 0x0196, 0x026A, 0x026B, 0x026C, 0x026D, 0x026E, 0x019C,
  0x0270, 0x0271, 0x019D, 0x0273, 0x0274, 0x019F, 0x0276, 0x0277, 0x0278, 0x0279, 0x027A, 0x027B, 0x027C, 0x027D, 0x027E, 0x027F,
  0x01A6, 0x0281, 0x0282, 0x01A9, 0x0284, 0x0285, 0x0286, 0x0287, 0x01AE, 0x0289, 0x01B1, 0x01B2, 0x028C, 0x028D, 0x028E, 0x028F,
  0x0290, 0x0291, 0x01B7, 0x0293, 0x0294, 0x0295, 0x0296, 0x0297, 0x0298, 0x0299, 0x029A, 0x029B, 0x029C, 0x029D, 0x029E, 0x029F,
  0x02A0, 0x02A1, 0x02A2, 0x02A3, 0x02A4, 0x02A5, 0x02A6, 0x02A7, 0x02A8, 0x02A9, 0x02AA, 0x02AB, 0x02AC, 0x02AD, 0x02AE, 0x02AF,
  0x02B0, 0x02B1, 0x02B2, 0x02B3, 0x02B4, 0x02B5, 0x02B6, 0x02B7, 0x02B8, 0x02B9, 0x02BA, 0x02BB, 0x02BC, 0x02BD, 0x02BE, 0x02BF,
  0x02C0, 0x02C1, 0x02C2, 0x02C3, 0x02C4, 0x02C5, 0x02C6, 0x02C7, 0x02C8, 0x02C9, 0x02CA, 0x02CB, 0x02CC, 0x02CD, 0x02CE, 0x02CF,
  0x02D0, 0x02D1, 0x02D2, 0x02D3, 0x02D4, 0x02D5, 0x02D6, 0x02D7, 0x02D8, 0x02D9, 0x02DA, 0x02DB, 0x02DC, 0x02DD, 0x02DE, 0x02DF,
  0x02E0, 0x02E1, 0x02E2, 0x02E3, 0x02E4, 0x02E5, 0x02E6, 0x02E7, 0x02E8, 0x02E9, 0x02EA, 0x02EB, 0x02EC, 0x02ED, 0x02EE, 0x02EF,
  0x02F0, 0x02F1, 0x02F2, 0x02F3, 0x02F4, 0x02F5, 0x02F6, 0x02F7, 0x02F8, 0x02F9, 0x02FA, 0x02FB, 0x02FC, 0x02FD, 0x02FE, 0x02FF
};

static const uint16_t plane03[] = {
  0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x0308, 0x0309, 0x030A, 0x030B, 0x030C, 0x030D, 0x030E, 0x030F,
  0x0310, 0x0311, 0x0312, 0x0313, 0x0314, 0x0315, 0x0316, 0x0317, 0x0318, 0x0319, 0x031A, 0x031B, 0x031C, 0x031D, 0x031E, 0x031F,
  0x0320, 0x0321, 0x0322, 0x0323, 0x0324, 0x0325, 0x0326, 0x0327, 0x0328, 0x0329, 0x032A, 0x032B, 0x032C, 0x032D, 0x032E, 0x032F,
  0x0330, 0x0331, 0x0332, 0x0333, 0x0334, 0x0335, 0x0336, 0x0337, 0x0338, 0x0339, 0x033A, 0x033B, 0x033C, 0x033D, 0x033E, 0x033F,
  0x0340, 0x0341, 0x0342, 0x0343, 0x0344, 0x0399, 0x0346, 0x0347, 0x0348, 0x0349, 0x034A, 0x034B, 0x034C, 0x034D, 0x034E, 0x034F,
  0x0350, 0x0351, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x0357, 0x0358, 0x0359, 0x035A, 0x035B, 0x035C, 0x035D, 0x035E, 0x035F,
  0x0360, 0x0361, 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0367, 0x0368, 0x0369, 0x036A, 0x036B, 0x036C, 0x036D, 0x036E, 0x036F,
  0x0370, 0x0371, 0x0372, 0x0373, 0x0374, 0x0375, 0x0376, 0x0377, 0x0378, 0x0379, 0x037A, 0x037B, 0x037C, 0x037D, 0x037E, 0x037F,
  0x0380, 0x0381, 0x0382, 0x0383, 0x0384, 0x0385, 0x0391, 0x0387, 0x0395, 0x0397, 0x0399, 0x038B, 0x039F, 0x038D, 0x03A5, 0x03A9,
  0x0399, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A2, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x0391, 0x0395, 0x0397, 0x0399,
  0x03A5, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A3, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x039F, 0x03A5, 0x03A9, 0x03CF,
  0x0392, 0x0398, 0x03D2, 0x03D2, 0x03D2, 0x03A6, 0x03A0, 0x03D7, 0x03D8, 0x03D9, 0x03DA, 0x03DA, 0x03DC, 0x03DC, 0x03DE, 0x03DE,
  0x03E0, 0x03E0, 0x03E2, 0x03E2, 0x03E4, 0x03E4, 0x03E6, 0x03E6, 0x03E8, 0x03E8, 0x03EA, 0x03EA, 0x03EC, 0x03EC, 0x03EE, 0x03EE,
  0x039A, 0x03A1, 0x03A3, 0x03F3, 0x03F4, 0x03F5, 0x03F6, 0x03F7, 0x03F8, 0x03F9, 0x03FA, 0x03FB, 0x03FC, 0x03FD, 0x03FE, 0x03FF
};

static const uint16_t plane04[] = {
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0460, 0x0460, 0x0462, 0x0462, 0x0464, 0x0464, 0x0466, 0x0466, 0x0468, 0x0468, 0x046A, 0x046A, 0x046C, 0x046C, 0x046E, 0x046E,
  0x0470, 0x0470, 0x0472, 0x0472, 0x0474, 0x0474, 0x0474, 0x0474, 0x0478, 0x0478, 0x047A, 0x047A, 0x047C, 0x047C, 0x047E, 0x047E,
  0x0480, 0x0480, 0x0482, 0x0483, 0x0484, 0x0485, 0x0486, 0x0487, 0x0488, 0x0489, 0x048A, 0x048B, 0x048C, 0x048C, 0x048E, 0x048E,
  0x0490, 0x0490, 0x0492, 0x0492, 0x0494, 0x0494, 0x0496, 0x0496, 0x0498, 0x0498, 0x049A, 0x049A, 0x049C, 0x049C, 0x049E, 0x049E,
  0x04A0, 0x04A0, 0x04A2, 0x04A2, 0x04A4, 0x04A4, 0x04A6, 0x04A6, 0x04A8, 0x04A8, 0x04AA, 0x04AA, 0x04AC, 0x04AC, 0x04AE, 0x04AE,
  0x04B0, 0x04B0, 0x04B2, 0x04B2, 0x04B4, 0x04B4, 0x04B6, 0x04B6, 0x04B8, 0x04B8, 0x04BA, 0x04BA, 0x04BC, 0x04BC, 0x04BE, 0x04BE,
  0x04C0, 0x0416, 0x0416, 0x04C3, 0x04C3, 0x04C5, 0x04C6, 0x04C7, 0x04C7, 0x04C9, 0x04CA, 0x04CB, 0x04CB, 0x04CD, 0x04CE, 0x04CF,
  0x0410, 0x0410, 0x0410, 0x0410, 0x04D4, 0x04D4, 0x0415, 0x0415, 0x04D8, 0x04D8, 0x04D8, 0x04D8, 0x0416, 0x0416, 0x0417, 0x0417,
  0x04E0, 0x04E0, 0x0418, 0x0418, 0x0418, 0x0418, 0x041E, 0x041E, 0x04E8, 0x04E8, 0x04E8, 0x04E8, 0x042D, 0x042D, 0x0423, 0x0423,
  0x0423, 0x0423, 0x0423, 0x0423, 0x0427, 0x0427, 0x04F6, 0x04F7, 0x042B, 0x042B, 0x04FA, 0x04FB, 0x04FC, 0x04FD, 0x04FE, 0x04FF
};

static const uint16_t plane05[] = {
  0x0500, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506, 0x0507, 0x0508, 0x0509, 0x050A, 0x050B, 0x050C, 0x050D, 0x050E, 0x050F,
  0x0510, 0x0511, 0x0512, 0x0513, 0x0514, 0x0515, 0x0516, 0x0517, 0x0518, 0x0519, 0x051A, 0x051B, 0x051C, 0x051D, 0x051E, 0x051F,
  0x0520, 0x0521, 0x0522, 0x0523, 0x0524, 0x0525, 0x0526, 0x0527, 0x0528, 0x0529, 0x052A, 0x052B, 0x052C, 0x052D, 0x052E, 0x052F,
  0x0530, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0557, 0x0558, 0x0559, 0x055A, 0x055B, 0x055C, 0x055D, 0x055E, 0x055F,
  0x0560, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0587, 0x0588, 0x0589, 0x058A, 0x058B, 0x058C, 0x058D, 0x058E, 0x058F,
  0x0590, 0x0591, 0x0592, 0x0593, 0x0594, 0x0595, 0x0596, 0x0597, 0x0598, 0x0599, 0x059A, 0x059B, 0x059C, 0x059D, 0x059E, 0x059F,
  0x05A0, 0x05A1, 0x05A2, 0x05A3, 0x05A4, 0x05A5, 0x05A6, 0x05A7, 0x05A8, 0x05A9, 0x05AA, 0x05AB, 0x05AC, 0x05AD, 0x05AE, 0x05AF,
  0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7, 0x05B8, 0x05B9, 0x05BA, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
  0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05C4, 0x05C5, 0x05C6, 0x05C7, 0x05C8, 0x05C9, 0x05CA, 0x05CB, 0x05CC, 0x05CD, 0x05CE, 0x05CF,
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x05EB, 0x05EC, 0x05ED, 0x05EE, 0x05EF,
  0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4, 0x05F5, 0x05F6, 0x05F7, 0x05F8, 0x05F9, 0x05FA, 0x05FB, 0x05FC, 0x05FD, 0x05FE, 0x05FF
};

static const uint16_t plane1E[] = {
  0x0041, 0x0041, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0043, 0x0043, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044,
  0x0044, 0x0044, 0x0044, 0x0044, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0046, 0x0046,
  0x0047, 0x0047, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0049, 0x0049, 0x0049, 0x0049,
  0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004D, 0x004D,
  0x004D, 0x004D, 0x004D, 0x004D, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0050, 0x0050, 0x0050, 0x0050, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052,
  0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054,
  0x0054, 0x0054, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0056, 0x0056, 0x0056, 0x0056,
  0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0058, 0x0058, 0x0058, 0x0058, 0x0059, 0x0059,
  0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0048, 0x0054, 0x0057, 0x0059, 0x1E9A, 0x0053, 0x1E9C, 0x1E9D, 0x1E9E, 0x1E9F,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045,
  0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x1EFA, 0x1EFB, 0x1EFC, 0x1EFD, 0x1EFE, 0x1EFF
};

static const uint16_t plane1F[] = {
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F16, 0x1F17, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F1E, 0x1F1F,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399,
  0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F46, 0x1F47, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F4E, 0x1F4F,
  0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1F58, 0x03A5, 0x1F5A, 0x03A5, 0x1F5C, 0x03A5, 0x1F5E, 0x03A5,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x1FBB, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0399, 0x1FDB, 0x039F, 0x1FF9, 0x03A5, 0x1FEB, 0x03A9, 0x1FFB, 0x1F7E, 0x1F7F,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FB5, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FBB, 0x0391, 0x1FBD, 0x0399, 0x1FBF,
  0x1FC0, 0x1FC1, 0x0397, 0x0397, 0x0397, 0x1FC5, 0x0397, 0x0397, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0397, 0x1FCD, 0x1FCE, 0x1FCF,
  0x0399, 0x0399, 0x0399, 0x1FD3, 0x1FD4, 0x1FD5, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x1FDB, 0x1FDC, 0x1FDD, 0x1FDE, 0x1FDF,
  0x03A5, 0x03A5, 0x03A5, 0x1FE3, 0x03A1, 0x03A1, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1FEB, 0x03A1, 0x1FED, 0x1FEE, 0x1FEF,
  0x1FF0, 0x1FF1, 0x03A9, 0x03A9, 0x03A9, 0x1FF5, 0x03A9, 0x03A9, 0x039F, 0x1FF9, 0x03A9, 0x1FFB, 0x03A9, 0x1FFD, 0x1FFE, 0x1FFF
};

static const uint16_t plane21[] = {
  0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2105, 0x2106, 0x2107, 0x2108, 0x2109, 0x210A, 0x210B, 0x210C, 0x210D, 0x210E, 0x210F,
  0x2110, 0x2111, 0x2112, 0x2113, 0x2114, 0x2115, 0x2116, 0x2117, 0x2118, 0x2119, 0x211A, 0x211B, 0x211C, 0x211D, 0x211E, 0x211F,
  0x2120, 0x2121, 0x2122, 0x2123, 0x2124, 0x2125, 0x2126, 0x2127, 0x2128, 0x2129, 0x212A, 0x212B, 0x212C, 0x212D, 0x212E, 0x212F,
  0x2130, 0x2131, 0x2132, 0x2133, 0x2134, 0x2135, 0x2136, 0x2137, 0x2138, 0x2139, 0x213A, 0x213B, 0x213C, 0x213D, 0x213E, 0x213F,
  0x2140, 0x2141, 0x2142, 0x2143, 0x2144, 0x2145, 0x2146, 0x2147, 0x2148, 0x2149, 0x214A, 0x214B, 0x214C, 0x214D, 0x214E, 0x214F,
  0x2150, 0x2151, 0x2152, 0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215A, 0x215B, 0x215C, 0x215D, 0x215E, 0x215F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2180, 0x2181, 0x2182, 0x2183, 0x2184, 0x2185, 0x2186, 0x2187, 0x2188, 0x2189, 0x218A, 0x218B, 0x218C, 0x218D, 0x218E, 0x218F,
  0x2190, 0x2191, 0x2192, 0x2193, 0x2194, 0x2195, 0x2196, 0x2197, 0x2198, 0x2199, 0x219A, 0x219B, 0x219C, 0x219D, 0x219E, 0x219F,
  0x21A0, 0x21A1, 0x21A2, 0x21A3, 0x21A4, 0x21A5, 0x21A6, 0x21A7, 0x21A8, 0x21A9, 0x21AA, 0x21AB, 0x21AC, 0x21AD, 0x21AE, 0x21AF,
  0x21B0, 0x21B1, 0x21B2, 0x21B3, 0x21B4, 0x21B5, 0x21B6, 0x21B7, 0x21B8, 0x21B9, 0x21BA, 0x21BB, 0x21BC, 0x21BD, 0x21BE, 0x21BF,
  0x21C0, 0x21C1, 0x21C2, 0x21C3, 0x21C4, 0x21C5, 0x21C6, 0x21C7, 0x21C8, 0x21C9, 0x21CA, 0x21CB, 0x21CC, 0x21CD, 0x21CE, 0x21CF,
  0x21D0, 0x21D1, 0x21D2, 0x21D3, 0x21D4, 0x21D5, 0x21D6, 0x21D7, 0x21D8, 0x21D9, 0x21DA, 0x21DB, 0x21DC, 0x21DD, 0x21DE, 0x21DF,
  0x21E0, 0x21E1, 0x21E2, 0x21E3, 0x21E4, 0x21E5, 0x21E6, 0x21E7, 0x21E8, 0x21E9, 0x21EA, 0x21EB, 0x21EC, 0x21ED, 0x21EE, 0x21EF,
  0x21F0, 0x21F1, 0x21F2, 0x21F3, 0x21F4, 0x21F5, 0x21F6, 0x21F7, 0x21F8, 0x21F9, 0x21FA, 0x21FB, 0x21FC, 0x21FD, 0x21FE, 0x21FF
};

static const uint16_t plane24[] = {
  0x2400, 0x2401, 0x2402, 0x2403, 0x2404, 0x2405, 0x2406, 0x2407, 0x2408, 0x2409, 0x240A, 0x240B, 0x240C, 0x240D, 0x240E, 0x240F,
  0x2410, 0x2411, 0x2412, 0x2413, 0x2414, 0x2415, 0x2416, 0x2417, 0x2418, 0x2419, 0x241A, 0x241B, 0x241C, 0x241D, 0x241E, 0x241F,
  0x2420, 0x2421, 0x2422, 0x2423, 0x2424, 0x2425, 0x2426, 0x2427, 0x2428, 0x2429, 0x242A, 0x242B, 0x242C, 0x242D, 0x242E, 0x242F,
  0x2430, 0x2431, 0x2432, 0x2433, 0x2434, 0x2435, 0x2436, 0x2437, 0x2438, 0x2439, 0x243A, 0x243B, 0x243C, 0x243D, 0x243E, 0x243F,
  0x2440, 0x2441, 0x2442, 0x2443, 0x2444, 0x2445, 0x2446, 0x2447, 0x2448, 0x2449, 0x244A, 0x244B, 0x244C, 0x244D, 0x244E, 0x244F,
  0x2450, 0x2451, 0x2452, 0x2453, 0x2454, 0x2455, 0x2456, 0x2457, 0x2458, 0x2459, 0x245A, 0x245B, 0x245C, 0x245D, 0x245E, 0x245F,
  0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466, 0x2467, 0x2468, 0x2469, 0x246A, 0x246B, 0x246C, 0x246D, 0x246E, 0x246F,
  0x2470, 0x2471, 0x2472, 0x2473, 0x2474, 0x2475, 0x2476, 0x2477, 0x2478, 0x2479, 0x247A, 0x247B, 0x247C, 0x247D, 0x247E, 0x247F,
  0x2480, 0x2481, 0x2482, 0x2483, 0x2484, 0x2485, 0x2486, 0x2487, 0x2488, 0x2489, 0x248A, 0x248B, 0x248C, 0x248D, 0x248E, 0x248F,
  0x2490, 0x2491, 0x2492, 0x2493, 0x2494, 0x2495, 0x2496, 0x2497, 0x2498, 0x2499, 0x249A, 0x249B, 0x249C, 0x249D, 0x249E, 0x249F,
  0x24A0, 0x24A1, 0x24A2, 0x24A3, 0x24A4, 0x24A5, 0x24A6, 0x24A7, 0x24A8, 0x24A9, 0x24AA, 0x24AB, 0x24AC, 0x24AD, 0x24AE, 0x24AF,
  0x24B0, 0x24B1, 0x24B2, 0x24B3, 0x24B4, 0x24B5, 0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF,
  0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5, 0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF,
  0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF, 0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5,
  0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF, 0x24EA, 0x24EB, 0x24EC, 0x24ED, 0x24EE, 0x24EF,
  0x24F0, 0x24F1, 0x24F2, 0x24F3, 0x24F4, 0x24F5, 0x24F6, 0x24F7, 0x24F8, 0x24F9, 0x24FA, 0x24FB, 0x24FC, 0x24FD, 0x24FE, 0x24FF
};

static const uint16_t planeFF[] = {
  0xFF00, 0xFF01, 0xFF02, 0xFF03, 0xFF04, 0xFF05, 0xFF06, 0xFF07, 0xFF08, 0xFF09, 0xFF0A, 0xFF0B, 0xFF0C, 0xFF0D, 0xFF0E, 0xFF0F,
  0xFF10, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17, 0xFF18, 0xFF19, 0xFF1A, 0xFF1B, 0xFF1C, 0xFF1D, 0xFF1E, 0xFF1F,
  0xFF20, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF3B, 0xFF3C, 0xFF3D, 0xFF3E, 0xFF3F,
  0xFF40, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF5B, 0xFF5C, 0xFF5D, 0xFF5E, 0xFF5F,
  0xFF60, 0xFF61, 0xFF62, 0xFF63, 0xFF64, 0xFF65, 0xFF66, 0xFF67, 0xFF68, 0xFF69, 0xFF6A, 0xFF6B, 0xFF6C, 0xFF6D, 0xFF6E, 0xFF6F,
  0xFF70, 0xFF71, 0xFF72, 0xFF73, 0xFF74, 0xFF75, 0xFF76, 0xFF77, 0xFF78, 0xFF79, 0xFF7A, 0xFF7B, 0xFF7C, 0xFF7D, 0xFF7E, 0xFF7F,
  0xFF80, 0xFF81, 0xFF82, 0xFF83, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F,
  0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E, 0xFF9F,
  0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE, 0xFFAF,
  0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
  0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
  0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF,
  0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF,
  0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF
};

static const uint16_t* const planemap[256] = {
    plane00, plane01, plane02, plane03, plane04, plane05, NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, plane1E, plane1F, NULL,
    plane21, NULL,    NULL,    plane24, NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    planeFF
};
// clang-format on

static wchar_t GetCollationWeight(const wchar_t& r)
{
  // Lookup the "weight" of a UTF8 char, equivalent lowercase ascii letter, in the plane map,
  // the character comparison value used by using "accent folding" collation utf8_general_ci
  // in MySQL (AKA utf8mb3_general_ci in MariaDB 10)
  auto index = r >> 8;
  if (index > 255)
    return 0xFFFD;
  auto plane = planemap[index];
  if (plane == nullptr)
    return r;
  return static_cast<wchar_t>(plane[r & 0xFF]);
}
} // namespace

// Compares separately the numeric and alphabetic parts of a wide string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical.
// See also the equivalent StringUtils::AlphaNumericCollation() for UFT8 data
int64_t StringUtils::AlphaNumericCompare(const wchar_t* left, const wchar_t* right)
{
  const wchar_t *l = left;
  const wchar_t *r = right;
  const wchar_t* ld;
  const wchar_t* rd;
  wchar_t lc;
  wchar_t rc;
  int64_t lnum;
  int64_t rnum;
  bool lsym;
  bool rsym;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= L'0' && *l <= L'9' && *r >= L'0' && *r <= L'9')
    {
      ld = l;
      lnum = *ld++ - L'0';
      while (*ld >= L'0' && *ld <= L'9' && ld < l + 15)
      { // compare only up to 15 digits
        lnum *= 10;
        lnum += *ld++ - L'0';
      }
      rd = r;
      rnum = *rd++ - L'0';
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

    lc = *l;
    rc = *r;
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ above the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (lc >= 32 && lc < L'0') || (lc > L'9' && lc < L'A') || (lc > L'Z' && lc < L'a') ||
           (lc > L'z' && lc < 128);
    rsym = (rc >= 32 && rc < L'0') || (rc > L'9' && rc < L'A') || (rc > L'Z' && rc < L'a') ||
           (rc > L'z' && rc < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (lc != rc)
        return lc - rc;
      else
      { // Same symbol advance to next wchar
        l++;
        r++;
        continue;
      }
    }
    if (!g_langInfo.UseLocaleCollation())
    {
      // Apply case sensitive accent folding collation to non-ascii chars.
      // This mimics utf8_general_ci collation, and provides simple collation of LATIN-1 chars
      // for any platformthat doesn't have a language specific collate facet implemented
      if (lc > 128)
        lc = GetCollationWeight(lc);
      if (rc > 128)
        rc = GetCollationWeight(rc);
    }
    // Do case less comparison, convert ascii upper case to lower case
    if (lc >= L'A' && lc <= L'Z')
      lc += L'a' - L'A';
    if (rc >= L'A' && rc <= L'Z')
      rc += L'a' - L'A';

    if (lc != rc)
    {
      if (!g_langInfo.UseLocaleCollation())
      {
        // Compare unicode (having applied accent folding collation to non-ascii chars).
        int i = wcsncmp(&lc, &rc, 1);
        return i;
      }
      else
      {
        // Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not language specific but just compares unicode
        const std::collate<wchar_t>& coll =
            std::use_facet<std::collate<wchar_t>>(g_langInfo.GetSystemLocale());
        int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
        if (cmp_res != 0)
          return cmp_res;
      }
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

namespace
{
// TODO: Move to Anonymous namespace

/*
  Convert the UTF8 character to which z points into a 31-bit Unicode point.
  Return how many bytes (0 to 3) of UTF8 data encode the character.
  This only works right if z points to a well-formed UTF8 string.
  Byte-0    Byte-1    Byte-2    Byte-3     Value
  0xxxxxxx                                 00000000 00000000 0xxxxxxx
  110yyyyy  10xxxxxx                       00000000 00000yyy yyxxxxxx
  1110zzzz  10yyyyyy  10xxxxxx             00000000 zzzzyyyy yyxxxxxx
  11110uuu  10uuzzzz  10yyyyyy  10xxxxxx   000uuuuu zzzzyyyy yyxxxxxx
*/
static uint32_t UTF8ToUnicode(const unsigned char* z, int nKey, unsigned char& bytes)
{
  // Lookup table used decode the first byte of a multi-byte UTF8 character
  // clang-format off
  static const unsigned char utf8Trans1[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
  };
  // clang-format on

  uint32_t c;
  bytes = 0;
  c = z[0];
  if (c >= 0xc0)
  {
    c = utf8Trans1[c - 0xc0];
    int index = 1;
    while (index < nKey && (z[index] & 0xc0) == 0x80)
    {
      c = (c << 6) + (0x3f & z[index]);
      index++;
    }
    if (c < 0x80 || (c & 0xFFFFF800) == 0xD800 || (c & 0xFFFFFFFE) == 0xFFFE)
      c = 0xFFFD;
    bytes = static_cast<unsigned char>(index - 1);
  }
  return c;
}
} // namespace

/*
  SQLite collating function, see sqlite3_create_collation
  The equivalent of AlphaNumericCompare() but for comparing UTF8 encoded data

  This only processes enough data to find a difference, and avoids expensive data conversions.
  When sorting in memory item data is converted once to wstring in advance prior to sorting, the
  SQLite callback function can not do that kind of preparation. Instead, in order to use
  AlphaNumericCompare(), it would have to repeatedly convert the full input data to wstring for
  every pair comparison made. That approach was found to be 10 times slower than using this
  separate routine.
*/
int StringUtils::AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2)
{
  // Get exact matches of shorter text to start of larger test fast
  int n = std::min(nKey1, nKey2);
  int r = memcmp(pKey1, pKey2, n);
  if (r == 0)
    return nKey1 - nKey2;

  //Not a binary match, so process character at a time
  const unsigned char* zA = static_cast<const unsigned char*>(pKey1);
  const unsigned char* zB = static_cast<const unsigned char*>(pKey2);
  wchar_t lc;
  wchar_t rc;
  unsigned char bytes;
  int64_t lnum;
  int64_t rnum;
  bool lsym;
  bool rsym;
  int ld;
  int rd;
  int i = 0;
  int j = 0;
  // Looping Unicode point at a time through potentially 1 to 4 multi-byte encoded UTF8 data
  while (i < nKey1 && j < nKey2)
  {
    // Check if we have numerical values, compare only up to 15 digits
    if (isdigit(zA[i]) && isdigit(zB[j]))
    {
      lnum = zA[i] - '0';
      ld = i + 1;
      while (ld < nKey1 && isdigit(zA[ld]) && ld < i + 15)
      {
        lnum *= 10;
        lnum += zA[ld] - '0';
        ld++;
      }
      rnum = zB[j] - '0';
      rd = j + 1;
      while (rd < nKey2 && isdigit(zB[rd]) && rd < j + 15)
      {
        rnum *= 10;
        rnum += zB[rd] - '0';
        rd++;
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return static_cast<int>(lnum - rnum);
      }
      // Advance to after digits
      i = ld;
      j = rd;
      continue;
    }
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ before the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (zA[i] >= 32 && zA[i] < '0') || (zA[i] > '9' && zA[i] < 'A') ||
           (zA[i] > 'Z' && zA[i] < 'a') || (zA[i] > 'z' && zA[i] < 128);
    rsym = (zB[j] >= 32 && zB[j] < '0') || (zB[j] > '9' && zB[j] < 'A') ||
           (zB[j] > 'Z' && zB[j] < 'a') || (zB[j] > 'z' && zB[j] < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (zA[i] != zB[j])
        return zA[i] - zB[j];
      else
      { // Same symbol advance to next
        i++;
        j++;
        continue;
      }
    }
    //Decode single (1 to 4 bytes) UTF8 character to Unicode
    lc = UTF8ToUnicode(&zA[i], nKey1 - i, bytes);
    i += bytes;
    rc = UTF8ToUnicode(&zB[j], nKey2 - j, bytes);
    j += bytes;
    if (!g_langInfo.UseLocaleCollation())
    {
      // Apply case sensitive accent folding collation to non-ascii chars.
      // This mimics utf8_general_ci collation, and provides simple collation of LATIN-1 chars
      // for any platform that doesn't have a language specific collate facet implemented
      if (lc > 128)
        lc = GetCollationWeight(lc);
      if (rc > 128)
        rc = GetCollationWeight(rc);
    }
    // Caseless comparison so convert ascii upper case to lower case
    if (lc >= 'A' && lc <= 'Z')
      lc += 'a' - 'A';
    if (rc >= 'A' && rc <= 'Z')
      rc += 'a' - 'A';

    if (lc != rc)
    {
      if (!g_langInfo.UseLocaleCollation() || (lc <= 128 && rc <= 128))
        // Compare unicode (having applied accent folding collation to non-ascii chars).
        return lc - rc;
      else
      {
        // Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not language specific but just compares unicode
        const std::collate<wchar_t>& coll =
            std::use_facet<std::collate<wchar_t>>(g_langInfo.GetSystemLocale());
        int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
        if (cmp_res != 0)
          return cmp_res;
      }
    }
    i++;
    j++;
  }
  // Compared characters of shortest are the same as longest, length determines order
  return (nKey1 - nKey2);
}

int StringUtils::DateStringToYYYYMMDD(const std::string &dateString)
{
  std::vector<std::string> days = StringUtils::Split(dateString, '-');
  if (days.size() == 1)
    return atoi(days[0].c_str());
  else if (days.size() == 2)
    return atoi(days[0].c_str())*100+atoi(days[1].c_str());
  else if (days.size() == 3)
    return atoi(days[0].c_str())*10000+atoi(days[1].c_str())*100+atoi(days[2].c_str());
  else
    return -1;
}

std::string StringUtils::ISODateToLocalizedDate(const std::string& strIsoDate)
{
  // Convert ISO8601 date strings YYYY, YYYY-MM, or YYYY-MM-DD to (partial) localized date strings
  CDateTime date;
  std::string formattedDate = strIsoDate;
  if (formattedDate.size() == 10)
  {
    date.SetFromDBDate(strIsoDate);
    formattedDate = date.GetAsLocalizedDate();
  }
  else if (formattedDate.size() == 7)
  {
    std::string strFormat = date.GetAsLocalizedDate(false);
    std::string tempdate;
    // find which date separator we are using.  Can be -./
    size_t pos = strFormat.find_first_of("-./");
    if (pos != std::string::npos)
    {
      bool yearFirst = strFormat.find("1601") == 0; // true if year comes first
      std::string sep = strFormat.substr(pos, 1);
      if (yearFirst)
      { // build formatted date with year first, then separator and month
        tempdate = formattedDate.substr(0, 4);
        tempdate += sep;
        tempdate += formattedDate.substr(5, 2);
      }
      else
      {
        tempdate = formattedDate.substr(5, 2);
        tempdate += sep;
        tempdate += formattedDate.substr(0, 4);
      }
      formattedDate = tempdate;
    }
  // return either just the year or the locally formatted version of the ISO date
  }
  return formattedDate;
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
    std::vector<std::string> secs = StringUtils::Split(strCopy, ':');
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
  bool isNegative = lSeconds < 0;
  lSeconds = std::abs(lSeconds);

  std::string strHMS;
  if (format == TIME_FORMAT_SECS)
    strHMS = std::to_string(lSeconds);
  else if (format == TIME_FORMAT_MINS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 60.0f));
  else if (format == TIME_FORMAT_HOURS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 3600.0f));
  else if (format & TIME_FORMAT_M)
    strHMS += std::to_string(lSeconds % 3600 / 60);
  else
  {
    int hh = lSeconds / 3600;
    lSeconds = lSeconds % 3600;
    int mm = lSeconds / 60;
    int ss = lSeconds % 60;

    if (format == TIME_FORMAT_GUESS)
      format = (hh >= 1) ? TIME_FORMAT_HH_MM_SS : TIME_FORMAT_MM_SS;
    if (format & TIME_FORMAT_HH)
      strHMS += StringUtils::Format("{:02}", hh);
    else if (format & TIME_FORMAT_H)
      strHMS += std::to_string(hh);
    if (format & TIME_FORMAT_MM)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", mm);
    if (format & TIME_FORMAT_SS)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", ss);
  }

  if (isNegative)
    strHMS = "-" + strHMS;

  return strHMS;
}

bool StringUtils::IsNaturalNumber(const std::string& str)
{
  size_t i = 0;
  size_t n = 0;
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
  size_t i = 0;
  size_t n = 0;
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
  constexpr std::array<char, 9> prefixes = {' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
  unsigned int i = 0;
  double s = (double)size;
  while (i < prefixes.size() && s >= 1000.0)
  {
    s /= 1024.0;
    i++;
  }

  if (!i)
    strLabel = StringUtils::Format("{:.2f} B", s);
  else if (i == prefixes.size())
  {
    if (s >= 1000.0)
      strLabel = StringUtils::Format(">999.99 {}B", prefixes[i - 1]);
    else
      strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i - 1]);
  }
  else if (s >= 100.0)
    strLabel = StringUtils::Format("{:.1f} {}B", s, prefixes[i]);
  else
    strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i]);

  return strLabel;
}

std::string StringUtils::BinaryStringToString(const std::string& in)
{
  std::string out;
  out.reserve(in.size() / 2);
  for (const char *cur = in.c_str(), *end = cur + in.size(); cur != end; ++cur) {
    if (*cur == '\\') {
      ++cur;
      if (cur == end) {
        break;
      }
      if (isdigit(*cur)) {
        char* end;
        unsigned long num = strtol(cur, &end, 10);
        cur = end - 1;
        out.push_back(num);
        continue;
      }
    }
    out.push_back(*cur);
  }
  return out;
}

std::string StringUtils::ToHexadecimal(const std::string& in)
{
  std::ostringstream ss;
  ss << std::hex;
  for (unsigned char ch : in) {
    ss << std::setw(2) << std::setfill('0') << static_cast<unsigned long> (ch);
  }
  return ss.str();
}

namespace
{
// TODO: Move to Anonymous namespace

/*!
 * \brief Convert wstring to hex primarily for debugging purposes.
 *
 * Note: does not reorder hex for endian-ness
 *
 * \param in wstring to be rendered as hex
 *
 * \return Each character in 'in' represented in hex, separated by space
 *
 */

std::string ToHex(const std::wstring_view in)
{
  int width = sizeof(wchar_t) * 2; // Can vary
  std::string gap;
  std::ostringstream ss;
  ss << std::noshowbase; // manually show 0x (due to 0 omitting it)
  ss << std::internal;
  ss << std::setfill('0');
  for (unsigned char ch : in)
  {
    ss << std::setw(width) << std::hex << static_cast<unsigned long>(ch) << gap;
    gap = " "s;
  }
  return ss.str();
}

/*!
 * \brief Convert u32string to hex primarily for debugging purposes.
 *
 * Note: does not reorder hex for endian-ness
 *
 * \param in u32string to be rendered as hex
 *
 * \return Each character in 'in' represented in hex, separated by space
 *
 */

std::string ToHex(const std::u32string_view in)
{
  int width = sizeof(char32_t) * 2; // Can vary
  std::string gap;
  std::ostringstream ss;
  ss << std::noshowbase; // manually show 0x (due to 0 omitting it)
  ss << std::internal;
  ss << std::setfill('0');
  for (unsigned char ch : in)
  {
    ss << std::setw(width) << std::hex << static_cast<unsigned long>(ch) << gap;
    gap = " "s;
  }
  return ss.str();
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
} // namespace

size_t StringUtils::FindWords(const char *str, const char *wordLowerCase)
{
  // NOTE: This assumes word is lowercase!
  const unsigned char *s = (const unsigned char *)str;
  do
  {
    // start with a compare
    const unsigned char *c = s;
    const unsigned char *w = (const unsigned char *)wordLowerCase;
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
#ifdef HAVE_NEW_CROSSGUID
#ifdef TARGET_ANDROID
  JNIEnv* env = xbmc_jnienv();
  return xg::newGuid(env).str();
#else
  return xg::newGuid().str();
#endif /* TARGET_ANDROID */
#else
  static GuidGenerator guidGenerator;
  auto guid = guidGenerator.newGuid();

  std::stringstream strGuid; strGuid << guid;
  return strGuid.str();
#endif
}

bool StringUtils::ValidateUUID(const std::string &uuid)
{
  CRegExp guidRE;
  guidRE.RegComp(ADDON_GUID_RE);
  return (guidRE.RegFind(uuid.c_str()) == 0);
}

double StringUtils::CompareFuzzy(const std::string &left, const std::string &right)
{
  return (0.5 + fstrcmp(left.c_str(), right.c_str()) * (left.length() + right.length())) / 2.0;
}

int StringUtils::FindBestMatch(const std::string &str, const std::vector<std::string> &strings, double &matchscore)
{
  int best = -1;
  matchscore = 0;

  int i = 0;
  for (std::vector<std::string>::const_iterator it = strings.begin(); it != strings.end(); ++it, i++)
  {
    int maxlength = std::max(str.length(), it->length());
    double score = StringUtils::CompareFuzzy(str, *it) / maxlength;
    if (score > matchscore)
    {
      matchscore = score;
      best = i;
    }
  }
  return best;
}

bool StringUtils::ContainsKeyword(const std::string &str, const std::vector<std::string> &keywords)
{
  for (std::vector<std::string>::const_iterator it = keywords.begin(); it != keywords.end(); ++it)
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

std::string StringUtils::DeParamify(const std::string& param)
{
  std::string result = param;

  // remove double quotes around the whole string
  if (StringUtils::StartsWith(result, "\"") && StringUtils::EndsWith(result, "\""))
  {
    result.erase(0, 1);
    result.pop_back();

    // unescape double quotes
    StringUtils::Replace(result, "\\\"", "\"");

    // unescape backspaces
    StringUtils::Replace(result, "\\\\", "\\");
  }

  return result;
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

uint32_t StringUtils::ToUint32(std::string_view str, uint32_t fallback /* = 0 */) noexcept
{
  return NumberFromSS(str, fallback);
}

uint64_t StringUtils::ToUint64(std::string_view str, uint64_t fallback /* = 0 */) noexcept
{
  return NumberFromSS(str, fallback);
}

float StringUtils::ToFloat(std::string_view str, float fallback /* = 0.0f */) noexcept
{
  return NumberFromSS(str, fallback);
}

std::string StringUtils::FormatFileSize(uint64_t bytes)
{
  const std::array<std::string, 6> units{{"B", "kB", "MB", "GB", "TB", "PB"}};
  if (bytes < 1000)
    return Format("{}B", bytes);

  size_t i = 0;
  double value = static_cast<double>(bytes);
  while (i + 1 < units.size() && value >= 999.5)
  {
    ++i;
    value /= 1024.0;
  }
  unsigned int decimals = value < 9.995 ? 2 : (value < 99.95 ? 1 : 0);
  return Format("{:.{}f}{}", value, decimals, units[i]);
}

const std::locale& StringUtils::GetOriginalLocale() noexcept
{
  return g_langInfo.GetOriginalLocale();
}

std::string StringUtils::CreateFromCString(const char* cstr)
{
  return cstr != nullptr ? std::string(cstr) : std::string();
}
