/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "AEPacketizerIEC958.h"

static const uint16_t AC3Bitrates   [] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod      [] = {48000, 44100, 32000, 0};
static const uint32_t DTSSampleRates[] = {0, 8000, 16000, 32000, 64000, 128000, 11025, 22050, 44100, 88200, 176400, 12000, 24000, 48000, 96000, 192000};

enum IEC958DataType
{
  IEC958_TYPE_NULL   = 0x00,
  IEC958_TYPE_AC3    = 0x01,
  IEC958_TYPE_DTS1   = 0x0B, /*  512 samples */
  IEC958_TYPE_DTS2   = 0x0C, /* 1024 samples */
  IEC958_TYPE_DTS3   = 0x0D, /* 2048 samples */
  IEC958_TYPE_DTSHD  = 0x11,
  IEC958_TYPE_EAC3   = 0x15,
  IEC958_TYPE_TRUEHD = 0x16
};

CAEPacketizerIEC958::CAEPacketizerIEC958() :
  m_packetSize(0),
  m_hasPacket (false),
  m_dataType  (STREAM_FMT_INVALID),
  m_syncFunc  (&CAEPacketizerIEC958::DetectType),
  m_packFunc  (NULL ),
  m_sampleRate(48000)
{
  m_dllAvUtil.Load();

  m_packetData.m_syncwords[0] = 0xF872;
  m_packetData.m_syncwords[1] = 0x4E1F;
  m_packetData.m_type         = IEC958_TYPE_NULL;
  m_packetData.m_length       = 0;
  bzero(m_packetData.m_data, sizeof(m_packetData.m_data));
}

CAEPacketizerIEC958::~CAEPacketizerIEC958()
{
  m_dllAvUtil.Unload();
}

bool CAEPacketizerIEC958::Initialize()
{
  Reset();
  return true;
}

void CAEPacketizerIEC958::Deinitialize()
{
  Reset();
}

void CAEPacketizerIEC958::Reset()
{
  m_hasPacket  = false;
  m_packetSize = 0;
  m_dataType   = STREAM_FMT_INVALID;
  m_syncFunc   = &CAEPacketizerIEC958::DetectType;

  m_packetData.m_syncwords[0] = 0xF872;
  m_packetData.m_syncwords[1] = 0x4E1F;
  m_packetData.m_type         = IEC958_TYPE_NULL;
  m_packetData.m_length       = 0;
  bzero(m_packetData.m_data, sizeof(m_packetData.m_data));
}

int CAEPacketizerIEC958::AddData(uint8_t *data, unsigned int size)
{
  if (m_hasPacket) return 0;
  unsigned int fsize;
  unsigned int offset = (this->*m_syncFunc)(data, size, &fsize);
  size -= offset;
  data += offset;

  /* we need the complete frame to pack it */
  if (size < fsize)
    return offset;

  /* pack the frame and return the data used */
  ((this)->*m_packFunc)(data, fsize);
  return offset + size;
}

bool CAEPacketizerIEC958::HasPacket()
{
  return m_hasPacket;
}

int CAEPacketizerIEC958::GetPacket(uint8_t **data)
{
  /* if we dont have a packet, return a null packet */
  if (!m_hasPacket && m_packetData.m_type != IEC958_TYPE_NULL)
  {
    m_packetData.m_syncwords[0] = 0xF872;
    m_packetData.m_syncwords[1] = 0x4E1F;
    m_packetData.m_type   = IEC958_TYPE_NULL;
    m_packetData.m_length = 0;
    bzero(m_packetData.m_data, sizeof(m_packetData.m_data));
    SwapPacket();
  }
  else
    m_hasPacket = false;

  *data = (uint8_t*)&m_packetData;
  return sizeof(m_packetData);
}

inline void CAEPacketizerIEC958::SwapPacket()
{
#ifndef __BIG_ENDIAN__
  uint16_t *pos = (uint16_t*)&m_packetData;
  for(unsigned int i = 0; i < sizeof(m_packetData); i += 2, ++pos)
    *pos = Endian_Swap16(*pos);
#endif
}

/* PACK FUNCTIONS */
void CAEPacketizerIEC958::PackAC3(uint8_t *data, unsigned int fsize)
{
  m_packetData.m_syncwords[0] = 0xF872;
  m_packetData.m_syncwords[1] = 0x4E1F;
  m_packetData.m_type   = IEC958_TYPE_AC3;
  m_packetData.m_length = fsize;
  memcpy(m_packetData.m_data, data, fsize);
  bzero (m_packetData.m_data + fsize, sizeof(m_packetData.m_data) - fsize);

  SwapPacket();
  m_hasPacket  = true;
}

