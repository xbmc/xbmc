/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32PC
#ifndef _WIN32_DETECT_DVD_TYPE_H_
#define _WIN32_DETECT_DVD_TYPE_H_

#include "WIN32Util.h"
#include "FileSystem/cdioSupport.h"

namespace MEDIA_DETECT
{
class CWINDetectDVDMedia
{
public:
  static CWINDetectDVDMedia* GetInstance();
  static void Destroy();

  void AddMedia(CStdString& strDrive);
  void RemoveMedia(CStdString& strDrive);
  CCdInfo* GetCdInfo(CStdString& strDrive);
  bool IsAudio(CStdString strDrive="");
  bool IsDiscInDrive(CStdString strDrive="");
  DWORD GetTrayState(CStdString strDrive="");

private:
  static CWINDetectDVDMedia* m_instance;
  CWINDetectDVDMedia();
  CWINDetectDVDMedia(const CWINDetectDVDMedia& cc);
  ~CWINDetectDVDMedia();

  void RemoveAllMedia();
  CStdString GetDrive(CStdString strDrive="");
  CStdString GetDevice(CStdString strDrive="");

  std::map<char,CCdInfo*> m_mapCdInfo;
  CCriticalSection m_critsec;

};
}
#endif
#endif
