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

#include "IFile.h"
#include "ILiveTV.h"
#include "CMythSession.h"
#include "VideoInfoTag.h"

#include <queue>

extern "C" {
#include "lib/libcmyth/cmyth.h"
}

class DllLibCMyth;

namespace XFILE
{


class CCMythFile
  : public  IFile
  ,         ILiveTVInterface
  ,         IRecordable
  , private CCMythSession::IEventListener
{
public:
  CCMythFile();
  virtual ~CCMythFile();
  virtual bool          Open(const CURL& url);
  virtual __int64       Seek(__int64 pos, int whence=SEEK_SET);
  virtual __int64       GetPosition();
  virtual __int64       GetLength();
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual unsigned int  Read(void* buffer, __int64 size);
  virtual CStdString    GetContent() { return ""; }
  virtual bool          SkipNext();

  virtual bool          Delete(const CURL& url);
  virtual bool          Exists(const CURL& url);

  virtual ILiveTVInterface* GetLiveTV() {return (ILiveTVInterface*)this;}

  virtual bool           NextChannel();
  virtual bool           PrevChannel();
  virtual bool           SelectChannel(unsigned int channel);

  virtual int            GetTotalTime();
  virtual int            GetStartTime();

  virtual bool           UpdateItem(CFileItem& item);

  virtual IRecordable*   GetRecordable() {return (IRecordable*)this;}

  virtual bool           CanRecord();
  virtual bool           IsRecording();
  virtual bool           Record(bool bOnOff);

  virtual bool           GetCommBreakList(cmyth_commbreaklist_t& commbreaklist);

protected:
  virtual void OnEvent(int event, const std::string& data);

  bool HandleEvents();
  bool ChangeChannel(int direction, const CStdString &channel);

  bool SetupConnection(const CURL& url, bool control, bool event, bool database);
  bool SetupRecording(const CURL& url);
  bool SetupLiveTV(const CURL& url);
  bool SetupFile(const CURL& url);

  CStdString GetValue(char* str) { return m_session->GetValue(str); }

  CCMythSession*    m_session;
  DllLibCMyth*      m_dll;
  cmyth_conn_t      m_control;
  cmyth_database_t  m_database;
  cmyth_recorder_t  m_recorder;
  cmyth_proginfo_t  m_program;
  cmyth_file_t      m_file;
  cmyth_timestamp_t m_starttime;
  CStdString        m_filename;
  CVideoInfoTag     m_infotag;

  CCriticalSection  m_section;
  std::queue<std::pair<int, std::string> > m_events;

  bool              m_recording;

  DWORD             m_timestamp;
};

}


