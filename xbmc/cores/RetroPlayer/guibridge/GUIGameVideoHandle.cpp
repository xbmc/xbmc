/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameVideoHandle.h"
#include "GUIGameRenderManager.h"

using namespace KODI;
using namespace RETRO;

CGUIGameVideoHandle::CGUIGameVideoHandle(CGUIGameRenderManager &renderManager) :
  m_renderManager(renderManager)
{
}

CGUIGameVideoHandle::~CGUIGameVideoHandle()
{
  m_renderManager.UnregisterHandle(this);
}

bool CGUIGameVideoHandle::IsPlayingGame()
{
  return m_renderManager.IsPlayingGame();
}

bool CGUIGameVideoHandle::SupportsRenderFeature(RENDERFEATURE feature)
{
  return m_renderManager.SupportsRenderFeature(feature);
}

bool CGUIGameVideoHandle::SupportsScalingMethod(SCALINGMETHOD method)
{
  return m_renderManager.SupportsScalingMethod(method);
}
