/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderFactory.h"

#include <mutex>


using namespace VIDEOPLAYER;

CCriticalSection renderSection;
std::map<std::string, VIDEOPLAYER::CreateRenderer> CRendererFactory::m_renderers;

CBaseRenderer* CRendererFactory::CreateRenderer(const std::string& id, CVideoBuffer* buffer)
{
  std::unique_lock<CCriticalSection> lock(renderSection);

  auto it = m_renderers.find(id);
  if (it != m_renderers.end())
  {
    return it->second(buffer);
  }

  return nullptr;
}

std::vector<std::string> CRendererFactory::GetRenderers()
{
  std::unique_lock<CCriticalSection> lock(renderSection);

  std::vector<std::string> ret;
  ret.reserve(m_renderers.size());
  for (auto &renderer : m_renderers)
  {
    ret.push_back(renderer.first);
  }
  return ret;
}

void CRendererFactory::RegisterRenderer(const std::string& id, ::CreateRenderer createFunc)
{
  std::unique_lock<CCriticalSection> lock(renderSection);

  m_renderers[id] = createFunc;
}

void CRendererFactory::ClearRenderer()
{
  std::unique_lock<CCriticalSection> lock(renderSection);

  m_renderers.clear();
}
