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

#include "MusicInfoTagLoaderAAC.h"
#include "File.h"

#define PACK_UINT32(a,b,c,d) \
  ((((uint32_t)a) << 24) | \
   (((uint32_t)b) << 16) | \
   (((uint32_t)c) <<  8) | \
   (((uint32_t)d)))

#define PACK_UINT64(a,b,c,d,e,f,g,h) \
  ((((uint64_t)PACK_UINT32(a,b,c,d)) << 32) | \
   (((uint64_t)PACK_UINT32(e,f,g,h))))

const static int aac_sample_rates[] = {96000,88200,64000,48000,44100,32000,
                                       24000,22050,16000,12000,11025,8000};

using namespace MUSIC_INFO;

CMusicInfoTagLoaderAAC::CMusicInfoTagLoaderAAC(void)
{}

CMusicInfoTagLoaderAAC::~CMusicInfoTagLoaderAAC()
{}

int CMusicInfoTagLoaderAAC::ReadDuration(const CStdString& strFileName)
{
  int duration  = 0;
  XFILE::CFile file;
  if (file.Open(strFileName))
  {
    int tagOffset = ReadID3Length(file);

    if ((duration = ReadMP4Duration(file, tagOffset, 0)))
    {}
    else if ((duration = ReadADTSDuration(file, tagOffset)))
    {}
    else if ((duration = ReadADIFDuration(file, tagOffset)))
    {}
    
    file.Close();
  }

  return duration;
}

int CMusicInfoTagLoaderAAC::ReadID3Length(XFILE::CFile& file)
{
  char  buf[10]   = {};
  int   tagLength = 0;
  while (file.Read(buf, 10) == 10)
  {
    if (memcmp(buf, "ID3", 3))
      break;
    tagLength += ((buf[6] << 21) | (buf[7] << 14) | (buf[8] << 7) | buf[9]) + 10;
    file.Seek(tagLength);
  }

  return 0;
}

int CMusicInfoTagLoaderAAC::ReadADTSDuration(XFILE::CFile& file, int offset)
{
  uint8_t buf[10]       = {};
  uint64_t totalLength  = 0;
  uint32_t frames       = 0;
  float framesPerSec    = 0.f;

  file.Seek(offset);

  while (file.Read(buf, 10) == 10)
  {
    if ((buf[0] == 0xFF) && ((buf[1] & 0xF6) == 0xF0))
    {
      if (!frames)
      {
        int sr_idx = (buf[2] & 0x3C) >> 2;
        framesPerSec = aac_sample_rates[sr_idx] / 1024.f;
      }
      frames++;
      totalLength += ((((uint32_t)buf[3] & 0x03) << 11) | (((uint32_t)buf[4]) << 3) | buf[5] >> 5);
      file.Seek(totalLength + offset);
    }
    else
      return 0;
  }
  return ((int)framesPerSec) ? (int)((float)frames/framesPerSec) : 0;
}

int CMusicInfoTagLoaderAAC::ReadADIFDuration(XFILE::CFile& file, int offset)
{
  uint8_t buf[17]   = {};
  int skip          = 0;
  uint32_t bitrate  = 0;
  int64_t fileLen  = 0;

  file.Seek(offset);

  if (file.Read(buf, 17) == 17)
  {
    if (PACK_UINT32('A', 'D', 'I', 'F') == PACK_UINT32(buf[0], buf[1], buf[2], buf[3]))
    {
      skip = (buf[4] & 0x80) ? 9 : 0;
      bitrate |= (((uint32_t)buf[4 + skip] & 0x0F) << 19) |
                 ( (uint32_t)buf[5 + skip] << 11) |
                 ( (uint32_t)buf[6 + skip] << 3) |
                 (((uint32_t)buf[7 + skip] & 0xE0) >> 5);
      fileLen = std::max((int64_t)0, file.GetLength());
    }
  }
  return (bitrate) ? (int)(((float)fileLen*8.f)/(float)(bitrate)) : 0;
}

int CMusicInfoTagLoaderAAC::ReadMP4Duration(XFILE::CFile& file, int64_t position, int64_t endPosition)
{
  uint8_t buf[8] = {};

  if (!endPosition)
    file.Seek(position);

  while ((file.Read(buf, 8) == 8) && (endPosition ? position < endPosition : 1))
  {
    uint64_t  atom_size   = PACK_UINT32(buf[0], buf[1], buf[2], buf[3]);
    uint8_t   header_size = 8;
    if (atom_size == 1)
    {
        header_size = 16;
      uint8_t buf2[8] = {};
      if (file.Read(buf2, 8) == 8)
      {
        atom_size   = PACK_UINT64(buf2[0], buf2[1], buf2[2], buf2[3],
                                buf2[4], buf2[5], buf2[6], buf2[7]);
      }
      else
        return 0;
    }

    switch (PACK_UINT32(buf[4], buf[5], buf[6], buf[7]))
    {
      case PACK_UINT32('m', 'o', 'o', 'v'):
      case PACK_UINT32('t', 'r', 'a', 'k'):
      case PACK_UINT32('m', 'd', 'i', 'a'):
        return ReadMP4Duration(file, position + header_size, position + atom_size);
      case PACK_UINT32('m', 'd', 'h', 'd'):
      {
        uint8_t   data[20] = {};
        if (file.Read(data, 20) == 20)
        {
          uint32_t  duration    = 0;
          uint32_t  time_scale  = 0;
          time_scale  = PACK_UINT32(data[12], data[13], data[14], data[15]);
          duration    = PACK_UINT32(data[16], data[17], data[18], data[19]);
          return (time_scale) ? (int)((float)duration/(float)(time_scale)) : 0;
        }
        return 0;
      }
      default:
        break;
    }
    position += atom_size;
    if (!atom_size || file.Seek(position) != position)
      break;
  }
  return 0;
}
