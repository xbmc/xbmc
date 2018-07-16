/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

class BitstreamStats final
{
public:
  // in order not to cause a performance hit, we should only check the clock when
  // we reach m_nEstimatedBitrate bits.
  // if this value is 1, we will calculate bitrate on every sample.
  explicit BitstreamStats(unsigned int nEstimatedBitrate=(10240*8) /*10Kbit*/);

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
  int64_t m_tmStart;
  static int64_t m_tmFreq;
};

