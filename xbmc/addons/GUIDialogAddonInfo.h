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

#include "guilib/GUIDialog.h"
#include "addons/IAddon.h"
#include "utils/Job.h"

class CGUIDialogAddonInfo :
      public CGUIDialog,
      public IJobCallback
{
public:
  CGUIDialogAddonInfo(void);
  virtual ~CGUIDialogAddonInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return m_item; }
  virtual bool HasListItems() const { return true; }

  static bool ShowForItem(const CFileItemPtr& item);

  // job callback
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
protected:
  void OnInitWindow();

  /*! \brief Set the item to display addon info on.
   \param item to display
   \return true if we can display information, false otherwise
   */
  bool SetItem(const CFileItemPtr &item);
  void UpdateControls();

  void OnUpdate();
  void OnInstall();
  void OnUninstall();
  void OnEnable(bool enable);
  void OnSettings();
  void OnChangeLog();
  void OnRollback();

  CFileItemPtr m_item;
  ADDON::AddonPtr m_addon;
  ADDON::AddonPtr m_localAddon;
  unsigned int m_jobid;
  bool m_changelog;

  // rollback data
  void GrabRollbackVersions();
  std::vector<CStdString> m_rollbackVersions;
};

