/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//  CAutorun   -  Autorun for different Cd Media
//         like DVD Movies or XBOX Games
//
// by Bobbin007 in 2003
//
//
//

#ifdef HAS_OPTICAL_DRIVE

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct IntegerSettingOption;

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

  /*! \brief Execute the autorun. Used for example to automatically rip cds or play optical discs
    * @param path the path for the item (e.g. the disc path)
    * @return true if some action was executed, false otherwise
    */
  static bool ExecuteAutorun(const std::string& path);

  static void SettingOptionAudioCdActionsFiller(const std::shared_ptr<const CSetting>& setting,
                                                std::vector<IntegerSettingOption>& list,
                                                int& current,
                                                void* data);

protected:
  static bool RunDisc(XFILE::IDirectory* pDir, const std::string& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings, bool startFromBeginning);
  bool m_bEnable;
};
}

#endif
