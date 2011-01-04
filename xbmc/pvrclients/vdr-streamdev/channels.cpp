/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * This code is taken from channels.c in the Video Disk Recorder ('VDR')
 */

#include <string>
#include <vector>
#include "vtptransceiver.h"
#include "pvrclient-vdr_os.h"
#include "channels.h"
#include "client.h"

using namespace std;

const tChannelParameterMap InversionValues[] = {
  {   0, INVERSION_OFF,  "off" },
  {   1, INVERSION_ON,   "on" },
  { 999, INVERSION_AUTO, "auto" },
  { -1 }
  };

const tChannelParameterMap BandwidthValues[] = {
  {   6, 6000000, "6 MHz" },
  {   7, 7000000, "7 MHz" },
  {   8, 8000000, "8 MHz" },
  { -1 }
  };

const tChannelParameterMap CoderateValues[] = {
  {   0, FEC_NONE, "none" },
  {  12, FEC_1_2,  "1/2" },
  {  23, FEC_2_3,  "2/3" },
  {  34, FEC_3_4,  "3/4" },
  {  35, FEC_3_5,  "3/5" },
  {  45, FEC_4_5,  "4/5" },
  {  56, FEC_5_6,  "5/6" },
  {  67, FEC_6_7,  "6/7" },
  {  78, FEC_7_8,  "7/8" },
  {  89, FEC_8_9,  "8/9" },
  { 910, FEC_9_10, "9/10" },
  { 999, FEC_AUTO, "auto" },
  { -1 }
  };

const tChannelParameterMap ModulationValues[] = {
  {  16, QAM_16,   "QAM16" },
  {  32, QAM_32,   "QAM32" },
  {  64, QAM_64,   "QAM64" },
  { 128, QAM_128,  "QAM128" },
  { 256, QAM_256,  "QAM256" },
  {   2, QPSK,     "QPSK" },
  {   5, PSK_8,    "8PSK" },
  {   6, APSK_16,  "16APSK" },
  {  10, VSB_8,    "VSB8" },
  {  11, VSB_16,   "VSB16" },
  { 998, QAM_AUTO, "QAMAUTO" },
  { -1 }
  };

const tChannelParameterMap SystemValues[] = {
  {   0, SYS_DVBS,  "DVB-S" },
  {   1, SYS_DVBS2, "DVB-S2" },
  { -1 }
  };

const tChannelParameterMap TransmissionValues[] = {
  {   2, TRANSMISSION_MODE_2K,   "2K" },
  {   8, TRANSMISSION_MODE_8K,   "8K" },
  { 999, TRANSMISSION_MODE_AUTO, "auto" },
  { -1 }
  };

const tChannelParameterMap GuardValues[] = {
  {   4, GUARD_INTERVAL_1_4,  "1/4" },
  {   8, GUARD_INTERVAL_1_8,  "1/8" },
  {  16, GUARD_INTERVAL_1_16, "1/16" },
  {  32, GUARD_INTERVAL_1_32, "1/32" },
  { 999, GUARD_INTERVAL_AUTO, "auto" },
  { -1 }
  };

const tChannelParameterMap HierarchyValues[] = {
  {   0, HIERARCHY_NONE, "none" },
  {   1, HIERARCHY_1,    "1" },
  {   2, HIERARCHY_2,    "2" },
  {   4, HIERARCHY_4,    "4" },
  { 999, HIERARCHY_AUTO, "auto" },
  { -1 }
  };

const tChannelParameterMap RollOffValues[] = {
  {   0, ROLLOFF_AUTO, "auto" },
  {  20, ROLLOFF_20, "0.20" },
  {  25, ROLLOFF_25, "0.25" },
  {  35, ROLLOFF_35, "0.35" },
  { -1 }
  };


cChannel::cChannel(const PVR_CHANNEL *Channel)
{

}

