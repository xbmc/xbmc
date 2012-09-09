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

#include "DownloadQueue.h"
#include "system.h"

#define MAX_DOWNLOAD_QUEUES  3

class CDownloadQueueManager
{
public:
  CDownloadQueueManager();
  virtual ~CDownloadQueueManager(void);

  VOID Initialize();
  TICKET RequestContent(const CStdString& aUrl, IDownloadQueueObserver* aObserver);
  TICKET RequestFile(const CStdString& aUrl, IDownloadQueueObserver* aObserver);
  TICKET RequestFile(const CStdString& aUrl, const CStdString& aFilePath, IDownloadQueueObserver* aObserver);
  void CancelRequests(IDownloadQueueObserver *aObserver);

protected:

  CDownloadQueue* GetNextDownloadQueue();

  typedef std::vector<CDownloadQueue*> QUEUEPOOL;
  QUEUEPOOL m_queues;

  CCriticalSection m_critical;
};

// Single global instance of class is in cpp file
extern CDownloadQueueManager g_DownloadManager;
