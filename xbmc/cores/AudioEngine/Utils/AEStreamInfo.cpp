/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AEStreamInfo.h"

#define IEC61937_PREAMBLE1 0xF872
#define IEC61937_PREAMBLE2 0x4E1F
#define DTS_PREAMBLE_14BE  0x1FFFE800
#define DTS_PREAMBLE_14LE  0xFF1F00E8
#define DTS_PREAMBLE_16BE  0x7FFE8001
#define DTS_PREAMBLE_16LE  0xFE7F0180
#define DTS_PREAMBLE_HD    0x64582025
#define DTS_SFREQ_COUNT    16
#define MAX_EAC3_BLOCKS    6

static enum AEChannel OutputMaps[2][9] = {
  {AE_CH_RAW, AE_CH_RAW, AE_CH_NULL},
  {AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_RAW, AE_CH_NULL}
};

static const uint16_t AC3Bitrates   [] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640};
static const uint16_t AC3FSCod      [] = {48000, 44100, 32000, 0};
static const uint8_t  AC3BlkCod     [] = {1, 2, 3, 6};
static const uint8_t  AC3Channels   [] = {2, 1, 2, 3, 3, 4, 4, 5};
static const uint8_t  DTSChannels   [] = {1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8};
static const uint8_t  THDChanMap    [] = {2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 1, 1};

static const uint32_t DTSSampleRates[DTS_SFREQ_COUNT] =
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
  m_bufferSize    (0),
  m_skipBytes     (0),
  m_coreOnly      (false),
  m_needBytes     (0),
  m_syncFunc      (&CAEStreamInfo::DetectType),
  m_hasSync       (false),
  m_sampleRate    (0),
  m_outputRate    (0),
  m_outputChannels(0),
  m_channelMap    (CAEChannelInfo()),
  m_channels      (0),
  m_coreSize      (0),
  m_dtsBlocks     (0),
  m_dtsPeriod     (0),
  m_fsize         (0),
  m_repeat        (0),
  m_substreams    (0),
  m_dataType      (STREAM_TYPE_NULL),
  m_dataIsLE      (false),
  m_packFunc      (NULL)
{
  m_dllAvUtil.Load();
  m_dllAvUtil.av_crc_init(m_crcTrueHD, 0, 16, 0x2D, sizeof(m_crcTrueHD));
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
  if (m_skipBytes)
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

      if (m_needBytes > m_bufferSize)
        continue;

      m_needBytes = 0;
      offset      = (this->*m_syncFunc)(m_buffer, m_bufferSize);

      if (m_hasSync || m_needBytes)
        break;
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

    if (!m_needBytes)
      GetPacket(buffer, bufferSize);

    return consumed;
  }
}

