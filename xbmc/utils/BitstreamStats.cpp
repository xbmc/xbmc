#include "BitstreamStats.h"
#include "linux/XTimeUtils.h"

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




