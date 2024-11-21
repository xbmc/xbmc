/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUITextureCallbackManager.h
\brief
*/

#include "IMsgTargetCallback.h"
#include "utils/GlobalsHandling.h"

#include <unordered_set>

class CGUITexture;

/*!
 \ingroup textures
 \brief
 */
class CGUITextureCallbackManager : public IMsgTargetCallback
{
public:
  CGUITextureCallbackManager();
  ~CGUITextureCallbackManager() override;

  bool OnMessage(CGUIMessage& message) override;

  void RegisterOnWindowResizeCallback(CGUITexture& texture);
  void UnregisterOnWindowResizeCallback(CGUITexture& texture);

private:
  std::unordered_set<CGUITexture*> m_textures;
};
