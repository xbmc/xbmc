/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

class CDVDSubtitleParser;
class CDVDSubtitleStream;

typedef std::vector<std::string> VecSubtitleFiles;
typedef std::vector<std::string>::iterator VecSubtitleFilesIter;

class CDVDFactorySubtitle
{
public:
  static CDVDSubtitleParser* CreateParser(std::string& strFile);
};
