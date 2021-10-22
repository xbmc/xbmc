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

#include "utils/ColorUtils.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class CXBMCTinyXML;

class CGUIColorManager
{
public:
  CGUIColorManager(void);
  virtual ~CGUIColorManager(void);

  void Load(const std::string &colorFile);

  UTILS::COLOR::Color GetColor(const std::string& color) const;

  void Clear();

  /*! \brief Load a colors list from a XML file
    \param filePath The path to the XML file
    \param colors The vector to populate
    \param sortColors if true the colors will be sorted in a hue scale
    \return true if success, otherwise false
  */
  bool LoadColorsListFromXML(const std::string& filePath,
                             std::vector<std::pair<std::string, UTILS::COLOR::ColorInfo>>& colors,
                             bool sortColors);

protected:
  bool LoadXML(CXBMCTinyXML& xmlDoc);

  std::map<std::string, UTILS::COLOR::Color> m_colors;
};