void CAEPacketizerIEC958::PackDTS(uint8_t *data, unsigned int fsize)
{
  m_packetData.m_syncwords[0] = 0xF872;
  m_packetData.m_syncwords[1] = 0x4E1F;
  switch(m_dtsBlocks)
  {
    case  512: m_packetData.m_type = IEC958_TYPE_DTS1; break;
    case 1024: m_packetData.m_type = IEC958_TYPE_DTS2; break;
    case 2048: m_packetData.m_type = IEC958_TYPE_DTS3; break;
    default:
      /* invalid frame size */
      return;
  }

  m_packetData.m_length = fsize;
  memcpy(m_packetData.m_data, data, fsize);
  bzero (m_packetData.m_data + fsize, sizeof(m_packetData.m_data) - fsize);

  SwapPacket();
  m_hasPacket  = true;
}

void CAEPacketizerIEC958::PackAAC(uint8_t *data, unsigned int fsize)
{
}

/* SYNC FUNCTIONS */

/*
  This function looks for sync words across the types in paralell, and only does an exhaustive
  test if it finds a syncword. Once sync has been established, the relevent sync function sets
  m_syncFunc to itself. This function will only be called again if total sync is lost, which 
  allows is to switch stream types on the fly much like a real reicever does.
*/
unsigned int CAEPacketizerIEC958::DetectType(uint8_t *data, unsigned int size, unsigned int *fsize)
{
  unsigned int skipped = 0;

  while(size > 0) {
    /* if it could be AC3 */
    if (size > 6 && data[0] == 0x0b && data[1] == 0x77)
    {
      unsigned int skip = SyncAC3(data, size, fsize);
      if (m_dataType == STREAM_FMT_AC3)
        return skipped + skip;
    }

    /* if it could be DTS */
    if (size > 8)
    {
       if (
         (data[0] == 0x7F && data[1] == 0xFE && data[2] == 0x80 && data[3] == 0x01) ||
         (data[0] == 0x1F && data[1] == 0xFF && data[2] == 0xE8 && data[3] == 0x00 && data[4] == 0x07 && (data[5] & 0xF0) == 0xF0) ||
         (data[1] == 0x7F && data[0] == 0xFE && data[3] == 0x80 && data[2] == 0x01) ||
         (data[1] == 0x1F && data[0] == 0xFF && data[3] == 0xE8 && data[2] == 0x00 && data[5] == 0x07 && (data[4] & 0xF0) == 0xF0))
      {
        unsigned int skip = SyncDTS(data, size, fsize);
        if (m_dataType == STREAM_FMT_DTS)
          return skipped + skip;
      }
    }

    /* if it could be AAC */
    if (size > 5 && data[0] == 0xFF && (data[1] & 0xF0) == 0xF0)
    {
      unsigned int skip = SyncAAC(data, size, fsize);
      if (m_dataType == STREAM_FMT_AAC)
        return skipped + skip;
    }

    /* move along one byte */
    --size;
    ++skipped;
    ++data;
  }

  return skipped;
}

unsigned int CAEPacketizerIEC958::SyncAC3(uint8_t *data, unsigned int size, unsigned int *fsize)
{
  unsigned int skip = 0;
  for(skip = 0; size - skip > 6; ++skip, ++data)
  {
    /* search for an ac3 sync word */
    if(data[0] != 0x0b || data[1] != 0x77)
      continue;
 
    uint8_t fscod      = data[4] >> 6;
    uint8_t frmsizecod = data[4] & 0x3F;
    uint8_t bsid       = data[5] >> 3;

    /* sanity checks on the header */
    if (
        fscod      ==   3 ||
        frmsizecod >   37 ||
        bsid       > 0x11
    ) continue;

    /* get the details we need to check crc1 and framesize */
    uint16_t     bitrate   = AC3Bitrates[frmsizecod >> 1];
    unsigned int framesize = 0;
    switch(fscod)
    {
      case 0: framesize = bitrate * 2; break;
      case 1: framesize = (320 * bitrate / 147 + (frmsizecod & 1 ? 1 : 0)); break;
      case 2: framesize = bitrate * 4; break;
    }

    *fsize = framesize * 2;
    m_sampleRate = AC3FSCod[fscod];

    /* dont do extensive testing if we have not lost sync */
    if (m_dataType == STREAM_FMT_AC3 && skip == 0)
      return 0;

    unsigned int crc_size;
    /* if we have enough data, validate the entire packet, else try to validate crc2 (5/8 of the packet) */
    if (framesize <= size - skip)
         crc_size = framesize - 1;
    else crc_size = (framesize >> 1) + (framesize >> 3) - 1;

    if (crc_size <= size - skip)
      if(m_dllAvUtil.av_crc(m_dllAvUtil.av_crc_get_table(AV_CRC_16_ANSI), 0, &data[2], crc_size * 2))
        continue;

    /* if we get here, we can sync */
    m_syncFunc = &CAEPacketizerIEC958::SyncAC3;
    m_packFunc = &CAEPacketizerIEC958::PackAC3;
    m_dataType = STREAM_FMT_AC3;
    CLog::Log(LOGINFO, "CAEPacketizerIEC958::SyncAC3 - AC3 stream detected (%dHz)", m_sampleRate);
    return skip;
  }

  /* if we get here, the entire packet is invalid and we have lost sync */
  m_syncFunc   = &CAEPacketizerIEC958::DetectType;
  m_dataType   = STREAM_FMT_INVALID;
  m_packetSize = 0;
  return size;
}

