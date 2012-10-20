#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IDirectory.h"
#include "MythSession.h"
#include "XBDateTime.h"

namespace XFILE
{

enum FilterType
{
  MOVIES,
  TV_SHOWS,
  ALL
};

class CMythDirectory
  : public IDirectory
{
public:
  CMythDirectory();
  virtual ~CMythDirectory();

  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
  virtual bool IsAllowed(const CStdString &strFile) const { return true; };
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;

  static bool SupportsWriteFileOperations(const CStdString& strPath);
  static bool IsLiveTV(const CStdString& strPath);

private:
  void Release();
  bool GetGuide(const CStdString& base, CFileItemList &items);
  bool GetGuideForChannel(const CStdString& base, CFileItemList &items, const int channelNumber);
  bool GetRecordings(const CStdString& base, CFileItemList &items, enum FilterType type = ALL, const CStdString& filter = "");
  bool GetTvShowFolders(const CStdString& base, CFileItemList &items);
  bool GetChannels(const CStdString& base, CFileItemList &items);

  CStdString GetValue(char* str)           { return m_session->GetValue(str); }
  CDateTime  GetValue(cmyth_timestamp_t t) { return m_session->GetValue(t); }
  bool IsVisible(const cmyth_proginfo_t program);
  bool IsMovie(const cmyth_proginfo_t program);
  bool IsTvShow(const cmyth_proginfo_t program);

  XFILE::CMythSession*  m_session;
  DllLibCMyth*          m_dll;
  cmyth_database_t      m_database;
  cmyth_recorder_t      m_recorder;
  cmyth_proginfo_t      m_program;
};

}