cChannel::cChannel()
{
  name = strdup("");
  shortName = strdup("");
  provider = strdup("");
  memset(&__BeginData__, 0, (char *)&__EndData__ - (char *)&__BeginData__);
  inversion    = INVERSION_AUTO;
  bandwidth    = 8000000;
  coderateH    = FEC_AUTO;
  coderateL    = FEC_AUTO;
  modulation   = QPSK;
  system       = SYS_DVBS;
  transmission = TRANSMISSION_MODE_AUTO;
  guard        = GUARD_INTERVAL_AUTO;
  hierarchy    = HIERARCHY_AUTO;
  rollOff      = ROLLOFF_AUTO;
  modification = CHANNELMOD_NONE;

}

cChannel::~cChannel()
{
  free(name);
  free(shortName);
  free(provider);
}

bool cChannel::ReadFromVTP(int channel)
{
  vector<string> lines;
  int            code;
  char           buffer[64];

  if (!VTPTransceiver.CheckConnection())
    return false;

  sprintf(buffer, "LSTC %i", channel);
  while (!VTPTransceiver.SendCommand(buffer, code, lines))
  {
    if (code != 451)
      return false;

    Sleep(100);
  }

  vector<string>::iterator it = lines.begin();
  string& data(*it);
  CStdString str_result = data;

  if (g_bCharsetConv)
    XBMC->UnknownToUTF8(str_result);

  Parse(str_result.c_str());
  return true;
}

int UserIndex(int Value, const tChannelParameterMap *Map)
{
  const tChannelParameterMap *map = Map;
  while (map && map->userValue != -1)
  {
    if (map->userValue == Value)
      return map - Map;
    map++;
  }
  return -1;
}

int MapToDriver(int Value, const tChannelParameterMap *Map)
{
  int n = UserIndex(Value, Map);
  if (n >= 0)
     return Map[n].driverValue;
  return -1;
}

static const char *ParseParameter(const char *s, int &Value, const tChannelParameterMap *Map)
{
  if (*++s)
  {
    char *p = NULL;
    errno = 0;
    int n = strtol(s, &p, 10);
    if (!errno && p != s)
    {
      Value = MapToDriver(n, Map);
      if (Value >= 0)
        return p;
    }
  }
  XBMC->Log(LOG_ERROR, "PCRClient-vdr: invalid value for channelparameter '%c'", *(s - 1));
  return NULL;
}

static const char *SkipDigits(const char *s)
{
  while (*++s && isdigit(*s))
        ;
  return s;
}

bool cChannel::StringToParameters(const char *s)
{
  while (s && *s)
  {
    switch (toupper(*s))
    {
      case 'A': s = SkipDigits(s); break; // for compatibility with the "multiproto" approach - may be removed in future versions
      case 'B': s = ParseParameter(s, bandwidth, BandwidthValues); break;
      case 'C': s = ParseParameter(s, coderateH, CoderateValues); break;
      case 'D': s = ParseParameter(s, coderateL, CoderateValues); break;
      case 'G': s = ParseParameter(s, guard, GuardValues); break;
      case 'H': polarization = *s++; break;
      case 'I': s = ParseParameter(s, inversion, InversionValues); break;
      case 'L': polarization = *s++; break;
      case 'M': s = ParseParameter(s, modulation, ModulationValues); break;
      case 'O': s = ParseParameter(s, rollOff, RollOffValues); break;
      case 'P': s = SkipDigits(s); break; // for compatibility with the "multiproto" approach - may be removed in future versions
      case 'R': polarization = *s++; break;
      case 'S': s = ParseParameter(s, system, SystemValues); break;
      case 'T': s = ParseParameter(s, transmission, TransmissionValues); break;
      case 'V': polarization = *s++; break;
      case 'Y': s = ParseParameter(s, hierarchy, HierarchyValues); break;
      case 'Z': s = SkipDigits(s); break; // for compatibility with the original DVB-S2 patch - may be removed in future versions
      default: XBMC->Log(LOG_ERROR, "PCRClient-vdr: unknown parameter key '%c'", *s);
          return false;
    }
  }
  return true;
}

