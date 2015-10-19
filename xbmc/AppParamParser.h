/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItem.h"

class CAppParamParser
{
  public:
    CAppParamParser();
    void Parse(const char* argv[], int nArgs);

  private:
    bool m_testmode;
    CFileItemList m_playlist;
    void ParseArg(const std::string &arg);
    void DisplayHelp();
    void DisplayVersion();
    void EnableDebugMode();
    void PlayPlaylist();
};
