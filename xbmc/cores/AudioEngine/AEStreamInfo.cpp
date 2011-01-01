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

#include "AEStreamInfo.h"

#define IEC958_PREAMBLE1  0xF872
#define IEC958_PREAMBLE2  0x4E1F
#define DTS_PREAMBLE_14BE 0x1FFFE800
#define DTS_PREAMBLE_14LE 0xFF1F00E8
#define DTS_PREAMBLE_16BE 0x7FFE8001
#define DTS_PREAMBLE_16LE 0xFE7F0180
#define DTS_SRATE_COUNT   16
#define MAX_EAC3_BLOCKS   6

static const uint16_t AC3Bitrates   [] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod      [] = {48000, 44100, 32000, 0};
static const uint8_t  AC3BlkCod     [] = {1, 2, 3, 6};

static const uint32_t DTSSampleRates[DTS_SRATE_COUNT] =
{
  0     ,
  8000  ,
  16000 ,
  32000 ,
  64000 ,
  128000,
  11025 ,
  22050 ,
  44100 ,
  88200 ,
  176400,
  12000 ,
  24000 ,
  48000 ,
  96000 ,
  192000
};

CAEStreamInfo::CAEStreamInfo() :
  m_bufferSize(0),
  m_skipBytes (0),
  m_syncFunc  (&CAEStreamInfo::DetectType),
  m_hasSync   (false),
  m_dataType  (STREAM_TYPE_NULL),
  m_packFunc  (NULL)
{
  m_dllAvUtil.Load();
}

CAEStreamInfo::~CAEStreamInfo()
{
  m_dllAvUtil.Unload();
}

int CAEStreamInfo::AddData(uint8_t *data, unsigned int size, uint8_t **buffer/* = NULL */, unsigned int *bufferSize/* = 0 */)
{
  if (size == 0)
  {
    if (bufferSize)
      *bufferSize = 0;
    return 0;
  }

  unsigned int consumed = 0;
  if(m_skipBytes)
  {
    unsigned int canSkip = std::min(size, m_skipBytes);
    unsigned int room    = sizeof(m_buffer) - m_bufferSize;
    unsigned int copy    = std::min(room, canSkip);

    memcpy(m_buffer + m_bufferSize, data, copy);
    m_bufferSize += copy;
    m_skipBytes  -= copy;

    if (m_skipBytes)
    {
      if (bufferSize)
        *bufferSize = 0;
      return copy;
    }

    GetPacket(buffer, bufferSize);
    return copy;
  }
  else
  {
    unsigned int offset = 0;
    unsigned int room = sizeof(m_buffer) - m_bufferSize;
    while(1)
    {
      if (!size)
      {
        if (bufferSize)
          *bufferSize = 0;
        return consumed;
      }

      unsigned int copy = std::min(room, size);
      memcpy(m_buffer + m_bufferSize, data, copy);
      m_bufferSize += copy;
      consumed     += copy;
      data         += copy;
      size         -= copy;
      room         -= copy;

      offset = (this->*m_syncFunc)(m_buffer, m_bufferSize);
      if (m_hasSync) break;
      else
      {
        /* lost sync */
        m_syncFunc = &CAEStreamInfo::DetectType;
        m_dataType = STREAM_TYPE_NULL;
        m_packFunc = NULL;
        m_repeat   = 1;

        /* if the buffer is full, or the offset < the buffer size */
        if (m_bufferSize == sizeof(m_buffer) || offset < m_bufferSize)
        {
          m_bufferSize -= offset;
          room         += offset;
          memmove(m_buffer, m_buffer + offset, m_bufferSize);
        }
      }
    }   

    /* if we got here, we acquired sync on the buffer */

    /* align the buffer */
    if (offset)
    {
      m_bufferSize -= offset;
      memmove(m_buffer, m_buffer + offset, m_bufferSize);
    }

    /* bytes to skip until the next packet */
    m_skipBytes = std::max(0, (int)m_fsize - (int)m_bufferSize);
    if (m_skipBytes)
    {
      if (bufferSize)
        *bufferSize = 0;
      return consumed;
    }

    GetPacket(buffer, bufferSize);
    return consumed;
  }
}

