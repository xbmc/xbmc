/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIWindow.h"
#include "interfaces/IAnnouncer.h"
#include "utils/Job.h"

#include <vector>

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
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override;

  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  void FrameMove() override; //! @todo

  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

private:
  //void RegisterChildren(const std::vector<CGUIControl*> children);

  int m_updateRA; // flag for which recently added items needs to be queried
  void AddRecentlyAddedJobs(int flag);

  bool m_recentlyAddedRunning = false;
  int m_cumulativeUpdateFlag = 0;
};
