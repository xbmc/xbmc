/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
