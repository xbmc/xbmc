/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "AEELDParser.h"
#include "AEDeviceInfo.h"
#include "utils/EndianSwap.h"
#include <string.h>
#include <algorithm>
#include <functional>

#include <stdio.h>

#define GRAB_BITS(buf, byte, lowbit, bits) ((buf[byte] >> (lowbit)) & ((1 << (bits)) - 1))

typedef struct
{
  uint8_t     eld_ver;
  uint8_t     baseline_eid_len;
  uint8_t     cea_edid_ver;
  uint8_t     monitor_name_length;
  uint8_t     sad_count;
  uint8_t     conn_type;
  bool        s_ai;
  bool        hdcp;
  uint8_t     audio_sync_delay;
  bool        rlrc; /* rear left and right of center */
  bool        flrc; /* front left and right of center */
  bool        rc;   /* rear center */
  bool        rlr;  /* rear left and right */
  bool        fc;   /* front center */
  bool        lfe;  /* LFE */
  bool        flr;  /* front left and right */
  uint64_t    port_id;
  char        mfg_name[4];
  uint16_t    product_code;
  std::string monitor_name;
} ELDHeader;

#define ELD_VER_CEA_816D         2
#define ELD_VER_PARTIAL          31

#define ELD_EDID_VER_NONE        0
#define ELD_EDID_VER_CEA_861     1
#define ELD_EDID_VER_CEA_861_A   2
#define ELD_EDID_VER_CEA_861_BCD 3

#define ELD_CONN_TYPE_HDMI       0
#define ELD_CONN_TYPE_DP         1
#define ELD_CONN_TYPE_RESERVED1  2
#define ELD_CONN_TYPE_RESERVED2  3

#define CEA_861_FORMAT_RESERVED1 0
#define CEA_861_FORMAT_LPCM      1
#define CEA_861_FORMAT_AC3       2
#define CEA_861_FORMAT_MPEG1     3
#define CEA_861_FORMAT_MP3       4
#define CEA_861_FORMAT_MPEG2     5
#define CEA_861_FORMAT_AAC       6
#define CEA_861_FORMAT_DTS       7
#define CEA_861_FORMAT_ATRAC     8
#define CEA_861_FORMAT_SACD      9
#define CEA_861_FORMAT_EAC3      10
#define CEA_861_FORMAT_DTSHD     11
#define CEA_861_FORMAT_MLP       12
#define CEA_861_FORMAT_DST       13
#define CEA_861_FORMAT_WMAPRO    14
#define CEA_861_FORMAT_RESERVED2 15

#define rtrim(s) s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end())

