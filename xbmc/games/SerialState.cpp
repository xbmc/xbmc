/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// Code is by Hans-Kristian Arntzen

#include "SerialState.h"
#include "system.h" // for SAFE_DELETE_ARRAY()


// Pad forward to nearest boundary of bytes
#define PAD_TO_CEIL(x, bytes) (((x) + (bytes) - 1) / (bytes))

void CSerialState::Init(size_t frameSize, size_t frameCount)
{
  Reset();
  m_frameSize = frameSize; // Size of the frame from retro_serialize_size()
  m_maxFrames = frameCount;
  m_stateSize = PAD_TO_CEIL(m_frameSize, sizeof(uint32_t)); // Size of the padded frame ( >= m_frameSize)
  m_state = new uint32_t[m_stateSize];
  m_nextState = new uint32_t[m_stateSize];
}

// Make sure m_state and m_nextState are zero-initialized in the constructor
void CSerialState::Reset()
{
  SAFE_DELETE_ARRAY(m_state);
  SAFE_DELETE_ARRAY(m_nextState);
  m_rewindBuffer.clear();
  m_frameSize = 0;
  m_maxFrames = 0;
  m_stateSize = 0;
}

void CSerialState::SetMaxFrames(size_t frameCount)
{
  m_maxFrames = frameCount;
  if (!m_maxFrames && (m_state || m_nextState))
  {
    Reset();
  }
  else
  {
    while (m_rewindBuffer.size() > m_maxFrames)
      m_rewindBuffer.pop_front();
  }
}

void CSerialState::AdvanceFrame()
{
  m_rewindBuffer.push_back(DeltaPairVector());
  DeltaPairVector& buffer = m_rewindBuffer.back();

  for (size_t i = 0; i < m_stateSize; i++)
  {
    uint32_t xor_val = m_state[i] ^ m_nextState[i];
    if (xor_val)
    {
      DeltaPair pair = {i, xor_val};
      buffer.push_back(pair);
    }
  }

  // Delta is generated, bring the new frame forward (m_nextState is now disposable)
  std::swap(m_state, m_nextState);

  while (m_rewindBuffer.size() > m_maxFrames)
    m_rewindBuffer.pop_front();
}

unsigned int CSerialState::RewindFrames(unsigned int frameCount)
{
  unsigned int rewound = 0;
  while (frameCount > 0 && !m_rewindBuffer.empty())
  {
    const DeltaPair *buffer = m_rewindBuffer.back().data();

    size_t bufferSize = m_rewindBuffer.back().size();
    // buffer pointer redirection violates data-dependency requirements...
    // no vecorization for us :(
    for (size_t i = 0; i < bufferSize; i++)
      m_state[buffer[i].pos] ^= buffer[i].delta;

    rewound++;
    frameCount--;
    m_rewindBuffer.pop_back();
  }

  return rewound;
}
