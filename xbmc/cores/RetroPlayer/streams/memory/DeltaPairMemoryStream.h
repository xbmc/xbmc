/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LinearMemoryStream.h"

#include <deque>
#include <vector>

namespace KODI
{
namespace RETRO
{
/*!
 * \brief Implementation of a linear memory stream using XOR deltas
 */
class CDeltaPairMemoryStream : public CLinearMemoryStream
{
public:
  CDeltaPairMemoryStream() = default;

  ~CDeltaPairMemoryStream() override = default;

  // implementation of IMemoryStream via CLinearMemoryStream
  void Reset() override;
  void SetMaxFrameCount(uint64_t maxFrameCount) override;
  void SubmitFrame() override;
  void SubmitFrame(uint32_t discStateId);
  void SubmitFrame(uint32_t discStateId, uint64_t frameCounter);
  uint64_t FutureFramesAvailable() const override;
  uint64_t AdvanceFrames(uint64_t frameCount) override;
  uint64_t PastFramesAvailable() const override;
  uint64_t RewindFrames(uint64_t frameCount) override;

  uint32_t GetDiscStateID() const { return m_currentDiscStateId; }

protected:
  // implementation of CLinearMemoryStream
  void SubmitFrameInternal() override;
  void CullPastFrames(uint64_t frameCount) override;

  /*!
   * Rewinding is implemented by applying XOR deltas on the specific parts of
   * the save state buffer which have changed. In practice, this is very fast
   * and simple (linear scan) and allows deltas to be compressed down to 1-3%
   * of original save state size depending on the system. The algorithm runs
   * on 32 bits at a time for speed.
   *
   * Use std::deque here to achieve amortized O(1) on pop/push to front and
   * back.
   */
  struct DeltaPair
  {
    size_t pos;
    uint32_t delta;
  };

  using DeltaPairVector = std::vector<DeltaPair>;

  struct MemoryFrame
  {
    DeltaPairVector buffer;
    uint64_t beforeFrameHistoryCount;
    uint64_t afterFrameHistoryCount;
    uint32_t beforeDiscStateId;
    uint32_t afterDiscStateId;
  };

  std::deque<MemoryFrame> m_rewindBuffer;
  std::deque<MemoryFrame> m_advanceBuffer;
  uint32_t m_currentDiscStateId{0};
  uint32_t m_nextDiscStateId{0};
  uint64_t m_nextFrameHistory{0};
};
} // namespace RETRO
} // namespace KODI
