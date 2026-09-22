/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinearMemoryStream.h"

using namespace KODI;
using namespace RETRO;

CLinearMemoryStream::CLinearMemoryStream()
{
  Reset();
}

void CLinearMemoryStream::Init(size_t frameSize, uint64_t maxFrameCount)
{
  Reset();

  m_frameSize = frameSize;
  m_paddedFrameSize = (m_frameSize + sizeof(uint32_t) - 1) / sizeof(uint32_t);
  m_maxFrames = maxFrameCount;
  if (m_paddedFrameSize != 0)
  {
    m_currentFrame = std::make_unique<uint32_t[]>(m_paddedFrameSize);
    m_nextFrame = std::make_unique<uint32_t[]>(m_paddedFrameSize);
  }
}

void CLinearMemoryStream::Reset()
{
  m_hasRetiredFrame = false;
  m_frameSize = 0;
  m_paddedFrameSize = 0;
  m_maxFrames = 0;
  m_currentFrame.reset();
  m_nextFrame.reset();
  m_bHasCurrentFrame = false;
  m_bHasNextFrame = false;
  m_currentFrameHistory = 0;
}

void CLinearMemoryStream::SetMaxFrameCount(uint64_t maxFrameCount)
{
  if (maxFrameCount == 0)
  {
    Reset();
  }
  else
  {
    const uint64_t frameCount = BufferSize();
    if (maxFrameCount < frameCount)
      CullPastFrames(frameCount - maxFrameCount);
  }

  m_maxFrames = maxFrameCount;
}

uint8_t* CLinearMemoryStream::BeginFrame()
{
  m_hasRetiredFrame = false;
  if (m_paddedFrameSize == 0)
    return nullptr;

  if (!m_bHasCurrentFrame)
  {
    return reinterpret_cast<uint8_t*>(m_currentFrame.get());
  }

  return reinterpret_cast<uint8_t*>(m_nextFrame.get());
}

const uint8_t* CLinearMemoryStream::CurrentFrame() const
{
  if (m_bHasCurrentFrame)
    return reinterpret_cast<const uint8_t*>(m_currentFrame.get());

  return nullptr;
}

void CLinearMemoryStream::SubmitFrame()
{
  if (!m_bHasCurrentFrame)
  {
    m_bHasCurrentFrame = true;
  }
  else if (!m_bHasNextFrame)
  {
    m_bHasNextFrame = true;
  }

  if (m_bHasNextFrame)
  {
    SubmitFrameInternal();
    m_hasRetiredFrame = true;
  }
}

uint64_t CLinearMemoryStream::BufferSize() const
{
  return PastFramesAvailable() + (m_bHasCurrentFrame ? 1 : 0);
}

bool CLinearMemoryStream::ExchangeRetiredFrame(std::unique_ptr<uint32_t[]>& replacement)
{
  if (!replacement || !m_hasRetiredFrame)
    return false;

  m_nextFrame.swap(replacement);
  m_hasRetiredFrame = false;
  return true;
}
