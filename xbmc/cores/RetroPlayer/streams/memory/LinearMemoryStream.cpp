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

// Pad forward to nearest boundary of bytes
#define PAD_TO_CEIL(x, bytes) ((((x) + (bytes)-1) / (bytes)) * (bytes))

CLinearMemoryStream::CLinearMemoryStream()
{
  Reset();
}

void CLinearMemoryStream::Init(size_t frameSize, uint64_t maxFrameCount)
{
  Reset();

  m_frameSize = frameSize;
  m_paddedFrameSize = PAD_TO_CEIL(m_frameSize, sizeof(uint32_t));
  m_maxFrames = maxFrameCount;
}

void CLinearMemoryStream::Reset()
{
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
  if (m_paddedFrameSize == 0)
    return nullptr;

  if (!m_bHasCurrentFrame)
  {
    if (!m_currentFrame)
      m_currentFrame.reset(new uint32_t[m_paddedFrameSize]);
    return reinterpret_cast<uint8_t*>(m_currentFrame.get());
  }

  if (!m_nextFrame)
    m_nextFrame.reset(new uint32_t[m_paddedFrameSize]);
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
  }
}

uint64_t CLinearMemoryStream::BufferSize() const
{
  return PastFramesAvailable() + (m_bHasCurrentFrame ? 1 : 0);
}
