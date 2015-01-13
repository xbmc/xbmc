#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

//  CAutorun   -  Autorun for different Cd Media
//         like DVD Movies or XBOX Games
//
// by Bobbin007 in 2003
//
//
//

#include "system.h" // for HAS_DVD_DRIVE

#ifdef HAS_DVD_DRIVE

#include <string>
#include <vector>

namespace XFILE
{
  class IDirectory;
}

class CSetting;

enum AutoCDAction
{
  AUTOCD_NONE = 0,
  AUTOCD_PLAY,
  AUTOCD_RIP
};

namespace MEDIA_DETECT
{
class CAutorun
{
public:
  CAutorun();
  virtual ~CAutorun();
  static bool CanResumePlayDVD(const std::string& path);
  static bool PlayDisc(const std::string& path="", bool bypassSettings = false, bool startFromBeginning = false);
  static bool PlayDiscAskResume(const std::string& path="");
  bool IsEnabled() const;
  void Enable();
  void Disable();
  void HandleAutorun();
  static void ExecuteAutorun(const std::string& path = "", bool bypassSettings = false, bool ignoreplaying = false, bool startFromBeginning = false);

  static void SettingOptionAudioCdActionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

protected:
  static bool RunDisc(XFILE::IDirectory* pDir, const std::string& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings, bool startFromBeginning);
  bool m_bEnable;
};
}

#endif
