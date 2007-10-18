#ifndef BITSTREAM_STATS__H__
#define BITSTREAM_STATS__H__

#include <string>
#include "linux/PlatformDefs.h"

class BitstreamStats
{
public:
  // in order not to cause a performance hit, we should only check the clock when
  // we reach m_nEstimatedBitrate bits.
  // if this value is 1, we will calculate bitrate on every sample.
  BitstreamStats(unsigned int nEstimatedBitrate=(10240*8) /*10Kbit*/);
  virtual ~BitstreamStats();

  void AddSampleBytes(unsigned int nBytes);
  void AddSampleBits(unsigned int nBits);

  inline double GetBitrate()    const { return m_dBitrate; }
  inline double GetMaxBitrate() const { return m_dMaxBitrate; }
  inline double GetMinBitrate() const { return m_dMinBitrate; }

  void Start();
  void CalculateBitrate();

private:
  double m_dBitrate;
  double m_dMaxBitrate;
  double m_dMinBitrate;
  unsigned int m_nBitCount;
  unsigned int m_nEstimatedBitrate; // when we reach this amount of bits we check current bitrate.
  LARGE_INTEGER m_tmStart;  
  static LARGE_INTEGER m_tmFreq;  
};

#endif

