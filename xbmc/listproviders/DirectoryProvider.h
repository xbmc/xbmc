/*
 *      Copyright (C) 2013 Team XBMC
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

#pragma once

#include <string>
#include <vector>
#include "addons/AddonEvents.h"
#include "IListProvider.h"
#include "guilib/GUIStaticItem.h"
#include "pvr/PVRManagerState.h"
#include "utils/Job.h"
#include "threads/CriticalSection.h"
#include "interfaces/IAnnouncer.h"

class TiXmlElement;
class CVariant;

enum class InfoTagType
{
  VIDEO,
  AUDIO,
  PICTURE,
  PROGRAM
};

class CDirectoryProvider :
  public IListProvider,
  public IJobCallback,
  public ANNOUNCEMENT::IAnnouncer
{
public:
  typedef enum
  {
    OK,
    INVALIDATED,
    DONE
  } UpdateState;

  CDirectoryProvider(const TiXmlElement *element, int parentID);
  virtual ~CDirectoryProvider();

  virtual bool Update(bool forceRefresh) override;
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;
  virtual void Fetch(std::vector<CGUIListItemPtr> &items) const override;
  virtual void Reset(bool immediately = false) override;
  virtual bool OnClick(const CGUIListItemPtr &item) override;
  bool OnInfo(const CGUIListItemPtr &item) override;
  bool OnContextMenu(const CGUIListItemPtr &item) override;
  virtual bool IsUpdating() const override;

  // callback from directory job
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;
private:
  UpdateState      m_updateState;
  bool             m_isAnnounced;
  unsigned int     m_jobID;
  CGUIInfoLabel    m_url;
  CGUIInfoLabel    m_target;
  CGUIInfoLabel    m_sortMethod;
  CGUIInfoLabel    m_sortOrder;
  CGUIInfoLabel    m_limit;
  std::string      m_currentUrl;
  std::string      m_currentTarget;   ///< \brief node.target property on the list as a whole
  SortDescription  m_currentSort;
  unsigned int     m_currentLimit;
  std::vector<CGUIStaticItemPtr> m_items;
  std::vector<InfoTagType> m_itemTypes;
  CCriticalSection m_section;

  bool UpdateURL();
  bool UpdateLimit();
  bool UpdateSort();
  void OnAddonEvent(const ADDON::AddonEvent& event);
  void OnPVRManagerEvent(const PVR::ManagerState& event);
};
