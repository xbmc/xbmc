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

#include "IDirectory.h"
#include "CMythSession.h"

class CDateTime;

namespace DIRECTORY
{


class CCMythDirectory
  : public IDirectory
{
public:
  CCMythDirectory();
  virtual ~CCMythDirectory();

  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

private:
  void Release();
  bool GetGuide(const CStdString& base, CFileItemList &items);
  bool GetGuideForChannel(const CStdString& base, int ChanNum, CFileItemList &items);
  bool GetRecordings(const CStdString& base, CFileItemList &items);
  bool GetChannels  (const CStdString& base, CFileItemList &items);
  bool GetChannelsDb(const CStdString& base, CFileItemList &items);

  CStdString GetValue(char* str)           { return m_session->GetValue(str); }
  int        GetValue(int integer)           { return m_session->GetValue(integer); }
  CDateTime  GetValue(cmyth_timestamp_t t);

  XFILE::CCMythSession* m_session;
  DllLibCMyth*          m_dll;
  cmyth_database_t      m_database;
  cmyth_recorder_t      m_recorder;
  cmyth_proginfo_t      m_program;
};

}
