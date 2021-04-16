/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BaseRenderer.h"

#include <map>
#include <string>
#include <vector>

namespace VIDEOPLAYER
{

typedef CBaseRenderer* (*CreateRenderer)(CVideoBuffer *buffer);

class CRendererFactory
{
public:
  static CBaseRenderer* CreateRenderer(const std::string& id, CVideoBuffer* buffer);

  static void RegisterRenderer(const std::string& id, VIDEOPLAYER::CreateRenderer createFunc);
  static std::vector<std::string> GetRenderers();
  static void ClearRenderer();

protected:

  static std::map<std::string, VIDEOPLAYER::CreateRenderer> m_renderers;
};

}
