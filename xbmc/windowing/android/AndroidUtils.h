#pragma once

/*
 *      Copyright (C) 2011-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

#include <androidjni/Display.h>

#include "windowing/Resolution.h"

class CAndroidUtils
{
public:
  CAndroidUtils();
  virtual ~CAndroidUtils();
  virtual bool  GetNativeResolution(RESOLUTION_INFO *res) const;
  virtual bool  SetNativeResolution(const RESOLUTION_INFO &res);
  virtual bool  ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions);

protected:
  mutable int m_width;
  mutable int m_height;
};
