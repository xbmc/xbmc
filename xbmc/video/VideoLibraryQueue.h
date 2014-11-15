#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
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

#include <map>
#include <set>

#include "FileItem.h"
#include "threads/CriticalSection.h"
#include "utils/JobManager.h"

class CVideoLibraryJob;

class CVideoLibraryQueue : protected CJobQueue
{
public:
  ~CVideoLibraryQueue();

  static CVideoLibraryQueue& Get();

  void MarkAsWatched(const CFileItemPtr &item, bool watched);

  void AddJob(CVideoLibraryJob *job);
  void CancelJob(CVideoLibraryJob *job);
  void CancelAllJobs();

  bool IsRunning() const;

protected:
  // implementation of IJobCallback
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

private:
  CVideoLibraryQueue();
  CVideoLibraryQueue(const CVideoLibraryQueue&);
  CVideoLibraryQueue const& operator=(CVideoLibraryQueue const&);

  typedef std::set<CVideoLibraryJob*> VideoLibraryJobs;
  typedef std::map<std::string, VideoLibraryJobs> VideoLibraryJobMap;
  VideoLibraryJobMap m_jobs;
  CCriticalSection m_critical;
};
