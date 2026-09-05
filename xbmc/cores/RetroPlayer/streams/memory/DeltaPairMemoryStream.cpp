/*
 *  Copyright (C) 2016-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DeltaPairMemoryStream.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace RETRO;

void CDeltaPairMemoryStream::Reset()
{
  CLinearMemoryStream::Reset();

  m_rewindBuffer.clear();
  m_advanceBuffer.clear();
  m_currentDiscStateId = 0;
  m_nextDiscStateId = 0;
  m_nextFrameHistory = 0;
}

void CDeltaPairMemoryStream::SetMaxFrameCount(uint64_t maxFrameCount)
{
  if (maxFrameCount == 0)
  {
    Reset();
    return;
  }

  uint64_t framesToRemove =
      PastFramesAvailable() + FutureFramesAvailable() + (m_bHasCurrentFrame ? 1 : 0) -
      std::min(maxFrameCount,
               PastFramesAvailable() + FutureFramesAvailable() + (m_bHasCurrentFrame ? 1 : 0));

  const uint64_t pastToRemove = std::min(framesToRemove, PastFramesAvailable());
  CullPastFrames(pastToRemove);
  framesToRemove -= pastToRemove;

  while (framesToRemove > 0 && !m_advanceBuffer.empty())
  {
    m_advanceBuffer.pop_back();
    --framesToRemove;
  }

  m_maxFrames = maxFrameCount;
}

void CDeltaPairMemoryStream::SubmitFrame()
{
  SubmitFrame(0);
}

void CDeltaPairMemoryStream::SubmitFrame(uint32_t discStateId)
{
  SubmitFrame(discStateId, m_bHasCurrentFrame ? m_currentFrameHistory + 1 : m_currentFrameHistory);
}

void CDeltaPairMemoryStream::SubmitFrame(uint32_t discStateId, uint64_t frameCounter)
{
  if (!m_bHasCurrentFrame)
  {
    m_bHasCurrentFrame = true;
    m_currentDiscStateId = discStateId;
    m_currentFrameHistory = frameCounter;
    return;
  }

  if (!m_bHasNextFrame)
    m_bHasNextFrame = true;

  m_nextDiscStateId = discStateId;
  m_nextFrameHistory = frameCounter;
  SubmitFrameInternal();
}

void CDeltaPairMemoryStream::SubmitFrameInternal()
{
  m_advanceBuffer.clear();
  MemoryFrame& frame = m_rewindBuffer.emplace_back();

  frame.beforeFrameHistoryCount = m_currentFrameHistory;
  frame.afterFrameHistoryCount = m_nextFrameHistory;
  frame.beforeDiscStateId = m_currentDiscStateId;
  frame.afterDiscStateId = m_nextDiscStateId;

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
  m_currentFrameHistory = m_nextFrameHistory;
  m_currentDiscStateId = m_nextDiscStateId;

  m_bHasNextFrame = false;

  if (PastFramesAvailable() + 1 > MaxFrameCount())
    CullPastFrames(1);
}

uint64_t CDeltaPairMemoryStream::PastFramesAvailable() const
{
  return static_cast<uint64_t>(m_rewindBuffer.size());
}

uint64_t CDeltaPairMemoryStream::FutureFramesAvailable() const
{
  return static_cast<uint64_t>(m_advanceBuffer.size());
}

uint64_t CDeltaPairMemoryStream::AdvanceFrames(uint64_t frameCount)
{
  uint64_t advanced = 0;

  for (; advanced < frameCount && !m_advanceBuffer.empty(); ++advanced)
  {
    MemoryFrame frame = std::move(m_advanceBuffer.front());
    m_advanceBuffer.pop_front();

    for (const DeltaPair& pair : frame.buffer)
      m_currentFrame[pair.pos] ^= pair.delta;

    m_currentFrameHistory = frame.afterFrameHistoryCount;
    m_currentDiscStateId = frame.afterDiscStateId;
    m_rewindBuffer.emplace_back(std::move(frame));
  }

  return advanced;
}

uint64_t CDeltaPairMemoryStream::RewindFrames(uint64_t frameCount)
{
  uint64_t rewound;

  for (rewound = 0; rewound < frameCount; rewound++)
  {
    if (m_rewindBuffer.empty())
      break;

    MemoryFrame frame = std::move(m_rewindBuffer.back());
    m_rewindBuffer.pop_back();
    const DeltaPair* buffer = frame.buffer.data();

    size_t bufferSize = frame.buffer.size();

    // buffer pointer redirection violates data-dependency requirements...
    // no vectorization for us :(
    for (size_t i = 0; i < bufferSize; i++)
      m_currentFrame[buffer[i].pos] ^= buffer[i].delta;

    // Restore frame history
    m_currentFrameHistory = frame.beforeFrameHistoryCount;
    m_currentDiscStateId = frame.beforeDiscStateId;

    m_advanceBuffer.emplace_front(std::move(frame));
  }

  return rewound;
}

void CDeltaPairMemoryStream::CullPastFrames(uint64_t frameCount)
{
  for (uint64_t removedCount = 0; removedCount < frameCount; removedCount++)
  {
    if (m_rewindBuffer.empty())
      break;
    m_rewindBuffer.pop_front();
  }
}
