/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "LinearMemoryStream.h"

#include <deque>
#include <vector>

namespace KODI
{
namespace GAME
{
  /*!
   * \brief Implementation of a linear memory stream using XOR deltas
   */
  class CDeltaPairMemoryStream : public CLinearMemoryStream
  {
  public:
    CDeltaPairMemoryStream() = default;

    virtual ~CDeltaPairMemoryStream() = default;

    // implementation of IMemoryStream via CLinearMemoryStream
    virtual void         Reset() override;
    virtual unsigned int PastFramesAvailable() const override;
    virtual unsigned int RewindFrames(unsigned int frameCount) override;

  protected:
    // implementation of CLinearMemoryStream
    virtual void SubmitFrameInternal() override;
    virtual void CullPastFrames(unsigned int frameCount) override;

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
      size_t   pos;
      uint32_t delta;
    };

    using DeltaPairVector = std::vector<DeltaPair>;

    struct MemoryFrame
    {
      DeltaPairVector buffer;
      uint64_t        frameHistoryCount;
    };

    std::deque<MemoryFrame> m_rewindBuffer;
  };
}
}
