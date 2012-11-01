/*!
\file GUIColorManager.h
\brief
*/

#ifndef GUILIB_COLORMANAGER_H
#define GUILIB_COLORMANAGER_H

#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

/*!
 \ingroup textures
 \brief
 */

#include "utils/StdString.h"

#include <map>

class CXBMCTinyXML;

typedef uint32_t color_t;

class CGUIColorManager
{
public:
  CGUIColorManager(void);
  virtual ~CGUIColorManager(void);

  void Load(const CStdString &colorFile);

  color_t GetColor(const CStdString &color) const;

  void Clear();

protected:
  bool LoadXML(CXBMCTinyXML &xmlDoc);

  std::map<CStdString, color_t> m_colors;
  typedef std::map<CStdString, color_t>::iterator iColor;
  typedef std::map<CStdString, color_t>::const_iterator icColor;
};

/*!
 \ingroup textures
 \brief
 */
extern CGUIColorManager g_colorManager;
#endif
