/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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




