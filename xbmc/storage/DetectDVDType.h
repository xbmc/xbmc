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
#include "utils/StdString.h"
#include "utils/Job.h"

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

  static const CStdString &GetDVDLabel();
  static const CStdString &GetDVDPath();

  static void UpdateState();
protected:
  void UpdateDvdrom();
  DWORD GetTrayState();


  void DetectMediaType();
  void SetNewDVDShareUrl( const CStdString& strNewUrl, bool bCDDA, const CStdString& strDiscLabel );

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

  static CStdString m_diskLabel;
  static CStdString m_diskPath;

  CLibcdio* m_cdio;
};
}

class CDetectDisc : public CJob
{
public:
  CDetectDisc(const CStdString &strPath, bool bautorun);
  bool DoWork();

private:
  CStdString  m_strPath;
  bool        m_bautorun;
};

#endif
