/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "XBTimeZone.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/Undocumented.h"
#endif
// extracted and translated from xboxdash.xbe version 5960
//   structures @ file offset 0x101F0-0x10A24 / VA 0x201F0 - 0x20A24
//   strings @ file offset 0x142B8-0x14BFC / VA 0x242B8 - 0x24BFC
extern const MINI_TZI g_TimeZoneInfo[] = {
    {
      "GMT-12",
      720,
      "IDLW",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-11",
      660,
      "NT",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-10",
      600,
      "HST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-09 YDT",
      540,
      "YST",
      { 11, 1, 0, 2 },
      0,
      "YDT",
      {  3, 2, 0, 2 },
      -60,
    },
    {
      "GMT-09 PDT",
      480,
      "PST",
      { 11, 1, 0, 2 },
      0,
      "PDT",
      {  3, 2, 0, 2 },
      -60,
    },
    {
      "GMT-07 MST",
      420,
      "MST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-07 MDT",
      420,
      "MST",
      { 11, 1, 0, 2 },
      0,
      "MST",
      {  3, 2, 0, 2 },
      -60,
    },
    {
      "GMT-06 CAST",
      360,
      "CAST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-06 CDT",
      360,
      "CST",
      { 11, 1, 0, 2 },
      0,
      "CDT",
      {  3, 2, 0, 2 },
      -60,
    },
    {
      "GMT-06 MDT",
      360,
      "MST",
      { 10, 5, 0, 2 },
      0,
      "MDT",
      {  4, 1, 0, 2 },
      -60,
    },
    {
      "GMT-06 CCST",
      360,
      "CCST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-05 EDT",
      300,
      "EST",
      { 11, 1, 0, 2 },
      0,
      "EDT",
      {  3, 2, 0, 2 },
      -60,
    },
    {
      "GMT-05 EST",
      300,
      "EST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-05 SPST",
      300,
      "SPST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-04 ADT",
      240,
      "AST",
      { 10, 5, 0, 2 },
      0,
      "ADT",
      {  4, 1, 0, 2 },
      -60,
    },
    {
      "GMT-04 SWST",
      240,
      "SWST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-04 PSDT",
      240,
      "PSST",
      {  3, 2, 6, 0 },
      0,
      "PSDT",
      { 10, 2, 6, 0 },
      -60,
    },
    {
      "GMT-03:30",
      210,
      "NST",
      { 10, 5, 0, 2 },
      0,
      "NDT",
      {  4, 1, 0, 2 },
      -60,
    },
    {
      "GMT-03 ESDT",
      180,
      "ESST",
      {  2, 2, 0, 2 },
      0,
      "ESDT",
      { 10, 3, 0, 2 },
      -60,
    },
    {
      "GMT-03 SEST",
      180,
      "SEST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT-03 GDT",
      180,
      "GST",
      { 10, 5, 0, 2 },
      0,
      "GDT",
      {  4, 1, 0, 2 },
      -60,
    },
    {
      "GMT-02",
      120,
      "MAST",
      {  9, 5, 0, 2 },
      0,
      "MADT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT-01 ADT",
      60,
      "AST",
      { 10, 5, 0, 3 },
      0,
      "ADT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT-01 WAT",
      60,
      "WAT",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+00 GST",
      0,
      "GST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+00 BST",
      0,
      "GMT",
      { 10, 5, 0, 2 },
      0,
      "BST",
      {  3, 5, 0, 1 },
      -60,
    },
    {
      "GMT+01 WEDT",
      -60,
      "WEST",
      { 10, 5, 0, 3 },
      0,
      "WEDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+01 CEDT",
      -60,
      "CEST",
      { 10, 5, 0, 3 },
      0,
      "CEDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+01 RDT",
      -60,
      "RST",
      { 10, 5, 0, 3 },
      0,
      "RDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+01 SCDT",
      -60,
      "SCST",
      { 10, 5, 0, 3 },
      0,
      "SCDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+01 WDST",
      -60,
      "WAST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+02 GTDT",
      -120,
      "GTST",
      { 10, 5, 0, 3 },
      0,
      "GTDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+02 EEDT",
      -120,
      "EEST",
      {  9, 5, 0, 1 },
      0,
      "EEDT",
      {  3, 5, 0, 0 },
      -60,
    },
    {
      "GMT+02 EDT",
      -120,
      "EST",
      {  9, 5, 3, 2 },
      0,
      "EDT",
      {  5, 1, 5, 2 },
      -60,
    },
    {
      "GMT+02S AST",
      -120,
      "SAST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+02 FLDT",
      -120,
      "FLST",
      { 10, 5, 0, 4 },
      0,
      "FLDT",
      {  3, 5, 0, 3 },
      -60,
    },
    {
      "GMT+02 JST",
      -120,
      "JST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+03 ADT",
      -180,
      "AST",
      { 10, 1, 0, 4 },
      0,
      "ADT",
      {  4, 1, 0, 3 },
      -60,
    },
    {
      "GMT+03 AST",
      -180,
      "AST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+03 RDT",
      -180,
      "RST",
      { 10, 5, 0, 3 },
      0,
      "RDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+03 EAST",
      -180,
      "EAST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+03:30",
      -210,
      "IST",
      {  9, 4, 2, 2 },
      0,
      "IDT",
      {  3, 1, 0, 2 },
      -60,
    },
    {
      "GMT+04 AST",
      -240,
      "AST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+04 CDT",
      -240,
      "CST",
      { 10, 5, 0, 3 },
      0,
      "CDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+04:30",
      -270,
      "AST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+05 EDT",
      -300,
      "EST",
      { 10, 5, 0, 3 },
      0,
      "EDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+05 WAST",
      -300,
      "WAST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+05:30",
      -330,
      "IST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+05:45",
      -345,
      "NST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+06 NCDT",
      -360,
      "NCST",
      { 10, 5, 0, 3 },
      0,
      "NCDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+06 CAST",
      -360,
      "CAST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+06 SRST",
      -360,
      "SRST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+06:30",
      -390,
      "MST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+07 SAST",
      -420,
      "SAST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+07 NADT",
      -420,
      "NAST",
      { 10, 5, 0, 3 },
      0,
      "NADT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+08 CST",
      -480,
      "CST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+08 NEDT",
      -480,
      "NEST",
      { 10, 5, 0, 3 },
      0,
      "NEDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+08 MPST",
      -480,
      "MPST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+08 AWST",
      -480,
      "AWST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+08 TST",
      -480,
      "TST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+09 TST",
      -540,
      "TST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+09 KST",
      -540,
      "KST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+09 YDT",
      -540,
      "YST",
      { 10, 5, 0, 3 },
      0,
      "YDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+09:30 ACDT",
      -570,
      "ACST",
      {  3, 5, 0, 2 },
      0,
      "ACDT",
      { 10, 5, 0, 2 },
      -60,
    },
    {
      "GMT+09:30 ACST",
      -570,
      "ACST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+10 AEST",
      -600,
      "AEST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+10 AEDT",
      -600,
      "AEST",
      {  3, 5, 0, 2 },
      0,
      "AEDT",
      { 10, 5, 0, 2 },
      -60,
    },
    {
      "GMT+10 WPST",
      -600,
      "WPST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+10 TDT",
      -600,
      "TST",
      {  3, 5, 0, 2 },
      0,
      "TDT",
      { 10, 1, 0, 2 },
      -60,
    },
    {
      "GMT+10 VDT",
      -600,
      "VST",
      { 10, 5, 0, 3 },
      0,
      "VDT",
      {  3, 5, 0, 2 },
      -60,
    },
    {
      "GMT+11",
      -660,
      "CPST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+12 NZDT",
      -720,
      "NZST",
      {  3, 3, 0, 2 },
      0,
      "NZDT",
      { 10, 1, 0, 2 },
      -60,
    },
    {
      "GMT+12 FST",
      -720,
      "FST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+13",
      -780,
      "TST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
    {
      "GMT+14",
      -840,
      "KST",
      {  0, 0, 0, 0 },
      0,
      NULL,
      {  0, 0, 0, 0 },
      0,
    },
};

static const int XBNumberOfTimeZones = sizeof(g_TimeZoneInfo) / sizeof(g_TimeZoneInfo[0]);

XBTimeZone g_timezone;

const char * XBTimeZone::GetTimeZoneString(int index)
{
  if (index >= 0 && index < XBNumberOfTimeZones)
    return g_TimeZoneInfo[index].Name;
  else
    return NULL;
}

const char * XBTimeZone::GetTimeZoneName(int index)
{
  if (index >= 0 && index < XBNumberOfTimeZones)
    return g_TimeZoneInfo[index].StandardName;
  else
    return NULL;
}

int XBTimeZone::GetNumberOfTimeZones()
{
  return XBNumberOfTimeZones;
}

// Reversed from xboxdash.xbe
int XBTimeZone::GetTimeZoneIndex()
{
#ifdef HAS_XBOX_HARDWARE
  TIME_ZONE_INFORMATION tzi;
  char tzi_std_name[32], tzi_day_name[32];

  if (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_INVALID || !tzi.StandardName[0])
  {
    // if we fail to get the timezone or it is not set, use the default for the region in langinfo
    g_langInfo.GetTimeZone();
    if (!g_langInfo.GetTimeZone().IsEmpty())
    {
      int i=0;
      while (i < g_timezone.GetNumberOfTimeZones() && !g_langInfo.GetTimeZone().Equals(g_timezone.GetTimeZoneName(i)))
        i++;

      if (i < g_timezone.GetNumberOfTimeZones())
        return i;
    }
    else
    {
      // timezone for the region xboxdash style
      switch(XGetGameRegion())
      {
      case XC_GAME_REGION_NA:
        return 11;
      case XC_GAME_REGION_JAPAN:
        return 60;
      default:
        return 25;
      }
    }
  }

  // convert tzi.standardname from wide to ascii for easier comparison in a cheezy way
  for (int i=0; i < sizeof(tzi.StandardName) / sizeof(WCHAR); i++)
    tzi_std_name[i] = (char) (tzi.StandardName[i] & 0xFF);
  for (int i=0; i < sizeof(tzi.DaylightName) / sizeof(WCHAR); i++)
    tzi_day_name[i] = (char) (tzi.DaylightName[i] & 0xFF);

  // find the returned TZI in our TZI array.
  for (int i=0; i < XBNumberOfTimeZones; i++)
  {
    if (*(DWORD *)g_TimeZoneInfo[i].StandardName == *(DWORD *)tzi_std_name)
    {
      if ((DWORD)g_TimeZoneInfo[i].DaylightName == 0)
      {
        if (*(DWORD*)tzi_day_name == 0)
          return i;
      }
      else
      {
        if (*(DWORD *)g_TimeZoneInfo[i].DaylightName == *(DWORD *)tzi_day_name)
          return i;
      }
    }
  }
#endif
  return -1;
}

void XBTimeZone::SetTimeZoneIndex(int index)
{
  const MINI_TZI * ptzi;

  if (index >= 0 && index < XBNumberOfTimeZones)
    ptzi = &g_TimeZoneInfo[index];
  else
    return;

  SetTimeZoneInfo(ptzi);
}

bool XBTimeZone::SetTimeZoneInfo(const MINI_TZI * tzi)
{
#ifdef HAS_XBOX_HARDWARE
  BYTE eepromdata[256];
  EEPROM_USER_SETTINGS * usersettings = (EEPROM_USER_SETTINGS *) &eepromdata;
  DWORD type, size;

  if (ExQueryNonVolatileSetting(XC_USER_SETTINGS, &type, &eepromdata, sizeof(eepromdata), &size) < 0)
  {
    CLog::Log(LOGNOTICE, "Failed to get EEPROM User settings!");
    return FALSE;
  }

  usersettings->TimeZoneBias    = tzi->Bias;

  usersettings->TimeZoneStdBias = tzi->StandardBias;
  usersettings->TimeZoneDltBias = tzi->DaylightBias;

  if (tzi->StandardName)
    usersettings->TimeZoneStdName = *(DWORD*)(tzi->StandardName);
  else
    usersettings->TimeZoneStdName = 0;
  usersettings->TimeZoneStdDate = tzi->StandardDate;

  if (tzi->DaylightName)
    usersettings->TimeZoneDltName = *(DWORD*)(tzi->DaylightName);
  else 
    usersettings->TimeZoneDltName = 0;
  usersettings->TimeZoneDltDate = tzi->DaylightDate;

  return ExSaveNonVolatileSetting(XC_USER_SETTINGS, (PULONG) REG_BINARY, &eepromdata, size) >= 0;
#else
  return false;
#endif
}

bool XBTimeZone::GetDST()
{
#ifdef HAS_XBOX_HARDWARE
  DWORD type, flags, size;
  if(ExQueryNonVolatileSetting(XC_DST_SETTING, &type, &flags, sizeof(flags), &size) < 0)
  {
    CLog::Log(LOGERROR, "Failed to get DST setting!");
    return false;
  }
  return (flags & XC_DISABLE_DST_FLAG ) == XC_DISABLE_DST_FLAG ? false : true;
#else
  return false;
#endif
}

void XBTimeZone::SetDST(BOOL bEnable)
{
#ifdef HAS_XBOX_HARDWARE
  DWORD type, flags, size;

  if (ExQueryNonVolatileSetting(XC_DST_SETTING, &type, &flags, sizeof(flags), &size) < 0)
  {
    CLog::Log(LOGERROR, "Failed to get DST setting!");
    return;
  }

  if (bEnable)
    flags &= ~XC_DISABLE_DST_FLAG; // DST Flag used by xboxdash
  else
    flags |= XC_DISABLE_DST_FLAG; // DST Flag used by xboxdash

  ExSaveNonVolatileSetting(XC_DST_SETTING, (PULONG) REG_DWORD, &flags, sizeof(flags));
#endif
}
