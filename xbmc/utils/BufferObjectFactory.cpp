/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BufferObjectFactory.h"

std::list<std::function<std::unique_ptr<CBufferObject>()>> CBufferObjectFactory::m_bufferObjects;

std::unique_ptr<CBufferObject> CBufferObjectFactory::CreateBufferObject(bool needsCreateBySize)
{
  for (const auto& bufferObject : m_bufferObjects)
  {
    auto bo = bufferObject();

    if (needsCreateBySize)
    {
      if (!bo->CreateBufferObject(1))
        continue;

      bo->DestroyBufferObject();
    }

    return bo;
  }

  return nullptr;
}

void CBufferObjectFactory::RegisterBufferObject(
    const std::function<std::unique_ptr<CBufferObject>()>& createFunc)
{
  m_bufferObjects.emplace_front(createFunc);
}

void CBufferObjectFactory::ClearBufferObjects()
{
  m_bufferObjects.clear();
}
