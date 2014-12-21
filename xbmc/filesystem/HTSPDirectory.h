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

#pragma once
#include "IDirectory.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "HTSPSession.h"
#include <memory>

class CURL;
class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace HTSP
{
  class CHTSPDirectorySession
      : public CThread
  {
    public:
      bool                    GetEvent(SEvent& event, uint32_t id);
      SChannels               GetChannels();
      SChannels               GetChannels(int tag);
      SChannels               GetChannels(STag &tag);
      STags                   GetTags();
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
      SChannels               m_channels;
      STags                   m_tags;
      SEvents                 m_events;
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
}

namespace XFILE
{

  class CHTSPDirectory : public IDirectory
  {
    public:
      CHTSPDirectory(void);
      virtual ~CHTSPDirectory(void);
      virtual bool GetDirectory(const CURL& url, CFileItemList &items);
      virtual DIR_CACHE_TYPE GetCacheType(const CURL& url) const { return DIR_CACHE_ONCE; };
    private:
      bool GetChannels(const CURL& base, CFileItemList &items);
      bool GetChannels(const CURL& base, CFileItemList &items, HTSP::SChannels channels, int tag);
      bool GetTag     (const CURL& base, CFileItemList &items);

      HTSP::CHTSPDirectorySession* m_session;
  };
}

