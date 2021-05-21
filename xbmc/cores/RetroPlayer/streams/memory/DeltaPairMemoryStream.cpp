/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DeltaPairMemoryStream.h"

#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

void CDeltaPairMemoryStream::Reset()
{
  CLinearMemoryStream::Reset();

  m_rewindBuffer.clear();
}

void CDeltaPairMemoryStream::SubmitFrameInternal()
{
  m_rewindBuffer.emplace_back();
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
      DeltaPair pair = {i, xor_val};
      frame.buffer.push_back(pair);
    }
  }

  // Delta is generated, bring the new frame forward (m_nextFrame is now disposable)
  std::swap(m_currentFrame, m_nextFrame);

  m_bHasNextFrame = false;

  if (PastFramesAvailable() + 1 > MaxFrameCount())
    CullPastFrames(1);
}

uint64_t CDeltaPairMemoryStream::PastFramesAvailable() const
{
  return static_cast<uint64_t>(m_rewindBuffer.size());
}

uint64_t CDeltaPairMemoryStream::RewindFrames(uint64_t frameCount)
{
  uint64_t rewound;

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

void CDeltaPairMemoryStream::CullPastFrames(uint64_t frameCount)
{
  for (uint64_t removedCount = 0; removedCount < frameCount; removedCount++)
  {
    if (m_rewindBuffer.empty())
    {
      CLog::Log(LOGDEBUG,
                "CDeltaPairMemoryStream: Tried to cull {} frames too many. Check your math!",
                frameCount - removedCount);
      break;
    }
    m_rewindBuffer.pop_front();
  }
}
