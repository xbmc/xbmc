/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "EncoderWav.h"
#include "filesystem/File.h"
#include "utils/log.h"

CEncoderWav::CEncoderWav()
{
  m_iBytesWritten = 0;
}

bool CEncoderWav::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  m_iBytesWritten = 0;

  // we only accept 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16) return false;

  // set input stream information and open the file
  if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits)) return false;

  // write dummy header file
  WAVHDR dummyheader;
  memset(&dummyheader, 0, sizeof(dummyheader));
  FileWrite(&dummyheader, sizeof(dummyheader));

  return true;
}

int CEncoderWav::Encode(int nNumBytesRead, BYTE* pbtStream)
{
  // write stream to file (no conversion needed at this time)
  if (FileWrite(pbtStream, nNumBytesRead) == -1)
  {
    CLog::Log(LOGERROR, "Error writing buffer to file");
    return 0;
  }

  m_iBytesWritten += nNumBytesRead;
  return 1;
}

bool CEncoderWav::Close()
{
  WriteWavHeader();
  FileClose();
  return true;
}

bool CEncoderWav::WriteWavHeader()
{
  WAVHDR wav;
  int bps = 1;

  if (!m_file) return false;

  memcpy(wav.riff, "RIFF", 4);
  wav.len = m_iBytesWritten + 44 - 8;
  memcpy(wav.cWavFmt, "WAVEfmt ", 8);
  wav.dwHdrLen = 16;
  wav.wFormat = WAVE_FORMAT_PCM;
  wav.wNumChannels = m_iInChannels;
  wav.dwSampleRate = m_iInSampleRate;
  wav.wBitsPerSample = m_iInBitsPerSample;
  if (wav.wBitsPerSample == 16) bps = 2;
  wav.dwBytesPerSec = m_iInBitsPerSample * m_iInChannels * bps;
  wav.wBlockAlign = 4;
  memcpy(wav.cData, "data", 4);
  wav.dwDataLen = m_iBytesWritten;

  // write header to beginning of stream
  m_file->Seek(0, FILE_BEGIN);
  FileWrite(&wav, sizeof(wav));

  return true;
}
