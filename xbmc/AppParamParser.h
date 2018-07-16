/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"

class CAppParamParser
{
  public:
    CAppParamParser();
    void Parse(const char* const* argv, int nArgs);

    const CFileItemList &Playlist() const { return m_playlist; }

  private:
    bool m_testmode;
    void ParseArg(const std::string &arg);
    void DisplayHelp();
    void DisplayVersion();
    void EnableDebugMode();

    CFileItemList m_playlist;
};
