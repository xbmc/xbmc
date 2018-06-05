/*
 *      Copyright (C) 2005-2013 Team XBMC
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

/*!
\file GUIColorManager.h
\brief
*/

/*!
 \ingroup textures
 \brief
 */

#include <map>
#include <string>

#include "utils/Color.h"

class CXBMCTinyXML;

class CGUIColorManager
{
public:
  CGUIColorManager(void);
  virtual ~CGUIColorManager(void);

  void Load(const std::string &colorFile);

  UTILS::Color GetColor(const std::string &color) const;

  void Clear();

protected:
  bool LoadXML(CXBMCTinyXML &xmlDoc);

  std::map<std::string, UTILS::Color> m_colors;
};

/*!
 \ingroup textures
 \brief
 */
extern CGUIColorManager g_colorManager;

