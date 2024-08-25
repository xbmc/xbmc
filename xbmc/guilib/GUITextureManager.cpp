/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITextureManager.h"

#include "guilib/GUITexture.h"

#include <vector>

GUITextureManager::GUITextureManager() = default;

GUITextureManager::~GUITextureManager() = default;

bool GUITextureManager::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() != GUI_MSG_NOTIFY_ALL)
    return false;

  if (message.GetParam1() != GUI_MSG_WINDOW_RESIZE)
    return false;

  const std::vector<CGUITexture*> currentTextures{m_textures.begin(), m_textures.end()};
  for (auto it = currentTextures.begin(); it != currentTextures.end(); ++it)
  {
    CGUITexture* texture = *it;
    texture->OnWindowResize();
  }

  return true;
}

void GUITextureManager::RegisterOnWindowResizeCallback(CGUITexture* texture)
{
  m_textures.insert(texture);
}

void GUITextureManager::UnregisterOnWindowResizeCallback(CGUITexture* texture)
{
  m_textures.erase(texture);
}
