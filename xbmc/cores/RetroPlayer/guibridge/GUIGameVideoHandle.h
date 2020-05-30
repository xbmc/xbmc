/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"

namespace KODI
{
namespace RETRO
{
class CGUIGameRenderManager;

class CGUIGameVideoHandle
{
public:
  CGUIGameVideoHandle(CGUIGameRenderManager& renderManager);
  virtual ~CGUIGameVideoHandle();

  bool IsPlayingGame();
  bool SupportsRenderFeature(RENDERFEATURE feature);
  bool SupportsScalingMethod(SCALINGMETHOD method);

private:
  // Construction parameters
  CGUIGameRenderManager& m_renderManager;
};
} // namespace RETRO
} // namespace KODI
