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
  class CBasicMemoryStream : public IMemoryStream
  {
  public:
    CBasicMemoryStream();

    virtual ~CBasicMemoryStream() = default;

    // implementation of IMemoryStream
    virtual void           Init(size_t frameSize, size_t maxFrameCount) override;
    virtual void           Reset() override;
    virtual size_t         FrameSize() const override                      { return m_frameSize; }
    virtual unsigned int   MaxFrameCount() const override                  { return 1; }
    virtual void           SetMaxFrameCount(size_t maxFrameCount) override { }
    virtual uint8_t*       BeginFrame() override;
    virtual void           SubmitFrame() override;
    virtual const uint8_t* CurrentFrame() const override;
    virtual unsigned int   FutureFramesAvailable() const override          { return 0; }
    virtual unsigned int   AdvanceFrames(unsigned int frameCount) override { return 0; }
    virtual unsigned int   PastFramesAvailable() const override            { return 0; }
    virtual unsigned int   RewindFrames(unsigned int frameCount) override  { return 0; }
    virtual uint64_t       GetFrameCounter() const override                { return 0; }
    virtual void           SetFrameCounter(uint64_t frameCount) override   { };

  private:
    size_t                     m_frameSize;
    std::unique_ptr<uint8_t[]> m_frameBuffer;
    bool                       m_bHasFrame;
  };
}
}
