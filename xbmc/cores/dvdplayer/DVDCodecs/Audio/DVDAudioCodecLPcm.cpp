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

#include "DVDAudioCodecLPcm.h"
#include "DVDStreamInfo.h"

CDVDAudioCodecLPcm::CDVDAudioCodecLPcm() : CDVDAudioCodecPcm()
{
  m_codecID = CODEC_ID_NONE;
  m_bufferSize = LPCM_BUFFER_SIZE;
  memset(m_buffer, 0, sizeof(m_buffer));
}

bool CDVDAudioCodecLPcm::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  m_codecID = hints.codec;

  CDVDStreamInfo hints2(hints, true);
  hints2.codec = CODEC_ID_NONE;
#if 0
  if (hints.codecID = CODEC_ID_LPCM_S24BE) hints2.codec = CODEC_ID_PCM_S24BE;
#endif
  if (hints2.codec != CODEC_ID_NONE)
    return CDVDAudioCodecPcm::Open(hints2, options);

  return false;
}

int CDVDAudioCodecLPcm::Decode(BYTE* pData, int iSize)
{
  BYTE* d = m_buffer;
  BYTE* s = pData;
  int iDecoded = 0;

  if (iSize > m_bufferSize) iSize = m_bufferSize;

  if (iSize >= 12)
  {
#if 0
    if (m_codecID == CODEC_ID_LPCM_S24BE)
#endif
    {
      for (iDecoded = 0; iDecoded <= (iSize - 12); iDecoded += 12)
      {
        // first sample
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[8];
        // second sample
        d[3] = s[2];
        d[4] = s[3];
        d[5] = s[9];
        // third sample
        d[6] = s[4];
        d[7] = s[5];
        d[8] = s[10];
        // fourth sample
        d[9] = s[6];
        d[10] = s[7];
        d[11] = s[11];

        s += 12;
        d += 12;
      }
    }

    return CDVDAudioCodecPcm::Decode(m_buffer, iDecoded);
  }

  return iSize;
}
