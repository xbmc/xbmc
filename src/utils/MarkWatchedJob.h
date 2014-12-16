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

#include "Job.h"
#include "utils/JobManager.h"

class CMarkWatchedJob : public CJob
{
public:
  CMarkWatchedJob(const CFileItemPtr &item, bool bMark);
private:
  virtual ~CMarkWatchedJob();
  virtual const char *GetType() const { return "markwatched"; }
  virtual bool operator==(const CJob* job) const;
  virtual bool DoWork();
  CFileItemPtr m_item;
  bool m_bMark;
};

class CMarkWatchedQueue: public CJobQueue
{
public:
  static CMarkWatchedQueue &Get();
private:
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
};
