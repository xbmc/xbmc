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

#include "BitstreamStats.h"
#include "utils/TimeUtils.h"

int64_t BitstreamStats::m_tmFreq;

BitstreamStats::BitstreamStats(unsigned int nEstimatedBitrate)
{
  m_dBitrate = 0.0;
  m_dMaxBitrate = 0.0;
  m_dMinBitrate = -1.0;

  m_nBitCount = 0;
  m_nEstimatedBitrate = nEstimatedBitrate;
  m_tmStart = 0LL;

  if (m_tmFreq == 0LL)
    m_tmFreq = CurrentHostFrequency();
}

BitstreamStats::~BitstreamStats()
{
}

void BitstreamStats::AddSampleBytes(unsigned int nBytes)
{
  AddSampleBits(nBytes*8);
}

void BitstreamStats::AddSampleBits(unsigned int nBits)
{
  m_nBitCount += nBits;
  if (m_nBitCount >= m_nEstimatedBitrate)
    CalculateBitrate();
}

void BitstreamStats::Start()
{
  m_nBitCount = 0;
  m_tmStart = CurrentHostCounter();
}

void BitstreamStats::CalculateBitrate()
{
  int64_t tmNow;
  tmNow = CurrentHostCounter();

  double elapsed = (double)(tmNow - m_tmStart) / (double)m_tmFreq;
  // only update once every 2 seconds
  if (elapsed >= 2)
  {
    m_dBitrate = (double)m_nBitCount / elapsed;

    if (m_dBitrate > m_dMaxBitrate)
      m_dMaxBitrate = m_dBitrate;

    if (m_dBitrate < m_dMinBitrate || m_dMinBitrate == -1)
      m_dMinBitrate = m_dBitrate;

    Start();
  }
}




