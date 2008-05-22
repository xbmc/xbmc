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
#include "BitstreamStats.h"
#ifdef _LINUX
#include "linux/XTimeUtils.h"
#endif

LARGE_INTEGER BitstreamStats::m_tmFreq;

BitstreamStats::BitstreamStats(unsigned int nEstimatedBitrate)
{
  m_dBitrate = 0.0;
  m_dMaxBitrate = 0.0;
  m_dMinBitrate = -1.0;

  m_nBitCount = 0;
  m_nEstimatedBitrate = nEstimatedBitrate;
  m_tmStart.QuadPart = 0LL;

  if (m_tmFreq.QuadPart == 0LL)
    QueryPerformanceFrequency(&m_tmFreq);
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
  QueryPerformanceCounter(&m_tmStart);
}

void BitstreamStats::CalculateBitrate()
{
  LARGE_INTEGER tmNow;
  QueryPerformanceCounter(&tmNow);

  double elapsed = (double)(tmNow.QuadPart - m_tmStart.QuadPart) / (double)m_tmFreq.QuadPart;
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




