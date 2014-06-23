#pragma once

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

#include "Job.h"
#include <string>

class CInfoLoader : public IJobCallback
{
public:
  CInfoLoader(unsigned int timeToRefresh = 5 * 60 * 1000);
  virtual ~CInfoLoader();

  std::string GetInfo(int info);
  void Refresh();

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
protected:
  virtual CJob *GetJob() const=0;
  virtual std::string TranslateInfo(int info) const;
  virtual std::string BusyInfo(int info) const;
private:
  unsigned int m_refreshTime;
  unsigned int m_timeToRefresh;
  bool m_busy;
};