bool cChannel::Parse(const char *s)
{
  bool ok = true;

  char namebuf[256];
  char sourcebuf[256];
  char parambuf[256];
  char vpidbuf[128];
  char apidbuf[128];
  char caidbuf[128];
  int fields = sscanf(s, "%d %[^:]:%d:%[^:]:%[^:]:%d :%[^:]:%[^:]:%d :%[^:]:%d :%d :%d :%d ", &number, namebuf, &frequency, parambuf, sourcebuf, &srate, vpidbuf, apidbuf, &tpid, caidbuf, &sid, &nid, &tid, &rid);
  if (fields >= 9)
  {
    if (fields == 9)
    {
      // allow reading of old format
      sid = atoi(caidbuf);
      caids[0] = tpid;
      caids[1] = 0;
      tpid = 0;
    }
    vpid = ppid = 0;
    vtype = 2; // default is MPEG-2
    apids[0] = 0;
    dpids[0] = 0;

    ok = StringToParameters(parambuf);

    char *p;
    if ((p = strchr(vpidbuf, '=')) != NULL)
    {
      *p++ = 0;
      if (sscanf(p, "%d", &vtype) != 1)
      return false;
    }
    if ((p = strchr(vpidbuf, '+')) != NULL)
    {
      *p++ = 0;
      if (sscanf(p, "%d", &ppid) != 1)
      return false;
    }
    if (sscanf(vpidbuf, "%d", &vpid) != 1)
      return false;
    if (!ppid)
      ppid = vpid;

    char *dpidbuf = strchr(apidbuf, ';');
    if (dpidbuf)
      *dpidbuf++ = 0;
    p = apidbuf;
    char *q;
    int NumApids = 0;
    char *strtok_next;
    while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
    {
      if (NumApids < MAXAPIDS) {
      char *l = strchr(q, '=');
      if (l)
      {
        *l++ = 0;
        strn0cpy(alangs[NumApids], l, MAXLANGCODE2);
      }
      else
        *alangs[NumApids] = 0;
      apids[NumApids++] = strtol(q, NULL, 10);
      }
      else
        XBMC->Log(LOG_ERROR, "PCRClient-vdr: too many APIDs!"); // no need to set ok to 'false'
      p = NULL;
    }
    apids[NumApids] = 0;

    if (dpidbuf)
    {
      char *p = dpidbuf;
      char *q;
      char *strtok_next;

      int NumDpids = 0;
      while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
      {
        if (NumDpids < MAXDPIDS)
        {
          char *l = strchr(q, '=');
          if (l)
          {
            *l++ = 0;
            strn0cpy(dlangs[NumDpids], l, MAXLANGCODE2);
          }
          else
            *dlangs[NumDpids] = 0;
          dpids[NumDpids++] = strtol(q, NULL, 10);
        }
        else
          XBMC->Log(LOG_ERROR, "PCRClient-vdr: too many DPIDs!"); // no need to set ok to 'false'
        p = NULL;
      }
      dpids[NumDpids] = 0;
    }

    p = caidbuf;
    int NumCaIds = 0;
    while ((q = strtok_r(p, ",", &strtok_next)) != NULL)
    {
      if (NumCaIds < MAXCAIDS)
      {
        caids[NumCaIds++] = strtol(q, NULL, 16) & 0xFFFF;
        if (NumCaIds == 1 && caids[0] <= CA_USER_MAX)
          break;
      }
      else
        XBMC->Log(LOG_ERROR, "PCRClient-vdr: too many CA ids!"); // no need to set ok to 'false'
      p = NULL;
    }
    caids[NumCaIds] = 0;
    strreplace(namebuf, '|', ':');

    p = strchr(namebuf, ';');
    if (p)
    {
      *p++ = 0;
      provider = strcpyrealloc(provider, p);
    }
    p = strchr(namebuf, ',');
    if (p)
    {
      *p++ = 0;
      shortName = strcpyrealloc(shortName, p);
    }
    name = strcpyrealloc(name, namebuf);
  }
  else
    return false;

  return ok;
}
