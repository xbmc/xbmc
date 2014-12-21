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

//  CDetectDVDMedia   -  Thread running in the background to detect a CD change
//       and the filesystem
//
// by Bobbin007 in 2003
//
//
//

#include "system.h"

#ifdef HAS_DVD_DRIVE

#include "threads/CriticalSection.h"

#include "threads/Thread.h"
#include <memory>
#include <string>

namespace MEDIA_DETECT
{
class CCdInfo;
class CLibcdio;

class CDetectDVDMedia : public CThread
{
public:
  CDetectDVDMedia();
  virtual ~CDetectDVDMedia();

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  static void WaitMediaReady();
  static bool IsDiscInDrive();
  static int DriveReady();
  static CCdInfo* GetCdInfo();
  static CEvent m_evAutorun;

  static const std::string &GetDVDLabel();
  static const std::string &GetDVDPath();

  static void UpdateState();
protected:
  void UpdateDvdrom();
  DWORD GetTrayState();


  void DetectMediaType();
  void SetNewDVDShareUrl( const std::string& strNewUrl, bool bCDDA, const std::string& strDiscLabel );

private:
  static CCriticalSection m_muReadingMedia;

  static int m_DriveState;
  static time_t m_LastPoll;
  static CDetectDVDMedia* m_pInstance;

  static CCdInfo* m_pCdInfo;

  bool m_bStartup;
  bool m_bAutorun;
  DWORD m_dwTrayState;
  DWORD m_dwTrayCount;
  DWORD m_dwLastTrayState;

  static std::string m_diskLabel;
  static std::string m_diskPath;

  std::shared_ptr<CLibcdio> m_cdio;
};
}
#endif