void CAEStreamInfo::GetPacket(uint8_t **buffer, unsigned int *bufferSize)
{
  /* if the caller wants the packet */
  if (buffer)
  {
    /* if it is dtsHD and we only want the core, just fetch that */
    unsigned int size = m_fsize;
    if (m_dataType == STREAM_TYPE_DTSHD_CORE)
      size = m_coreSize;

    /* make sure the buffer is allocated and big enough */
    if (!*buffer || !bufferSize || *bufferSize < size)
    {
      delete[] *buffer;
      *buffer = new uint8_t[size];
    }

    /* copy the data into the buffer and update the size */
    memcpy(*buffer, m_buffer, size);
    if (bufferSize)
      *bufferSize = size;
  }

  /* remove the parsed data from the buffer */
  m_bufferSize -= m_fsize;
  memmove(m_buffer, m_buffer + m_fsize, m_bufferSize);
  m_fsize = 0;
  m_coreSize = 0;
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

  while (size > 8)
  {
    /* if it could be DTS */
    unsigned int header = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    if (header == DTS_PREAMBLE_14LE ||
        header == DTS_PREAMBLE_14BE ||
        header == DTS_PREAMBLE_16LE ||
        header == DTS_PREAMBLE_16BE)
    {
      unsigned int skip = SyncDTS(data, size);
      if (m_hasSync || m_needBytes)
        return skipped + skip;
      else
        possible = skipped;
    }

    /* if it could be AC3 */
    if (data[0] == 0x0b && data[1] == 0x77)
    {
      unsigned int skip = SyncAC3(data, size);
      if (m_hasSync)
        return skipped + skip;
      else
        possible = skipped;
    }

    /* if it could be TrueHD */
    if (data[4] == 0xf8 && data[5] == 0x72 && data[6] == 0x6f && data[7] == 0xba)
    {
      unsigned int skip = SyncTrueHD(data, size);
      if (m_hasSync)
        return skipped + skip;
      else
        possible = skipped;
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

  for (; size - skip > 7; ++skip, ++data)
  {
    /* search for an ac3 sync word */
    if (data[0] != 0x0b || data[1] != 0x77)
      continue;

    uint8_t bsid  = data[5] >> 3;
    uint8_t acmod = data[6] >> 5;
    uint8_t lfeon;

    int8_t pos = 4;
    if ((acmod & 0x1) && (acmod != 0x1))
      pos -= 2;
    if (acmod & 0x4 )
      pos -= 2;
    if (acmod == 0x2)
      pos -= 2;
    if (pos < 0)
      lfeon = (data[7] & 0x64) ? 1 : 0;
    else
      lfeon = ((data[6] >> pos) & 0x1) ? 1 : 0;

    if (bsid > 0x11 || acmod > 7)
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
      switch (fscod)
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
      else 
        crc_size = (framesize >> 1) + (framesize >> 3) - 1;

      if (crc_size <= size - skip)
        if (m_dllAvUtil.av_crc(m_dllAvUtil.av_crc_get_table(AV_CRC_16_ANSI), 0, &data[2], crc_size * 2))
          continue;

      /* if we get here, we can sync */
      m_hasSync        = true;
      m_outputRate     = m_sampleRate;
      m_outputChannels = 2;
      m_channelMap     = CAEChannelInfo(OutputMaps[0]);
      m_channels       = AC3Channels[acmod] + lfeon;
      m_syncFunc       = &CAEStreamInfo::SyncAC3;
      m_dataType       = STREAM_TYPE_AC3;
      m_packFunc       = &CAEPackIEC61937::PackAC3;
      m_repeat         = 1;

      CLog::Log(LOGINFO, "CAEStreamInfo::SyncAC3 - AC3 stream detected (%d channels, %dHz)", m_channels, m_sampleRate);
      return skip;
    }
    else
    {
      /* Enhanced AC-3 */
      uint8_t strmtyp = data[2] >> 6;
      if (strmtyp == 3)
        continue;

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

      if (m_sampleRate == 48000 || m_sampleRate == 96000 || m_sampleRate == 192000)
        m_outputRate = 192000;
      else
        m_outputRate = 176400;

      if (m_dataType == STREAM_TYPE_EAC3 && m_hasSync && skip == 0)
        return 0;

      /* if we get here, we can sync */
      m_hasSync        = true;
      m_outputChannels = 8;
      m_channelMap     = CAEChannelInfo(OutputMaps[1]);
      m_channels       = 8; /* FIXME: this should be read out of the stream */
      m_syncFunc       = &CAEStreamInfo::SyncAC3;
      m_dataType       = STREAM_TYPE_EAC3;
      m_packFunc       = &CAEPackIEC61937::PackEAC3;

      CLog::Log(LOGINFO, "CAEStreamInfo::SyncAC3 - E-AC3 stream detected (%d channels, %dHz)", m_channels, m_sampleRate);
      return skip;
    }
  }

  /* if we get here, the entire packet is invalid and we have lost sync */
  CLog::Log(LOGINFO, "CAEStreamInfo::SyncAC3 - AC3 sync lost");
  m_hasSync = false;
  return skip;
}

unsigned int CAEStreamInfo::SyncDTS(uint8_t *data, unsigned int size)
{
  if (size < 13)
  {
    if (m_needBytes < 13)
      m_needBytes = 14;
    return 0;
  }

  unsigned int skip = 0;
  for (; size - skip > 13; ++skip, ++data)
  {
    unsigned int header = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    unsigned int hd_sync = 0;
    bool match = true;
    unsigned int dtsBlocks;
    unsigned int amode;
    unsigned int sfreq;
    unsigned int lfe;
    int bits;

    switch (header)
    {
      /* 14bit BE */
      case DTS_PREAMBLE_14BE:
        if (data[4] != 0x07 || (data[5] & 0xf0) != 0xf0)
        {
          match = false;
          break;
        }
        dtsBlocks = (((data[5] & 0x7) << 4) | ((data[6] & 0x3C) >> 2)) + 1;
        m_fsize     = ((((data[6] & 0x3 << 8) | data[7]) << 4) | ((data[8] & 0x3C) >> 2)) + 1;
        amode       = ((data[8] & 0x3) << 4) | ((data[9] & 0xF0) >> 4);
        sfreq       = data[9] & 0xF;
        lfe         = (data[12] & 0x18) >> 3;
        m_dataIsLE  = false;
        bits        = 14;
        break;

      /* 14bit LE */
      case DTS_PREAMBLE_14LE:
        if (data[5] != 0x07 || (data[4] & 0xf0) != 0xf0)
        {
          match = false;
          break;
        }
        dtsBlocks = (((data[4] & 0x7) << 4) | ((data[7] & 0x3C) >> 2)) + 1;
        m_fsize     = ((((data[7] & 0x3 << 8) | data[6]) << 4) | ((data[9] & 0x3C) >> 2)) + 1;
        amode       = ((data[9] & 0x3) << 4) | ((data[8] & 0xF0) >> 4);
        sfreq       = data[8] & 0xF;
        lfe         = (data[13] & 0x18) >> 3;
        m_dataIsLE  = true;
        bits        = 14;
        break;

      /* 16bit BE */
      case DTS_PREAMBLE_16BE:
        dtsBlocks = (((data[4] & 0x1) << 7) | ((data[5] & 0xFC) >> 2)) + 1;
        m_fsize     = (((((data[5] & 0x3) << 8) | data[6]) << 4) | ((data[7] & 0xF0) >> 4)) + 1;
        amode       = ((data[7] & 0x0F) << 2) | ((data[8] & 0xC0) >> 6);
        sfreq       = (data[8] & 0x3C) >> 2;
        lfe         = (data[10] >> 1) & 0x3;
        m_dataIsLE  = false;
        bits        = 16;
        break;

      /* 16bit LE */
      case DTS_PREAMBLE_16LE:
        dtsBlocks = (((data[5] & 0x1) << 7) | ((data[4] & 0xFC) >> 2)) + 1;
        m_fsize     = (((((data[4] & 0x3) << 8) | data[7]) << 4) | ((data[6] & 0xF0) >> 4)) + 1;
        amode       = ((data[6] & 0x0F) << 2) | ((data[9] & 0xC0) >> 6);
        sfreq       = (data[9] & 0x3C) >> 2;
        lfe         = (data[11] >> 1) & 0x3;
        m_dataIsLE  = true;
        bits        = 16;
        break;

      default:
        match = false;
        break;
    }

    if (!match || sfreq == 0 || sfreq >= DTS_SFREQ_COUNT)
      continue;

    /* make sure the framesize is sane */
    if (m_fsize < 96 || m_fsize > 16384)
      continue;

    bool invalid = false;
    DataType dataType;
    switch (dtsBlocks << 5)
    {
      case 512 : dataType = STREAM_TYPE_DTS_512 ; break;
      case 1024: dataType = STREAM_TYPE_DTS_1024; break;
      case 2048: dataType = STREAM_TYPE_DTS_2048; break;
      default:
        invalid = true;
        break;
    }

    if (invalid)
      continue;

    /* adjust the fsize for 14 bit streams */
    if (bits == 14)
      m_fsize = m_fsize / 14 * 16;

    /* we need enough data to check for DTS-HD */
    if (size - skip < m_fsize + 10)
    {
      /* we can assume DTS sync at this point */
      m_syncFunc  = &CAEStreamInfo::SyncDTS;
      m_needBytes = m_fsize + 10;
      m_fsize     = 0;

      return skip;
    }

    /* look for DTS-HD */
    hd_sync = (data[m_fsize] << 24) | (data[m_fsize + 1] << 16) | (data[m_fsize + 2] << 8) | data[m_fsize + 3];
    if (hd_sync == DTS_PREAMBLE_HD)
    {
      int hd_size;
      bool blownup = (data[m_fsize + 5] & 0x20) != 0;
      if (blownup)
        hd_size = (((data[m_fsize + 6] & 0x01) << 19) | (data[m_fsize + 7] << 11) | (data[m_fsize + 8] << 3) | ((data[m_fsize + 9] & 0xe0) >> 5)) + 1;
      else
        hd_size = (((data[m_fsize + 6] & 0x1f) << 11) | (data[m_fsize + 7] << 3) | ((data[m_fsize + 8] & 0xe0) >> 5)) + 1;

      /* set the type according to core or not */
      if (m_coreOnly)
        dataType = STREAM_TYPE_DTSHD_CORE;
      else
        dataType = STREAM_TYPE_DTSHD;

      m_coreSize  = m_fsize;
      m_fsize    += hd_size;
    }

    unsigned int sampleRate = DTSSampleRates[sfreq];
    if (!m_hasSync || skip || dataType != m_dataType || sampleRate != m_sampleRate || dtsBlocks != m_dtsBlocks)
    {
      m_hasSync        = true;
      m_dataType       = dataType;
      m_sampleRate     = sampleRate;
      m_dtsBlocks      = dtsBlocks;
      m_channels       = DTSChannels[amode] + (lfe ? 1 : 0);
      m_syncFunc       = &CAEStreamInfo::SyncDTS;
      m_repeat         = 1;

      if (dataType == STREAM_TYPE_DTSHD)
      {
        m_outputRate     = 192000;
        m_outputChannels = 8;
        m_channelMap     = CAEChannelInfo(OutputMaps[1]);
        m_channels      += 2; /* FIXME: this needs to be read out, not sure how to do that yet */
      }
      else
      {
        m_outputRate     = m_sampleRate;
        m_outputChannels = 2;
        m_channelMap     = CAEChannelInfo(OutputMaps[0]);
      }

      std::string type;
      switch (dataType)
      {
        case STREAM_TYPE_DTSHD     : type = "dtsHD"; break;
        case STREAM_TYPE_DTSHD_CORE: type = "dtsHD (core)"; break;
        default                    : type = "dts"; break;
      }

      /* calculate the period size for dtsHD */
      m_dtsPeriod = (m_outputRate * (m_outputChannels >> 1)) * (m_dtsBlocks << 5) / m_sampleRate;

      CLog::Log(LOGINFO, "CAEStreamInfo::SyncDTS - %s stream detected (%d channels, %dHz, %dbit %s, period: %u)",
                type.c_str(), m_channels, m_sampleRate,
                bits, m_dataIsLE ? "LE" : "BE",
                m_dtsPeriod);
    }

    return skip;
  }

  /* lost sync */
  CLog::Log(LOGINFO, "CAEStreamInfo::SyncDTS - DTS sync lost");
  m_hasSync = false;
  return skip;
}

inline unsigned int CAEStreamInfo::GetTrueHDChannels(const uint16_t chanmap)
{
  int channels = 0;
  for (int i = 0; i < 13; ++i)
    channels += THDChanMap[i] * ((chanmap >> i) & 1);
  return channels;
}

unsigned int CAEStreamInfo::SyncTrueHD(uint8_t *data, unsigned int size)
{
  unsigned int left = size;
  unsigned int skip = 0;

  /* if MLP */
  for (; left; ++skip, ++data, --left)
  {
    /* if we dont have sync and there is less the 8 bytes, then break out */
    if (!m_hasSync && left < 8)
      return size;

    /* if its a major audio unit */
    uint16_t length   = ((data[0] & 0x0F) << 8 | data[1]) << 1;
    uint32_t syncword = ((((data[4] << 8 | data[5]) << 8) | data[6]) << 8) | data[7];
    if (syncword == 0xf8726fba)
    {
      /* we need 32 bytes to sync on a master audio unit */
      if (left < 32)
        return skip;

      /* get the rate and ensure its valid */
      int rate = (data[8] & 0xf0) >> 4;
      if (rate == 0xF)
        continue;

      /* verify the crc of the audio unit */
      uint16_t crc = m_dllAvUtil.av_crc(m_crcTrueHD, 0, data + 4, 24);
      crc ^= (data[29] << 8) | data[28];
      if (((data[31] << 8) | data[30]) != crc)
        continue;

      /* get the sample rate and substreams, we have a valid master audio unit */
      m_sampleRate = (rate & 0x8 ? 44100 : 48000) << (rate & 0x7);
      m_substreams = (data[20] & 0xF0) >> 4;

      /* get the number of encoded channels */
      uint16_t channel_map = ((data[10] & 0x1F) << 8) | data[11];
      if (!channel_map)
        channel_map = (data[9] << 1) | (data[10] >> 7);
      m_channels = CAEStreamInfo::GetTrueHDChannels(channel_map);

      if (m_sampleRate == 48000 || m_sampleRate == 96000 || m_sampleRate == 192000)
        m_outputRate = 192000;
      else
        m_outputRate = 176400;

      if (!m_hasSync)
        CLog::Log(LOGINFO, "CAEStreamInfo::SyncTrueHD - TrueHD stream detected (%d channels, %dHz)", m_channels, m_sampleRate);

      m_hasSync        = true;
      m_fsize          = length;
      m_dataType       = STREAM_TYPE_TRUEHD;
      m_outputChannels = 8;
      m_channelMap     = CAEChannelInfo(OutputMaps[1]);
      m_syncFunc       = &CAEStreamInfo::SyncTrueHD;
      m_packFunc       = &CAEPackIEC61937::PackTrueHD;
      m_repeat         = 1;
      return skip;
    }
    else
    {
      /* we cant sink to a subframe until we have the information from a master audio unit */
      if (!m_hasSync)
        continue;

      /* if there is not enough data left to verify the packet, just return the skip amount */
      if (left < (unsigned int)m_substreams * 4)
        return skip;

      /* verify the parity */
      int     p     = 0;
      uint8_t check = 0;
      for (int i = -1; i < m_substreams; ++i)
      {
        check ^= data[p++];
        check ^= data[p++];
        if (i == -1 || data[p - 2] & 0x80)
        {
          check ^= data[p++];
          check ^= data[p++];
        }
      }

      /* if the parity nibble does not match */
      if ((((check >> 4) ^ check) & 0xF) != 0xF)
      {
        /* lost sync */
        m_hasSync  = false;
        CLog::Log(LOGINFO, "CAEStreamInfo::SyncTrueHD - Sync Lost");
        continue;
      }
      else
      {
        m_fsize = length;
        return skip;
      }
    }
  }

  /* lost sync */
  m_hasSync  = false;
  return skip;
}

