/*
 *      Copyright (C) 2016-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "IMemoryStream.h"

#include <memory>

namespace KODI
{
namespace GAME
{
  class CBasicMemoryStream : public IMemoryStream
  {
  public:
    CBasicMemoryStream();

    virtual ~CBasicMemoryStream() = default;

    // implementation of IMemoryStream
    virtual void Init(size_t frameSize, uint64_t maxFrameCount) override;
    virtual void Reset() override;
    virtual size_t FrameSize() const override { return m_frameSize; }
    virtual uint64_t MaxFrameCount() const override { return 1; }
    virtual void SetMaxFrameCount(uint64_t maxFrameCount) override { }
    virtual uint8_t* BeginFrame() override;
    virtual void SubmitFrame() override;
    virtual const uint8_t* CurrentFrame() const override;
    virtual uint64_t FutureFramesAvailable() const override { return 0; }
    virtual uint64_t AdvanceFrames(uint64_t frameCount) override { return 0; }
    virtual uint64_t PastFramesAvailable() const override { return 0; }
    virtual uint64_t RewindFrames(uint64_t frameCount) override { return 0; }
    virtual uint64_t GetFrameCounter() const override { return 0; }
    virtual void SetFrameCounter(uint64_t frameCount) override { };

  private:
    size_t m_frameSize;
    std::unique_ptr<uint8_t[]> m_frameBuffer;
    bool m_bHasFrame;
  };
}
}
