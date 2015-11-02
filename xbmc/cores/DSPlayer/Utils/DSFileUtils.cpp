/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "utils/DSFileUtils.h"
#include "DSUtil/DSUtil.h"
#include "utils/charsetconverter.h"
#include "utils/StringUtils.h"

#ifdef HAS_DS_PLAYER

static struct SDSCharsetMapping
{
  const char* charset;
  const char* caption;
  int win_id;
} g_charsets[] = {
  { "ISO-8859-1", "Western Europe (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-2", "Central Europe (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-3", "South Europe (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-4", "Baltic (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-5", "Cyrillic (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-6", "Arabic (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-7", "Greek (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-8", "Hebrew (ISO)", DEFAULT_CHARSET }
  , { "ISO-8859-9", "Turkish (ISO)", DEFAULT_CHARSET }
  , { "CP1250", "Central Europe (Windows)", EASTEUROPE_CHARSET }
  , { "CP1251", "Cyrillic (Windows)", RUSSIAN_CHARSET }
  , { "CP1252", "Western Europe (Windows)", ANSI_CHARSET }
  , { "CP1253", "Greek (Windows)", GREEK_CHARSET }
  , { "CP1254", "Turkish (Windows)", TURKISH_CHARSET }
  , { "CP1255", "Hebrew (Windows)", HEBREW_CHARSET }
  , { "CP1256", "Arabic (Windows)", ARABIC_CHARSET }
  , { "CP1257", "Baltic (Windows)", BALTIC_CHARSET }
  , { "CP1258", "Vietnamesse (Windows)", VIETNAMESE_CHARSET }
  , { "CP874", "Thai (Windows)", THAI_CHARSET }
  , { "BIG5", "Chinese Traditional (Big5)", CHINESEBIG5_CHARSET }
  , { "GBK", "Chinese Simplified (GBK)", GB2312_CHARSET }
  , { "SHIFT_JIS", "Japanese (Shift-JIS)", SHIFTJIS_CHARSET }
  , { "CP949", "Korean", HANGEUL_CHARSET }
  , { "BIG5-HKSCS", "Hong Kong (Big5-HKSCS)", DEFAULT_CHARSET }
  , { NULL, NULL, 0 }
};


std::string CDSFile::SmbToUncPath(const std::string& strFileName)
{
  if (!StringUtils::StartsWithNoCase(strFileName, "smb://"))
    return strFileName;

  CStdString strWinFileName;
  // Find first "/" after " smb://"
  int iEndOfHostNameInd = strFileName.find_first_of('/', 6);
  std::size_t found = strFileName.find_last_of('@', iEndOfHostNameInd);
  
  if (found != std::string::npos)
  {
    strWinFileName = "\\\\" + strFileName.substr(found + 1);
  }
  else
  {
    strWinFileName = strFileName;
    strWinFileName.Replace("smb://", "\\\\");
  }

  strWinFileName.Replace('/', '\\');

  return strWinFileName;
}

bool CDSFile::Exists(const std::string& strFileName, long* errCode)
{
  CStdString strWinFile = SmbToUncPath(strFileName);
  CStdStringW strFileW;
  g_charsetConverter.utf8ToW(strWinFile, strFileW, false);

  DWORD dwAttr = GetFileAttributesW(strFileW.c_str());
  if(dwAttr != 0xffffffff)
    return true;

  if (errCode)
    *errCode = GetLastError();

  return false;
}

int CDSCharsetConverter::getCharsetIdByName(const std::string& charsetName)
{
  for (SDSCharsetMapping *c = g_charsets; c->charset; c++)
  {
    if (StringUtils::EqualsNoCase(charsetName, c->charset))
      return c->win_id;
  }

  return 1;
}

int64_t CDSTimeUtils::GetPerfCounter()
{
  LARGE_INTEGER i64Ticks100ns;
  LARGE_INTEGER llPerfFrequency;

  QueryPerformanceFrequency(&llPerfFrequency);
  if (llPerfFrequency.QuadPart != 0)
  {
    QueryPerformanceCounter(&i64Ticks100ns);
    return llMulDiv(i64Ticks100ns.QuadPart, 10000000, llPerfFrequency.QuadPart, 0);
  }
  else
  {
    // ms to 100ns units
    return timeGetTime() * 10000;
  }
}

#endif
