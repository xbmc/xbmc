#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

#include "utils/StdString.h"

namespace XFILE
{
  class IDirectory;
}

namespace MEDIA_DETECT
{
class CAutorun
{
public:
  CAutorun();
  virtual ~CAutorun();
  static bool CanResumePlayDVD(const CStdString& path);
  static bool PlayDisc(const CStdString& path="", bool bypassSettings = false, bool startFromBeginning = false);
  static bool PlayDiscAskResume(const CStdString& path="");
  bool IsEnabled() const;
  void Enable();
  void Disable();
  void HandleAutorun();
  static void ExecuteAutorun(const CStdString& path = "", bool bypassSettings = false, bool ignoreplaying = false, bool startFromBeginning = false);
protected:
  static bool RunDisc(XFILE::IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings, bool startFromBeginning);
  bool m_bEnable;
};
}

#endif
