/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//  CDetectDVDMedia
//  Thread running in the background to detect a CD change and the filesystem
//
// by Bobbin007 in 2003

#include "PlatformDefs.h"

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
  ~CDetectDVDMedia() override;

  void OnStartup() override;
  void OnExit() override;
  void Process() override;

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

  bool m_bStartup = true; // Do not autorun on startup
  bool m_bAutorun = false;
  DWORD m_dwTrayState;
  DWORD m_dwLastTrayState = 0;

  static std::string m_diskLabel;
  static std::string m_diskPath;

  std::shared_ptr<CLibcdio> m_cdio;
};
}
#endif
