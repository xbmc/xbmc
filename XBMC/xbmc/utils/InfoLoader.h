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

#include "StdString.h"
#include "Job.h"

class CInfoLoader;

class CInfoJob : public CJob
{
public:
  CInfoJob(CInfoLoader *loader);
  virtual void DoWork();
private:
  CInfoLoader *m_loader;  
};

class CInfoLoader : public IJobCallback
{
public:
  CInfoLoader(unsigned int timeToRefresh = 5 * 60 * 1000);
  virtual ~CInfoLoader();

  CStdString GetInfo(int info);
  void Refresh();
  
  virtual void OnJobComplete(unsigned int jobID, CJob *job);
  virtual void DoWork()=0;
protected:
  virtual CStdString TranslateInfo(int info) const;
  virtual CStdString BusyInfo(int info) const;
private:
  unsigned int m_refreshTime;
  unsigned int m_timeToRefresh;
  bool m_busy;
};
