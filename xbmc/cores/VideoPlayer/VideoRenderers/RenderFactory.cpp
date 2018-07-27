/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderFactory.h"
#include "threads/SingleLock.h"

using namespace VIDEOPLAYER;

CCriticalSection renderSection;
std::map<std::string, VIDEOPLAYER::CreateRenderer> CRendererFactory::m_renderers;

CBaseRenderer* CRendererFactory::CreateRenderer(std::string id, CVideoBuffer *buffer)
{
  CSingleLock lock(renderSection);

  auto it = m_renderers.find(id);
  if (it != m_renderers.end())
  {
    return it->second(buffer);
  }

  return nullptr;
}

std::vector<std::string> CRendererFactory::GetRenderers()
{
  CSingleLock lock(renderSection);

  std::vector<std::string> ret;
  for (auto &renderer : m_renderers)
  {
    ret.push_back(renderer.first);
  }
  return ret;
}

void CRendererFactory::RegisterRenderer(std::string id, ::CreateRenderer createFunc)
{
  CSingleLock lock(renderSection);

  m_renderers[id] = createFunc;
}

void CRendererFactory::ClearRenderer()
{
  CSingleLock lock(renderSection);

  m_renderers.clear();
}
