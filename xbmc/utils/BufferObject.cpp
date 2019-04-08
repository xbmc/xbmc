/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BufferObject.h"

#include "BufferObjectFactory.h"

std::unique_ptr<CBufferObject> CBufferObject::GetBufferObject()
{
  return CBufferObjectFactory::CreateBufferObject();
}

int CBufferObject::GetFd()
{
  return m_fd;
}

uint32_t CBufferObject::GetStride()
{
  return m_stride;
}

uint64_t CBufferObject::GetModifier()
{
  return 0; // linear
}
