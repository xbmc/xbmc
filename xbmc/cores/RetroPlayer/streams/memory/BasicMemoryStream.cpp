/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BasicMemoryStream.h"

using namespace KODI;
using namespace RETRO;

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
