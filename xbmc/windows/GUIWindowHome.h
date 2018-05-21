#pragma once

/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIWindow.h"
#include "interfaces/IAnnouncer.h"
#include "utils/Job.h"

class CVariant;

class CGUIWindowHome :
      public CGUIWindow,
      public ANNOUNCEMENT::IAnnouncer,
      public IJobCallback
{
public:
  CGUIWindowHome(void);
  ~CGUIWindowHome(void) override;
  void OnInitWindow() override;
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;
private:
  int m_updateRA; // flag for which recently added items needs to be queried
  void AddRecentlyAddedJobs(int flag);

  bool m_recentlyAddedRunning;
  int m_cumulativeUpdateFlag;
};
