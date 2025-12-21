/*
 *  Copyright (C) 2010-2021 Hendrik Leppkes (original LAV Filters implementation)
 *  Copyright (C) 2025 Team Kodi (Kodi adaptation)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 *
 *  This is a Kodi adaptation of the FloatingAverage class from LAV Filters,
 *  used for audio jitter tracking and A/V sync correction in passthrough mode.
 *  Original source: LAVFilters/common/DSUtilLite/FloatingAverage.h
 */

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace AudioSync
{

/*!
 * \brief Circular buffer for computing floating averages and statistics
 *
 * This class maintains a fixed-size circular buffer of samples and provides
 * various statistical operations commonly used in A/V sync correction.
 * Adapted from LAV Filters' FloatingAverage class by Hendrik Leppkes.
 *
 * \tparam T The numeric type to store (typically double for timestamps)
 * \tparam N The number of samples to track (buffer size)
 */
template <typename T, size_t N>
class CFloatingAverage
{
public:
  CFloatingAverage() { m_samples.fill(T{0}); }

  /*!
   * \brief Add a sample to the circular buffer
   * \param sample The sample value to add
   */
  void Sample(T sample)
  {
    m_samples[m_currentSample] = sample;
    if (++m_currentSample >= N)
      m_currentSample = 0;
    if (m_sampleCount < N)
      m_sampleCount++;
  }

  /*!
   * \brief Calculate the average of all samples
   * \return The arithmetic mean of all samples in the buffer
   */
  T Average() const
  {
    if (m_sampleCount == 0)
      return T{0};
    T sum{0};
    for (size_t i = 0; i < m_sampleCount; ++i)
      sum += m_samples[i];
    return sum / static_cast<T>(m_sampleCount);
  }

  /*!
   * \brief Find the minimum value in the buffer
   * \return The smallest sample value
   */
  T Minimum() const
  {
    if (m_sampleCount == 0)
      return T{0};
    T minVal = m_samples[0];
    for (size_t i = 1; i < m_sampleCount; ++i)
    {
      if (m_samples[i] < minVal)
        minVal = m_samples[i];
    }
    return minVal;
  }

  /*!
   * \brief Find the sample with the smallest absolute value
   *
   * This is the key metric LAV Filters uses for jitter correction.
   * By correcting to the minimum absolute jitter, we avoid over-correction
   * and achieve more stable A/V sync.
   *
   * \return The sample with the smallest absolute value (preserving sign)
   */
  T AbsMinimum() const
  {
    if (m_sampleCount == 0)
      return T{0};
    T minVal = m_samples[0];
    for (size_t i = 1; i < m_sampleCount; ++i)
    {
      if (std::abs(m_samples[i]) < std::abs(minVal))
        minVal = m_samples[i];
    }
    return minVal;
  }

  /*!
   * \brief Find the maximum value in the buffer
   * \return The largest sample value
   */
  T Maximum() const
  {
    if (m_sampleCount == 0)
      return T{0};
    T maxVal = m_samples[0];
    for (size_t i = 1; i < m_sampleCount; ++i)
    {
      if (m_samples[i] > maxVal)
        maxVal = m_samples[i];
    }
    return maxVal;
  }

  /*!
   * \brief Find the sample with the largest absolute value
   * \return The sample with the largest absolute value (preserving sign)
   */
  T AbsMaximum() const
  {
    if (m_sampleCount == 0)
      return T{0};
    T maxVal = m_samples[0];
    for (size_t i = 1; i < m_sampleCount; ++i)
    {
      if (std::abs(m_samples[i]) > std::abs(maxVal))
        maxVal = m_samples[i];
    }
    return maxVal;
  }

  /*!
   * \brief Offset all values in the buffer by a constant
   *
   * Used after applying a jitter correction to update all tracked
   * jitter values so they reflect the new baseline.
   *
   * \param value The offset to add to each sample
   */
  void OffsetValues(T value)
  {
    for (size_t i = 0; i < N; ++i)
      m_samples[i] += value;
  }

  /*!
   * \brief Reset the buffer to empty state
   */
  void Reset()
  {
    m_samples.fill(T{0});
    m_currentSample = 0;
    m_sampleCount = 0;
  }

  /*!
   * \brief Get the current sample index in the circular buffer
   * \return Index of the next sample to be written
   */
  size_t CurrentSample() const { return m_currentSample; }

  /*!
   * \brief Get the number of samples collected
   * \return Number of samples (up to N)
   */
  size_t SampleCount() const { return m_sampleCount; }

  /*!
   * \brief Check if the buffer is full
   * \return true if N samples have been collected
   */
  bool IsFull() const { return m_sampleCount >= N; }

private:
  std::array<T, N> m_samples;
  size_t m_currentSample{0};
  size_t m_sampleCount{0};
};

} // namespace AudioSync
