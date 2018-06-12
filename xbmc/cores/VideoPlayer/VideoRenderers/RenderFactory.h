/*
 *      Copyright (C) 2005-2017 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
  static CBaseRenderer* CreateRenderer(std::string id, CVideoBuffer *buffer);

  static void RegisterRenderer(std::string id, VIDEOPLAYER::CreateRenderer createFunc);
  static std::vector<std::string> GetRenderers();
  static void ClearRenderer();

protected:

  static std::map<std::string, VIDEOPLAYER::CreateRenderer> m_renderers;
};

}
