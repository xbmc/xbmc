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

#include "DeltaPairMemoryStream.h"
#include "utils/log.h"

using namespace GAME;

void CDeltaPairMemoryStream::Reset()
{
  CLinearMemoryStream::Reset();

  m_rewindBuffer.clear();
}

void CDeltaPairMemoryStream::SubmitFrameInternal()
{
  m_rewindBuffer.push_back(MemoryFrame());
  MemoryFrame& frame = m_rewindBuffer.back();

  // Record frame history
  frame.frameHistoryCount = m_currentFrameHistory++;

  uint32_t* currentFrame = m_currentFrame.get();
  uint32_t* nextFrame = m_nextFrame.get();

  for (size_t i = 0; i < m_paddedFrameSize; i++)
  {
    uint32_t xor_val = currentFrame[i] ^ nextFrame[i];
    if (xor_val)
    {
      DeltaPair pair = { i, xor_val };
      frame.buffer.push_back(pair);
    }
  }

  // Delta is generated, bring the new frame forward (m_nextFrame is now disposable)
  std::swap(m_currentFrame, m_nextFrame);

  m_bHasNextFrame = false;

  if (PastFramesAvailable() + 1 > MaxFrameCount())
    CullPastFrames(1);
}

unsigned int CDeltaPairMemoryStream::PastFramesAvailable() const
{
  return static_cast<unsigned int>(m_rewindBuffer.size());
}

unsigned int CDeltaPairMemoryStream::RewindFrames(unsigned int frameCount)
{
  unsigned int rewound;

  for (rewound = 0; rewound < frameCount; rewound++)
  {
    if (m_rewindBuffer.empty())
      break;

    const MemoryFrame& frame = m_rewindBuffer.back();
    const DeltaPair* buffer = frame.buffer.data();

    size_t bufferSize = frame.buffer.size();

    // buffer pointer redirection violates data-dependency requirements...
    // no vectorization for us :(
    for (size_t i = 0; i < bufferSize; i++)
      m_currentFrame[buffer[i].pos] ^= buffer[i].delta;

    // Restore frame history
    m_currentFrameHistory = frame.frameHistoryCount;

    m_rewindBuffer.pop_back();
  }

  return rewound;
}

void CDeltaPairMemoryStream::CullPastFrames(unsigned int frameCount)
{
  for (unsigned int removedCount = 0; removedCount < frameCount; removedCount++)
  {
    if (m_rewindBuffer.empty())
    {
      CLog::Log(LOGDEBUG, "CDeltaPairMemoryStream: Tried to cull %d frames too many. Check your math!", frameCount - removedCount);
      break;
    }
    m_rewindBuffer.pop_front();
  }
}
