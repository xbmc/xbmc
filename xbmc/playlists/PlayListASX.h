/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayList.h"

#include <iostream>

namespace tinyxml2
{
class XMLDocument;
class XMLNode;
} // namespace tinyxml2

namespace KODI::PLAYLIST
{
class CPlayListASX : public CPlayList
{
public:
  bool LoadData(std::istream& stream) override;

private:
  bool LoadAsxIniInfo(std::istream& stream);

  /*  recurseLowercaseNames
   *  Function allows recursive iteration of a source element to lowercase all
   *  element and attrib Names, and save to a targetNode.
   *  targetNode must be a separate XMLDocument to sourceNode XMLDocument
   */
  void recurseLowercaseNames(tinyxml2::XMLNode& targetNode, tinyxml2::XMLNode* sourceNode);
};
} // namespace KODI::PLAYLIST
