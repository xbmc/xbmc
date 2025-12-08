/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CDVDInputStream;
class IVideoPlayer;

class CDVDFactoryInputStream
{
public:
  static std::shared_ptr<CDVDInputStream> CreateInputStream(IVideoPlayer* pPlayer, const CFileItem &fileitem, bool scanforextaudio = false);
  static std::shared_ptr<CDVDInputStream> CreateInputStream(IVideoPlayer* pPlayer, const CFileItem &fileitem, const std::vector<std::string>& filenames);
};
