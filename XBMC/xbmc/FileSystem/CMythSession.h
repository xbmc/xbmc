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

#include "utils/Thread.h"

typedef struct cmyth_ringbuf   *cmyth_ringbuf_t;
typedef struct cmyth_conn      *cmyth_conn_t;
typedef struct cmyth_recorder  *cmyth_recorder_t;
typedef struct cmyth_proginfo  *cmyth_proginfo_t;
typedef struct cmyth_proglist  *cmyth_proglist_t;
typedef struct cmyth_file      *cmyth_file_t;
typedef struct cmyth_database  *cmyth_database_t;
typedef struct cmyth_timestamp *cmyth_timestamp_t;

class DllLibCMyth;
class CDateTime;
class CURL;

namespace XFILE
{

class CCMythSession
  : private CThread
{
public:
  static CCMythSession* AquireSession(const CURL& url);
  static void           ReleaseSession(CCMythSession*);
  static void           CheckIdle();

  class IEventListener
  {
  public:
    virtual ~IEventListener() {};
    virtual void OnEvent(int event, const std::string& data)=0;
  };

  bool             SetListener(IEventListener *listener);
  cmyth_conn_t     GetControl();
  cmyth_database_t GetDatabase();
  DllLibCMyth*     GetLibrary();

  bool             UpdateItem(CFileItem &item, cmyth_proginfo_t info);

  CDateTime        GetValue(cmyth_timestamp_t t);
  CStdString       GetValue(char* str);
  int              GetValue(int integer);
private:
  CCMythSession(const CURL& url);
  ~CCMythSession();

  virtual void Process();

  bool             CanSupport(const CURL& url);
  void             Disconnect();

  IEventListener*  m_listener;
  cmyth_conn_t     m_control;
  cmyth_conn_t     m_event;
  cmyth_database_t m_database;
  CStdString       m_hostname;
  CStdString       m_username;
  CStdString       m_password;
  int              m_port;
  DllLibCMyth*     m_dll;
  CCriticalSection m_section;
  DWORD            m_timestamp;

  static CCriticalSection            m_section_session;
  static std::vector<CCMythSession*> m_sessions;
};

}
