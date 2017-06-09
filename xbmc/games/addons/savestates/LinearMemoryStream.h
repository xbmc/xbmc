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

#include "IMemoryStream.h"

#include <memory>

namespace KODI
{
namespace GAME
{
  class CLinearMemoryStream : public IMemoryStream
  {
  public:
    CLinearMemoryStream();

    virtual ~CLinearMemoryStream() = default;

    // partial implementation of IMemoryStream
    virtual void           Init(size_t frameSize, size_t maxFrameCount) override;
    virtual void           Reset() override;
    virtual size_t         FrameSize() const override                      { return m_frameSize; }
    virtual unsigned int   MaxFrameCount() const override                  { return m_maxFrames; }
    virtual void           SetMaxFrameCount(size_t maxFrameCount) override;
    virtual uint8_t*       BeginFrame() override;
    virtual void           SubmitFrame() override;
    virtual const uint8_t* CurrentFrame() const override;
    virtual unsigned int   FutureFramesAvailable() const override          { return 0; }
    virtual unsigned int   AdvanceFrames(unsigned int frameCount) override { return 0; }
    virtual unsigned int   PastFramesAvailable() const override = 0;
    virtual unsigned int   RewindFrames(unsigned int frameCount) override = 0;
    virtual uint64_t       GetFrameCounter() const override                { return m_currentFrameHistory; }
    virtual void           SetFrameCounter(uint64_t frameCount) override   { m_currentFrameHistory = frameCount; }

  protected:
    virtual void SubmitFrameInternal() = 0;
    virtual void CullPastFrames(unsigned int frameCount) = 0;

    // Helper function
    unsigned int BufferSize() const;

    size_t m_paddedFrameSize;
    unsigned int m_maxFrames;

    /**
     * Simple double-buffering. After XORing the two states, the next becomes
     * the current, and the current becomes a buffer for the next call to
     * CGameClient::Serialize().
     */
    std::unique_ptr<uint32_t[]> m_currentFrame;
    std::unique_ptr<uint32_t[]> m_nextFrame;
    bool                        m_bHasCurrentFrame;
    bool                        m_bHasNextFrame;

    uint64_t m_currentFrameHistory;

  private:
    size_t m_frameSize;
  };
}
}