void CAEStreamInfo::GetPacket(uint8_t **buffer, unsigned int *bufferSize)
{
  /* if the caller wants the packet */
  if (buffer)
  {
    /* make sure the buffer is allocated and big enough */
    if (!*buffer || !bufferSize || *bufferSize < m_fsize)
    {
      delete[] *buffer;
      *buffer = new uint8_t[m_fsize];
    }

    /* copy the data into the buffer and update the size */
    memcpy(*buffer, m_buffer, m_fsize);
    if (bufferSize)
      *bufferSize = m_fsize;
  }

  /* remove the parsed data from the buffer */
  m_bufferSize -= m_fsize;
  memmove(m_buffer, m_buffer + m_fsize, m_bufferSize);
}

/* SYNC FUNCTIONS */

/*
  This function looks for sync words across the types in paralell, and only does an exhaustive
  test if it finds a syncword. Once sync has been established, the relevent sync function sets
  m_syncFunc to itself. This function will only be called again if total sync is lost, which 
  allows is to switch stream types on the fly much like a real reicever does.
*/
unsigned int CAEStreamInfo::DetectType(uint8_t *data, unsigned int size)
{
  unsigned int skipped  = 0;
  unsigned int possible = 0;

  while(size > 0) {
    /* if it could be AC3 */
    if (size > 6 && data[0] == 0x0b && data[1] == 0x77)
    {
      unsigned int skip = SyncAC3(data, size);
      if (m_hasSync)
        return skipped + skip;
      else
        possible = skipped;
    }

    /* if it could be DTS */
    if (size > 8)
    {
      unsigned int header = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
      if (header == DTS_PREAMBLE_14LE ||
          header == DTS_PREAMBLE_14BE ||
          header == DTS_PREAMBLE_16LE || 
          header == DTS_PREAMBLE_16BE)
      {
        unsigned int skip = SyncDTS(data, size);
        if (m_hasSync)
          return skipped + skip;
        else
          possible = skipped;
      }
    }

    /* move along one byte */
    --size;
    ++skipped;
    ++data;
  }

  return possible ? possible : skipped;
}

unsigned int CAEStreamInfo::SyncAC3(uint8_t *data, unsigned int size)
{
  unsigned int skip = 0;

  for(; size - skip > 6; ++skip, ++data)
  {
    /* search for an ac3 sync word */
    if(data[0] != 0x0b || data[1] != 0x77)
      continue;

    uint8_t bsid = data[5] >> 3;
    if (bsid > 0x11)
      continue;

    if (bsid <= 10)
    {
      /* Normal AC-3 */

      uint8_t fscod      = data[4] >> 6;
      uint8_t frmsizecod = data[4] & 0x3F;
      if (fscod == 3 || frmsizecod > 37)
        continue;

      /* get the details we need to check crc1 and framesize */
      unsigned int bitRate = AC3Bitrates[frmsizecod >> 1];
      unsigned int framesize = 0;
      switch(fscod)
      {
        case 0: framesize = bitRate * 2; break;
        case 1: framesize = (320 * bitRate / 147 + (frmsizecod & 1 ? 1 : 0)); break;
        case 2: framesize = bitRate * 4; break;
      }

      m_fsize = framesize << 1;
      m_sampleRate = AC3FSCod[fscod];

      /* dont do extensive testing if we have not lost sync */
      if (m_dataType == STREAM_TYPE_AC3 && skip == 0)
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
      m_hasSync  = true;
      m_syncFunc = &CAEStreamInfo::SyncAC3;
      m_dataType = STREAM_TYPE_AC3;
      m_packFunc = &CAEPackIEC958::PackAC3;
      m_repeat   = 1;

      CLog::Log(LOGINFO, "CAEStreamInfo::SyncAC3 - AC3 stream detected (%dHz)", m_sampleRate);
      return skip;
    }
    else
    {
      /* Enhanced AC-3 */
      uint8_t strmtyp = data[2] >> 6;
      if (strmtyp == 3) continue;

      unsigned int framesize = (((data[2] & 0x7) << 8) | data[3]) + 1;
      uint8_t      fscod     = (data[4] >> 6) & 0x3;
      uint8_t      cod       = (data[4] >> 4) & 0x3;
      uint8_t      blocks;

      if (fscod == 0x3)
      {
        if (cod == 0x3)
          continue;

        blocks       = 6;
        m_sampleRate = AC3FSCod[cod] >> 1;
      }
      else
      {
        blocks       = AC3BlkCod[cod  ];
        m_sampleRate = AC3FSCod [fscod];
      }

      m_fsize        = framesize << 1;
      m_repeat       = MAX_EAC3_BLOCKS / blocks;
      m_sampleRate <<= 2; /* E-AC-3 uses 4x samplerate */

      if (m_dataType == STREAM_TYPE_EAC3 && m_hasSync && skip == 0)
        return 0;

      /* if we get here, we can sync */
      m_hasSync  = true;
      m_syncFunc = &CAEStreamInfo::SyncAC3;
      m_dataType = STREAM_TYPE_EAC3;
      m_packFunc = &CAEPackIEC958::PackEAC3;

      CLog::Log(LOGINFO, "CAEStreamInfo::SyncAC3 - E-AC3 stream detected (%dHz)", m_sampleRate >> 2);
      return skip;
    }
  }

  /* if we get here, the entire packet is invalid and we have lost sync */
  CLog::Log(LOGINFO, "CAEStreamInfo::SyncAC3 - AC3 sync lost");
  m_hasSync = false;
  return size;
}