unsigned int CAEPacketizerIEC958::SyncDTS(uint8_t *data, unsigned int size, unsigned int *fsize)
{
  unsigned int skip;
  bool littleEndian;

  for(skip = 0; size - skip > 8; ++skip, ++data)
  {
    unsigned int header = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    bool match = true;
    unsigned int blocks;
    switch(header)
    {
      /* 14bit BE */
      case 0x1FFFE800:
        if (data[4] != 0x07 || data[5] != 0xf0)
        {
          match = false;
          break;
        }
        blocks = ((data[5] & 0x7) << 4) | ((data[6] & 0x3f) >> 2);
        m_sampleRate = DTSSampleRates[(data[8] >> 2) & 0xf];
        break;

      /* 14bit LE */
      case 0xFF1F00E8:
        if (data[5] != 0x07 || data[4] != 0xf0)
        {
          match = false;
          break;
        }
        blocks = ((data[4] & 0x7) << 4) | ((data[7] & 0x3f) >> 2);
        m_sampleRate = DTSSampleRates[(data[9] >> 2) & 0xf];
        break;

      /* 16bit BE */
      case 0x7FFE8001:
        littleEndian = false;
        blocks = (data[4] >> 2) & 0x7f;
        m_sampleRate = DTSSampleRates[(data[8] >> 2) & 0xf];
        break;

      /* 16bit LE */
      case 0xFE7F0180:
        littleEndian = true;
        blocks = (data[4] >> 2) & 0x7f;
        m_sampleRate = DTSSampleRates[(data[9] >> 2) & 0xf];
        break;


      default:
        match = false;
        break;      
    }

    if (!match) continue;

    if (littleEndian)
    {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((data[4] & 0x80) == 0x80 && (data[4] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      *fsize = ((((data[5] & 0x3) << 8 | data[6]) << 4) | ((data[7] & 0xF0) >> 4)) + 1; 
   }
   else
   {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((data[5] & 0x80) == 0x80 && (data[5] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      *fsize = ((((data[4] & 0x3) << 8 | data[7]) << 4) | ((data[6] & 0xF0) >> 4)) + 1;
   }

    /* make sure the framesize is sane */
    if (*fsize < 96 || *fsize > 16384)
      continue;

    m_dtsBlocks = (blocks + 1) << 5;
    m_syncFunc  = &CAEPacketizerIEC958::SyncDTS;
    m_packFunc  = &CAEPacketizerIEC958::PackDTS;
    m_dataType  = STREAM_FMT_DTS;
    CLog::Log(LOGINFO, "CAEPacketizerIEC958::SyncDTS - DTS stream detected (%d blocks, %dHz)", m_dtsBlocks, m_sampleRate);
    return skip;
  }

  /* lost sync */
  m_syncFunc   = &CAEPacketizerIEC958::DetectType;
  m_dataType   = STREAM_FMT_INVALID;
  m_packetSize = 0;
  return size;
}

unsigned int CAEPacketizerIEC958::SyncAAC(uint8_t *data, unsigned int size, unsigned int *fsize)
{
  unsigned int skip;
  for(skip = 0; size - skip > 5; ++skip, ++data)
  {
    if (data[0] != 0xFF || (data[1] & 0xF0) != 0xF0)
      continue;

    *fsize = (data[3] & 0x03) << 11 | data[4] << 3 | (data[5] & 0xE0) >> 5;
    if (*fsize < 7)
      continue;

    m_syncFunc = &CAEPacketizerIEC958::SyncAAC;
    m_packFunc = &CAEPacketizerIEC958::PackAAC;
    m_dataType = STREAM_FMT_AAC;
    CLog::Log(LOGINFO, "CAEPacketizerIEC958::SyncAAC - AAC stream detected");
    return skip;
  }

  /* lost sync */
  m_syncFunc   = &CAEPacketizerIEC958::DetectType;
  m_dataType   = STREAM_FMT_INVALID;
  m_packetSize = 0;
  return size;
}

