/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdString.h"
#include "AEUtil.h"
#include "utils/log.h"

using namespace std;

unsigned int CAEUtil::GetChLayoutCount(const AEChLayout src)
{
  unsigned int i = 0;
  for(i = 0; i < AE_CH_MAX && src[i] != AE_CH_NULL;) ++i;
  return i;
}

const char* CAEUtil::GetChName(const AEChannel ch)
{
  if (ch < 0 || ch >= AE_CH_MAX)
    return "UNKNOWN";

  static const char* channels[AE_CH_MAX] =
  {
    "RAW" ,
    "FL"  , "FR" , "FC" , "LFE", "BL" , "BR" , "FLOC",
    "FROC", "BC" , "SL" , "SR" , "TFL", "TFR", "TFC" ,
    "TC"  , "TBL", "TBR", "TBC"
  };

  return channels[ch];
}

CStdString CAEUtil::GetChLayoutStr(const AEChLayout src)
{
  if (src == NULL) return "NULL";
  unsigned int i = 0;
  CStdString s;
  for(i = 0; i < AE_CH_MAX && src[i] != AE_CH_NULL; ++i)
  {
    s.append(GetChName(src[i]));
    if (i + 1 < AE_CH_MAX && src[i+1] != AE_CH_NULL)
      s.append(",");
  }

  return s;
}

const AEChLayout CAEUtil::GuessChLayout(const unsigned int channels)
{
  CLog::Log(LOGWARNING, "CAEUtil::GuessChLayout - This method should really never be used, please fix the code that called this");
  if (channels < 1 || channels > 8)
    return NULL;

  AEStdChLayout layout;
  switch(channels)
  {
    case 1: layout = AE_CH_LAYOUT_1_0; break;
    case 2: layout = AE_CH_LAYOUT_2_0; break;
    case 3: layout = AE_CH_LAYOUT_3_0; break;
    case 4: layout = AE_CH_LAYOUT_4_0; break;
    case 5: layout = AE_CH_LAYOUT_5_0; break;
    case 6: layout = AE_CH_LAYOUT_5_1; break;
    case 7: layout = AE_CH_LAYOUT_7_0; break;
    case 8: layout = AE_CH_LAYOUT_7_1; break;
  }

  return GetStdChLayout(layout);
}

const AEChLayout CAEUtil::GetStdChLayout(const enum AEStdChLayout layout)
{
  if (layout < 0 || layout >= AE_CH_LAYOUT_MAX)
    return NULL;

  static enum AEChannel layouts[AE_CH_LAYOUT_MAX][9] = {
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL , AE_CH_BR , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_BL , AE_CH_BR , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_LFE, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_SL , AE_CH_SR, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR, AE_CH_FC , AE_CH_BL , AE_CH_BR , AE_CH_SL , AE_CH_SR, AE_CH_LFE, AE_CH_NULL}
  };

  return layouts[layout];
}

const char* CAEUtil::GetStdChLayoutName(const enum AEStdChLayout layout)
{
  if (layout < 0 || layout >= AE_CH_LAYOUT_MAX)
    return "UNKNOWN";

  static const char* layouts[AE_CH_LAYOUT_MAX] = 
  {
    "1.0",
    "2.0", "2.1", "3.0", "3.1", "4.0",
    "4.1", "5.0", "5.1", "7.0", "7.1"
  };

  return layouts[layout];
}

const unsigned int CAEUtil::DataFormatToBits(const enum AEDataFormat dataFormat)
{
  if (dataFormat < 0 || dataFormat >= AE_FMT_MAX)
    return 0;

  static const unsigned int formats[AE_FMT_MAX] =
  {
    8,                   /* U8     */
    8,                   /* S8     */

    16,                  /* S16BE  */
    16,                  /* S16LE  */
    16,                  /* S16NE  */

    32,                  /* S32BE  */
    32,                  /* S32LE  */
    32,                  /* S32NE  */

    32,                  /* S24BE  */
    32,                  /* S24LE  */
    32,                  /* S24NE  */

    24,                  /* S24BE3 */
    24,                  /* S24LE3 */
    24,                  /* S24NE3 */

    sizeof(double) << 3, /* DOUBLE */
    sizeof(float ) << 3, /* FLOAT  */

    8                    /* RAW    */
  };

  return formats[dataFormat];
}

const char* CAEUtil::DataFormatToStr(const enum AEDataFormat dataFormat)
{
  if (dataFormat < 0 || dataFormat >= AE_FMT_MAX)
    return "UNKNOWN";

  static const char *formats[AE_FMT_MAX] =
  {
    "AE_FMT_U8",
    "AE_FMT_S8",

    "AE_FMT_S16BE",
    "AE_FMT_S16LE",
    "AE_FMT_S16NE",

    "AE_FMT_S32BE",
    "AE_FMT_S32LE",
    "AE_FMT_S32NE",

    "AE_FMT_S24BE4",
    "AE_FMT_S24LE4",
    "AE_FMT_S24NE4",  /* S24 in 4 bytes */

    "AE_FMT_S24BE3",
    "AE_FMT_S24LE3",
    "AE_FMT_S24NE3", /* S24 in 3 bytes */

    "AE_FMT_DOUBLE",
    "AE_FMT_FLOAT",

    /* for passthrough streams and the like */
    "AE_FMT_RAW"
  };

  return formats[dataFormat];
}
