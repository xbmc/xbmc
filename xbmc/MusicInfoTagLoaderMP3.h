#pragma once
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

#include <math.h>
#include "ImusicInfoTagLoader.h"
#include "cores/paplayer/ReplayGain.h"

namespace MUSIC_INFO
{

class CVBRMP3SeekHelper
{
public:
  CVBRMP3SeekHelper()
  {
    m_iSeekOffsets = 0;
    m_fTotalDuration = 0.0f;
    m_SeekOffset = NULL;
    m_iFirstSample = 0;
    m_iLastSample = 0;
  };
  virtual ~CVBRMP3SeekHelper()
  {
    delete[] m_SeekOffset;
  };

  int64_t GetByteOffset(float fTime)
  {
    if (!m_iSeekOffsets) return 0;  // no seek info
    if (fTime > m_fTotalDuration)
      fTime = m_fTotalDuration;
    float fOffset = (fTime / m_fTotalDuration) * m_iSeekOffsets;
    int iOffset = (int)floor(fOffset);
    if (iOffset > m_iSeekOffsets-1) iOffset = m_iSeekOffsets - 1;
    float fa = m_SeekOffset[iOffset];
    float fb = m_SeekOffset[iOffset + 1];
    return (int64_t)(fa + (fb - fa) * (fOffset - iOffset));
  };

  int64_t GetTimeOffset(int64_t iBytes)
  {
    if (!m_iSeekOffsets) return 0;  // no seek info
    float fBytes = (float)iBytes;
    if (fBytes > m_SeekOffset[m_iSeekOffsets])
      fBytes = m_SeekOffset[m_iSeekOffsets];
    if (fBytes < m_SeekOffset[0])
      fBytes = m_SeekOffset[0];
    // run through our byte offsets searching for our times...
    int iOffset = 1;
    while (iOffset < m_iSeekOffsets && fBytes > m_SeekOffset[iOffset])
      iOffset++;
    // iOffset will be the last of the two offsets and will be bigger than 1.
    float fTimeOffset = (float)iOffset - 1 + (fBytes - m_SeekOffset[iOffset - 1])/(m_SeekOffset[iOffset] - m_SeekOffset[iOffset - 1]);
    float fTime = fTimeOffset / m_iSeekOffsets * m_fTotalDuration;
    return (int64_t)(fTime * 1000.0f);
  };

  void SetDuration(float fDuration) { m_fTotalDuration = fDuration; };
  float GetDuration() const { return m_fTotalDuration; };

  void SetOffsets(int iSeekOffsets, const float *offsets)
  {
    m_iSeekOffsets = iSeekOffsets;
    delete[] m_SeekOffset;
    m_SeekOffset = NULL;
    if (m_iSeekOffsets <= 0)
      return;
    m_SeekOffset = new float[m_iSeekOffsets + 1];
    for (int i = 0; i <= m_iSeekOffsets; i++)
      m_SeekOffset[i] = offsets[i];
  };

  int GetNumOffsets() const { return m_iSeekOffsets; };
  const float *GetOffsets() const { return m_SeekOffset; };

  void SetSampleRange(int firstSample, int lastSample)
  {
    m_iFirstSample = firstSample;
    m_iLastSample = lastSample;
  };
  int GetFirstSample() const { return m_iFirstSample; };
  int GetLastSample() const { return m_iLastSample; };

protected:
  float m_fTotalDuration;
  int m_iSeekOffsets;
  float *m_SeekOffset;
  int m_iFirstSample;
  int m_iLastSample;
};

class CMusicInfoTagLoaderMP3: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderMP3(void);
  virtual ~CMusicInfoTagLoaderMP3();
  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  void GetSeekInfo(CVBRMP3SeekHelper &info) const;
  bool GetReplayGain(CReplayGain &info) const;
  bool ReadSeekAndReplayGainInfo(const CStdString &strFileName);
  static unsigned int IsID3v2Header(unsigned char* pBuf, size_t bufLen);
protected:
  virtual int ReadDuration(const CStdString& strFileName);
  bool ReadLAMETagInfo(unsigned char *p);
  int IsMp3FrameHeader(unsigned long head);
  virtual bool PrioritiseAPETags() const;

private:
  CVBRMP3SeekHelper m_seekInfo;
  CReplayGain       m_replayGainInfo;
};
}
