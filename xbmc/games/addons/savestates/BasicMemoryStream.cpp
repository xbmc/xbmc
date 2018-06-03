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

#include "BasicMemoryStream.h"

using namespace KODI;
using namespace GAME;

CBasicMemoryStream::CBasicMemoryStream()
{
  Reset();
}

void CBasicMemoryStream::Init(size_t frameSize, uint64_t maxFrameCount)
{
  Reset();

  m_frameSize = frameSize;
}

void CBasicMemoryStream::Reset()
{
  m_frameSize = 0;
  m_frameBuffer.reset();
  m_bHasFrame = false;
}

uint8_t* CBasicMemoryStream::BeginFrame()
{
  if (m_frameSize == 0)
    return nullptr;

  if (!m_frameBuffer)
    m_frameBuffer.reset(new uint8_t[m_frameSize]);

  m_bHasFrame = false;

  return m_frameBuffer.get();
}

void CBasicMemoryStream::SubmitFrame()
{
  if (m_frameBuffer)
    m_bHasFrame = true;
}

const uint8_t* CBasicMemoryStream::CurrentFrame() const
{
  return m_bHasFrame ? m_frameBuffer.get() : nullptr;
}
