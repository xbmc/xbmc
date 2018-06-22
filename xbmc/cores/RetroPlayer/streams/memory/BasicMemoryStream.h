/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IMemoryStream.h"

#include <memory>

namespace KODI
{
namespace RETRO
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