void CAEELDParser::Parse(const uint8_t *data, size_t length, CAEDeviceInfo& info)
{
  ELDHeader header;
  header.eld_ver = (data[0 ] & 0xF8) >> 3;
  if (header.eld_ver != ELD_VER_CEA_816D && header.eld_ver != ELD_VER_PARTIAL)
    return;

  header.baseline_eid_len    =  data[2 ];
  header.cea_edid_ver        = (data[4 ] & 0xE0) >> 5;
  header.monitor_name_length =  data[4 ] & 0x1F;
  header.sad_count           = (data[5 ] & 0xF0) >> 4;
  header.conn_type           = (data[5 ] & 0x0C) >> 2;
  header.s_ai                = (data[5 ] & 0x02) == 0x02;
  header.hdcp                = (data[5 ] & 0x01) == 0x01;
  header.audio_sync_delay    =  data[6 ];
  header.rlrc                = (data[7 ] & 0x40) == 0x40;
  header.flrc                = (data[7 ] & 0x20) == 0x20;
  header.rc                  = (data[7 ] & 0x10) == 0x10;
  header.rlr                 = (data[7 ] & 0x08) == 0x08;
  header.fc                  = (data[7 ] & 0x04) == 0x04;
  header.lfe                 = (data[7 ] & 0x02) == 0x02;
  header.flr                 = (data[7 ] & 0x01) == 0x01;
  header.port_id             = Endian_SwapLE64(*((uint64_t*)(data + 8)));
  header.mfg_name[0]         = 'A' + ((data[16] >> 2) & 0x1F) - 1;
  header.mfg_name[1]         = 'A' + (((data[16] << 3) | (data[17] >> 5)) & 0x1F) - 1;
  header.mfg_name[2]         = 'A' + (data[17] & 0x1F) - 1;
  header.mfg_name[3]         = '\0';
  header.product_code        = Endian_SwapLE16(*((uint16_t*)(data + 18)));

  switch (header.conn_type)
  {
    case ELD_CONN_TYPE_HDMI: info.m_deviceType = AE_DEVTYPE_HDMI; break;
    case ELD_CONN_TYPE_DP  : info.m_deviceType = AE_DEVTYPE_DP  ; break;
  }

  info.m_displayNameExtra = header.mfg_name;
  if (header.monitor_name_length <= 16)
  {
    header.monitor_name.assign((const char *)(data + 20), header.monitor_name_length);
    rtrim(header.monitor_name);
    if (header.monitor_name.length() > 0)
    {
      info.m_displayNameExtra.append(" ");
      info.m_displayNameExtra.append(header.monitor_name);
      if (header.conn_type == ELD_CONN_TYPE_HDMI)
        info.m_displayNameExtra.append(" on HDMI"       );
      else
        info.m_displayNameExtra.append(" on DisplayPort");
    }
  }

  if (header.flr)
  {
    if (!info.m_channels.HasChannel(AE_CH_FL))
      info.m_channels += AE_CH_FL;
    if (!info.m_channels.HasChannel(AE_CH_FR))
      info.m_channels += AE_CH_FR;
  }

  if (header.lfe)
    if (!info.m_channels.HasChannel(AE_CH_LFE))
      info.m_channels += AE_CH_LFE;

  if (header.fc)
    if (!info.m_channels.HasChannel(AE_CH_FC))
      info.m_channels += AE_CH_FC;

  if (header.rlr)
  {
    if (!info.m_channels.HasChannel(AE_CH_BL))
      info.m_channels += AE_CH_BL;
    if (!info.m_channels.HasChannel(AE_CH_BR))
      info.m_channels += AE_CH_BR;
  }

  if (header.rc)
    if (!info.m_channels.HasChannel(AE_CH_BC))
      info.m_channels += AE_CH_BC;

  if (header.flrc)
  {
    if (!info.m_channels.HasChannel(AE_CH_FLOC))
      info.m_channels += AE_CH_FLOC;
    if (!info.m_channels.HasChannel(AE_CH_FROC))
      info.m_channels += AE_CH_FROC;
  }

  if (header.rlrc)
  {
    if (!info.m_channels.HasChannel(AE_CH_BLOC))
      info.m_channels += AE_CH_BLOC;
    if (!info.m_channels.HasChannel(AE_CH_BROC))
      info.m_channels += AE_CH_BROC;
  }

  const uint8_t *sad = data + 20 + header.monitor_name_length;
  for(uint8_t i = 0; i < header.sad_count; ++i)
  {
    uint8_t offset = i * 3;
    uint8_t formatCode   = (sad[offset + 0] >> 3) & 0xF;
    //uint8_t channelCount = (sad[offset + 0] & 0x7) + 1;
    //uint8_t sampleRates  =  sad[offset + 1];

    AEDataFormat fmt = AE_FMT_INVALID;
    switch (formatCode)
    {
      case CEA_861_FORMAT_AAC  : fmt = AE_FMT_AAC   ; break;
      case CEA_861_FORMAT_AC3  : fmt = AE_FMT_AC3   ; break;
      case CEA_861_FORMAT_DTS  : fmt = AE_FMT_DTS   ; break;
      case CEA_861_FORMAT_DTSHD: fmt = AE_FMT_DTSHD ; break;
      case CEA_861_FORMAT_EAC3 : fmt = AE_FMT_EAC3  ; break;
      case CEA_861_FORMAT_LPCM : fmt = AE_FMT_LPCM  ; break;
      case CEA_861_FORMAT_MLP  : fmt = AE_FMT_TRUEHD; break;
    }

    if (fmt == AE_FMT_INVALID)
      continue;

    if (std::find(info.m_dataFormats.begin(), info.m_dataFormats.end(), fmt) == info.m_dataFormats.end())
      info.m_dataFormats.push_back(fmt);
  }
}
