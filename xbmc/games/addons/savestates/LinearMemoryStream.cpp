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

#include "LinearMemoryStream.h"

using namespace KODI;
using namespace GAME;

// Pad forward to nearest boundary of bytes
#define PAD_TO_CEIL(x, bytes)  (((x) + (bytes) - 1) / (bytes))

CLinearMemoryStream::CLinearMemoryStream()
{
  Reset();
}

void CLinearMemoryStream::Init(size_t frameSize, size_t maxFrameCount)
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

void CLinearMemoryStream::SetMaxFrameCount(size_t maxFrameCount)
{
  if (maxFrameCount == 0)
  {
    Reset();
  }
  else
  {
    const unsigned int frameCount = BufferSize();
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

unsigned int CLinearMemoryStream::BufferSize() const
{
  return PastFramesAvailable() + (m_bHasCurrentFrame ? 1 : 0);
}
