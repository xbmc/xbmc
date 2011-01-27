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

#include "DownloadQueue.h"
#include "URIUtils.h"
#include "log.h"

#include "filesystem/File.h"
#include "filesystem/FileCurl.h"

using namespace std;
using namespace XFILE;

WORD CDownloadQueue::m_wNextQueueId = 0;

CDownloadQueue::CDownloadQueue(void) : CThread()
{
  InitializeCriticalSection(&m_critical);
  m_bStop = false;
  m_wQueueId = m_wNextQueueId++;
  m_dwNextItemId = 0;
  CThread::Create(false);
}

void CDownloadQueue::OnStartup()
{
  SetPriority( GetMinPriority() );
}

CDownloadQueue::~CDownloadQueue(void)
{
  DeleteCriticalSection(&m_critical);
}


TICKET CDownloadQueue::RequestContent(const CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
  EnterCriticalSection(&m_critical);

  TICKET ticket(m_wQueueId, m_dwNextItemId++);

  Command request = {ticket, aUrl, "", aObserver};
  m_queue.push(request);

  LeaveCriticalSection(&m_critical);
  return request.ticket;
}

TICKET CDownloadQueue::RequestFile(const CStdString& aUrl, const CStdString& aFilePath, IDownloadQueueObserver* aObserver)
{
  EnterCriticalSection(&m_critical);

  TICKET ticket(m_wQueueId, m_dwNextItemId++);

  Command request = {ticket, aUrl, aFilePath, aObserver};
  m_queue.push(request);

  LeaveCriticalSection(&m_critical);
  return request.ticket;
}

void CDownloadQueue::CancelRequests(IDownloadQueueObserver *aObserver)
{
  EnterCriticalSection(&m_critical);

  CLog::Log(LOGDEBUG, "CancelRequests from observer at %p", aObserver);
  // run through our queue, and create a new queue with the requests
  // from aObserver NULL'd out.
  queue<Command> newQueue;
  while (m_queue.size())
  {
    Command request = m_queue.front();
    m_queue.pop();
    if (request.observer == aObserver)
      request.observer = NULL;
    newQueue.push(request);
  }
  m_queue = newQueue;
  LeaveCriticalSection(&m_critical);
}

TICKET CDownloadQueue::RequestFile(const CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
  EnterCriticalSection(&m_critical);

  CLog::Log(LOGDEBUG, "RequestFile from observer at %p", aObserver);
  // create a temporary destination
  CStdString strExtension;
  URIUtils::GetExtension(aUrl, strExtension);

  TICKET ticket(m_wQueueId, m_dwNextItemId++);

  CStdString strFilePath;
  strFilePath.Format("special://temp/q%d-item%u%s", ticket.wQueueId, ticket.dwItemId, strExtension.c_str());

  Command request = {ticket, aUrl, strFilePath, aObserver};
  m_queue.push(request);

  LeaveCriticalSection(&m_critical);
  return request.ticket;
}

VOID CDownloadQueue::Flush()
{
  EnterCriticalSection(&m_critical);

  m_queue.empty();

  LeaveCriticalSection(&m_critical);
}

void CDownloadQueue::Process()
{
  CLog::Log(LOGNOTICE, "DownloadQueue ready.");

  CFileCurl http;
  bool bSuccess;

  while ( !m_bStop )
  {
    while ( CDownloadQueue::Size() > 0 )
    {
      EnterCriticalSection(&m_critical);

      // get the first item, but don't pop it from our queue
      // so that the download can be interrupted
      Command request = m_queue.front();

      LeaveCriticalSection(&m_critical);

      bool bFileRequest = request.content.length() > 0;
      DWORD dwSize = 0;

      if (bFileRequest)
      {
        CFile::Delete(request.content);
        bSuccess = http.Download(request.location, request.content, &dwSize);
      }
      else
      {
        bSuccess = http.Get(request.location, request.content);
      }

      // now re-grab the item as we may have cancelled our download
      // while we were working
      EnterCriticalSection(&m_critical);

      request = m_queue.front();
      m_queue.pop();

      // if the request has been cancelled our observer will be NULL
      if (NULL != request.observer)
      {
        try
        {
          if (bFileRequest)
          {
            request.observer->OnFileComplete(request.ticket, request.content, dwSize,
                                            bSuccess ? IDownloadQueueObserver::Succeeded : IDownloadQueueObserver::Failed );
          }
          else
          {
            request.observer->OnContentComplete(request.ticket, request.content,
                                                bSuccess ? IDownloadQueueObserver::Succeeded : IDownloadQueueObserver::Failed );
          }
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "exception while updating download observer.");

          if (bFileRequest)
          {
            CFile::Delete(request.content);
          }
        }
      }
      LeaveCriticalSection(&m_critical);
    }

    Sleep(500);
  }

  CLog::Log(LOGNOTICE, "DownloadQueue terminated.");
}

INT CDownloadQueue::Size()
{
  EnterCriticalSection(&m_critical);

  int sizeOfQueue = m_queue.size();

  LeaveCriticalSection(&m_critical);

  return sizeOfQueue;
}

