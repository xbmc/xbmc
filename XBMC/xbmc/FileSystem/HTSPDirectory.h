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
#pragma once
#include "IDirectory.h"
#include "Thread.h"
#include "utils/CriticalSection.h"
#include "utils/Event.h"
#include "URL.h"
#include "HTSPSession.h"

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;

namespace DIRECTORY
{
  class CHTSPDirectorySession 
      : public CThread
  {
    public: 
      bool                    GetEvent(CHTSPSession::SEvent& event, uint32_t id);
      CHTSPSession::SChannels GetChannels();
      CHTSPSession::SChannels GetChannels(int tag);
      CHTSPSession::SChannels GetChannels(CHTSPSession::STag &tag);
      CHTSPSession::STags     GetTags();
      htsmsg_t*               ReadResult(htsmsg_t* m);


      static CHTSPDirectorySession* Acquire(const CURL& url);
      static void                   Release(CHTSPDirectorySession* &session);
      static void                   CheckIdle(DWORD idle = 60000);

    protected:
       CHTSPDirectorySession();
      ~CHTSPDirectorySession();

      bool   Open(const CURL& url);
      void   Close();

    private:
      virtual void Process();
      CHTSPSession            m_session;
      CHTSPSession::SChannels m_channels;
      CHTSPSession::STags     m_tags;
      CHTSPSession::SEvents   m_events;
      CCriticalSection        m_section;
      CEvent                  m_started;

      struct SMessage
      {
        CEvent   * event;
        htsmsg_t * msg;
      };
      typedef std::map<int, SMessage> SMessages;

      SMessages m_queue;
  };

  class CHTSPDirectory : public IDirectory
  {
    public:
      CHTSPDirectory(void);
      virtual ~CHTSPDirectory(void);
      virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
      virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const { return DIR_CACHE_ONCE; };
    private:
      bool GetChannels(const CURL& base, CFileItemList &items);
      bool GetChannels(const CURL& base, CFileItemList &items, CHTSPSession::SChannels channels);
      bool GetTag     (const CURL& base, CFileItemList &items);

      CHTSPDirectorySession* m_session;
  };
}

