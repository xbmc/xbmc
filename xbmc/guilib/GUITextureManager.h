/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUITextureManager.h
\brief
*/

#include "IMsgTargetCallback.h"
#include "utils/GlobalsHandling.h"

#include <set>

class CGUITexture;

/*!
 \ingroup textures
 \brief
 */
class GUITextureManager : public IMsgTargetCallback
{
public:
  GUITextureManager();
  ~GUITextureManager() override;

  bool OnMessage(CGUIMessage& message) override;

  void RegisterOnWindowResizeCallback(CGUITexture* texture);
  void UnregisterOnWindowResizeCallback(CGUITexture* texture);

private:
  std::set<CGUITexture*> m_textures;
};

/*!
 \ingroup textures
 \brief
 */
XBMC_GLOBAL_REF(GUITextureManager, g_textureManager);
#define g_textureManager XBMC_GLOBAL_USE(GUITextureManager)
