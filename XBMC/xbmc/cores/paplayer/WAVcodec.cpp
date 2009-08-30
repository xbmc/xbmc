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
#include "WAVcodec.h"

// Use SDL macros to perform byte swapping on big-endian systems
// SDL_endian.h is already included in PlatformDefs.h
#ifndef HAS_SDL
#define SDL_SwapLE16(X) (X)
#define SDL_SwapLE32(X) (X)
#endif

#if defined(WIN32)
#include <mmreg.h>
#endif

typedef struct
{
  char chunk_id[4];
  DWORD chunksize;
} WAVE_CHUNK;

typedef struct
{
  char riff[4];
  DWORD filesize;
  char rifftype[4];
} WAVE_RIFFHEADER;

WAVCodec::WAVCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_iDataStart=0;
  m_iDataLen=0;
  m_Bitrate = 0;
  m_CodecName = "WAV";
}

WAVCodec::~WAVCodec()
{
  DeInit();
}

bool WAVCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_file.Open(strFile, READ_CACHED))
    return false;

  // read header
  WAVE_RIFFHEADER riffh;
  m_file.Read(&riffh, sizeof(WAVE_RIFFHEADER));
  riffh.filesize = SDL_SwapLE32(riffh.filesize);

  // file valid?
  if (strncmp(riffh.riff, "RIFF", 4)!=0 && strncmp(riffh.rifftype, "WAVE", 4)!=0)
    return false;

  unsigned long offset = 0, pos;
  offset += sizeof(WAVE_RIFFHEADER);
  offset -= sizeof(WAVE_CHUNK);

  // parse chunks
  do
  {
    WAVE_CHUNK chunk;

    // always seeking to the start of a chunk
    m_file.Seek(offset + sizeof(WAVE_CHUNK), SEEK_SET);
    m_file.Read(&chunk, sizeof(WAVE_CHUNK));
    chunk.chunksize = SDL_SwapLE32(chunk.chunksize);

    if (!strncmp(chunk.chunk_id, "fmt ", 4))
    {
      pos = (long)m_file.GetPosition();

      // format chunk
      WAVEFORMATEXTENSIBLE wfx;
      m_file.Read(&wfx, sizeof(WAVEFORMATEX));

      //  Get file info
      m_SampleRate    = SDL_SwapLE32(wfx.Format.nSamplesPerSec);
      m_Channels      = SDL_SwapLE16(wfx.Format.nChannels);
      m_BitsPerSample = SDL_SwapLE16(wfx.Format.wBitsPerSample);

      //  Is it an extensible wav file
      if ((SDL_SwapLE16(wfx.Format.wFormatTag) == WAVE_FORMAT_EXTENSIBLE) && (SDL_SwapLE16(wfx.Format.cbSize) >= 22))
      {
        m_file.Read(&wfx + sizeof(WAVEFORMATEX), sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX));
        m_ChannelMask = SDL_SwapLE32(wfx.dwChannelMask);
      } else {
        m_ChannelMask = 0;
      }

      m_file.Seek(pos + chunk.chunksize, SEEK_SET);
    }
    else if (!strncmp(chunk.chunk_id, "data", 4))
    { // data chunk
      m_iDataStart=(long)m_file.GetPosition();
      m_iDataLen=chunk.chunksize;

      if (chunk.chunksize & 1)
        offset++;
    }
    else
    { // other chunk - unused, just skip
      m_file.Seek(chunk.chunksize, SEEK_CUR);
    }

    offset+=(chunk.chunksize+sizeof(WAVE_CHUNK));

    if (offset & 1)
      offset++;

  } while (offset+(int)sizeof(WAVE_CHUNK) < riffh.filesize);

  m_TotalTime = (int)(((float)m_iDataLen/(m_SampleRate*m_Channels*(m_BitsPerSample/8)))*1000);
  m_Bitrate = (int)(((float)m_iDataLen * 8) / ((float)m_TotalTime / 1000));

  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0 || m_iDataStart==0 || m_iDataLen==0)
    return false;

  //  Seek to the start of the data chunk
  m_file.Seek(m_iDataStart);

  return true;
}

void WAVCodec::DeInit()
{
  m_file.Close();
}

__int64 WAVCodec::Seek(__int64 iSeekTime)
{
  //  Calculate size of a single sample of the file
  int iSampleSize=m_SampleRate*m_Channels*(m_BitsPerSample/8);

  //  Seek to the position in the file
  m_file.Seek(m_iDataStart+((iSeekTime/1000)*iSampleSize));

  return iSeekTime;
}

int WAVCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;
  int iPos=(int)m_file.GetPosition();
  if (iPos >= m_iDataStart+m_iDataLen)
    return READ_EOF;

  int iAmountRead=m_file.Read(pBuffer, size);
  if (iAmountRead>0)
  {
    *actualsize=iAmountRead;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool WAVCodec::CanInit()
{
  return true;
}