unsigned int CAEStreamInfo::SyncDTS(uint8_t *data, unsigned int size)
{
  unsigned int skip = 0;

  for(; size - skip > 9; ++skip, ++data)
  {
    unsigned int header = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    bool match = true;
    unsigned int blocks;
    unsigned int srate_code;

    switch(header)
    {
      /* 14bit BE */
      case DTS_PREAMBLE_14BE:
        if (data[4] != 0x07 || data[5] != 0xf0)
        {
          match = false;
          break;
        }
        blocks = ((data[5] & 0x7) << 4) | ((data[6] & 0x3f) >> 2);
        srate_code = (data[8] >> 2) & 0xf;
        m_dataIsLE = false;
        break;

      /* 14bit LE */
      case DTS_PREAMBLE_14LE:
        if (data[5] != 0x07 || data[4] != 0xf0)
        {
          match = false;
          break;
        }
        blocks = ((data[4] & 0x7) << 4) | ((data[7] & 0x3f) >> 2);
        srate_code = (data[9] >> 2) & 0xf;
        m_dataIsLE = true;
        break;

      /* 16bit BE */
      case DTS_PREAMBLE_16BE:
        m_dataIsLE = false;
        blocks = (data[5] >> 2) & 0x7f;
        srate_code = (data[8] >> 2) & 0xf;
        m_dataIsLE = false;
        break;

      /* 16bit LE */
      case DTS_PREAMBLE_16LE:
        m_dataIsLE = true;
        blocks = (data[4] >> 2) & 0x7f;
        srate_code = (data[9] >> 2) & 0xf;
        m_dataIsLE = true;
        break;


      default:
        match = false;
        break;      
    }

    if (!match || srate_code == 0 || srate_code >= DTS_SRATE_COUNT) continue;

    if (m_dataIsLE)
    {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((data[5] & 0x80) == 0x80 && (data[5] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      m_fsize = ((((data[4] & 0x3) << 8 | data[7]) << 4) | ((data[6] & 0xF0) >> 4)) + 1; 
   }
   else
   {
      /* if it is not a termination frame, check the next 6 bits are set */
      if ((data[4] & 0x80) == 0x80 && (data[4] & 0x7C) != 0x7C)
        continue;

      /* get the frame size */
      m_fsize = ((((data[5] & 0x3) << 8 | data[6]) << 4) | ((data[7] & 0xF0) >> 4)) + 1;
   }

    /* make sure the framesize is sane */
    if (m_fsize < 96 || m_fsize > 16384)
      continue;

    bool invalid = false;
    DataType dataType;
    switch((blocks + 1) << 5)
    {
      case 512 : dataType = STREAM_TYPE_DTS_512 ; m_packFunc = &CAEPackIEC958::PackDTS_512 ; break;
      case 1024: dataType = STREAM_TYPE_DTS_1024; m_packFunc = &CAEPackIEC958::PackDTS_1024; break;
      case 2048: dataType = STREAM_TYPE_DTS_2048; m_packFunc = &CAEPackIEC958::PackDTS_2048; break;
      default:
        invalid = true;
        break;
    }

    if (invalid)
      continue;

    unsigned int sampleRate = DTSSampleRates[srate_code];
    if (!m_hasSync || skip || dataType != m_dataType || sampleRate != m_sampleRate)
    {
      m_hasSync    = true;
      m_dataType   = dataType;
      m_sampleRate = sampleRate;
      m_syncFunc   = &CAEStreamInfo::SyncDTS;
      m_repeat     = 1;

      CLog::Log(LOGINFO, "CAEStreamInfo::SyncDTS - DTS stream detected (%dHz)", m_sampleRate);
    }

    return skip;
  }

  /* lost sync */
  CLog::Log(LOGINFO, "CAEStreamInfo::SyncDTS - DTS sync lost");
  m_hasSync = false;
  return size;
}

