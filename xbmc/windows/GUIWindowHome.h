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

#include "guilib/GUIWindow.h"
#include "interfaces/IAnnouncer.h"
#include "utils/Job.h"

class CGUIWindowHome :
      public CGUIWindow,
      public ANNOUNCEMENT::IAnnouncer,
      public IJobCallback
{
public:
  CGUIWindowHome(void);
  virtual ~CGUIWindowHome(void);
  virtual void OnInitWindow();
  virtual void Announce(ANNOUNCEMENT::EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  virtual bool OnMessage(CGUIMessage& message);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
private:
  int m_updateRA; // flag for which recently added items needs to be queried
  void AddRecentlyAddedJobs(int flag);

  bool m_recentlyAddedRunning;
  int m_cumulativeUpdateFlag;
};
