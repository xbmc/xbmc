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

  virtual bool GetDirectory(const CURL& url, CFileItemList &items);
  virtual bool Exists(const CURL& url);
  virtual bool AllowAll() const { return true; }
  virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const;

  static bool SupportsWriteFileOperations(const std::string& strPath);
  static bool IsLiveTV(const std::string& strPath);

private:
  void Release();
  bool GetGuide(const std::string& base, CFileItemList &items);
  bool GetGuideForChannel(const std::string& base, CFileItemList &items, const int channelNumber);
  bool GetRecordings(const std::string& base, CFileItemList &items, enum FilterType type = ALL, const std::string& filter = "");
  bool GetTvShowFolders(const std::string& base, CFileItemList &items);
  bool GetChannels(const std::string& base, CFileItemList &items);

  std::string GetValue(char* str)           { return m_session->GetValue(str); }
  CDateTime   GetValue(cmyth_timestamp_t t) { return m_session->GetValue(t); }
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
