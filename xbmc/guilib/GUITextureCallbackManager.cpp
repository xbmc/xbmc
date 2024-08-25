/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITextureCallbackManager.h"

#include "guilib/GUITexture.h"

#include <vector>

CGUITextureCallbackManager::CGUITextureCallbackManager() = default;

CGUITextureCallbackManager::~CGUITextureCallbackManager() = default;

bool CGUITextureCallbackManager::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() != GUI_MSG_NOTIFY_ALL)
    return false;

  if (message.GetParam1() != GUI_MSG_WINDOW_RESIZE)
    return false;

  // Create a temporary copy because textures can be removed during the iteration
  const std::vector<CGUITexture*> currentTextures{m_textures.begin(), m_textures.end()};
  std::for_each(currentTextures.cbegin(), currentTextures.cend(),
                [](CGUITexture* texture) { texture->OnWindowResize(); });

  return true;
}

void CGUITextureCallbackManager::RegisterOnWindowResizeCallback(CGUITexture& texture)
{
  m_textures.insert(&texture);
}

void CGUITextureCallbackManager::UnregisterOnWindowResizeCallback(CGUITexture& texture)
{
  m_textures.erase(&texture);
}
