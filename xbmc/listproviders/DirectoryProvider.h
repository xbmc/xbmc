/*
 *      Copyright (C) 2013-2017 Team Kodi
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

#pragma once

#include <string>
#include <vector>
#include "addons/AddonEvents.h"
#include "IListProvider.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIStaticItem.h"
#include "pvr/PVREvent.h"
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
  ~CDirectoryProvider() override;

  bool Update(bool forceRefresh) override;
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data) override;
  void Fetch(std::vector<CGUIListItemPtr> &items) override;
  void Reset() override;
  bool OnClick(const CGUIListItemPtr &item) override;
  bool OnInfo(const CGUIListItemPtr &item) override;
  bool OnContextMenu(const CGUIListItemPtr &item) override;
  bool IsUpdating() const override;

  // callback from directory job
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;
private:
  UpdateState      m_updateState;
  bool             m_isAnnounced;
  unsigned int     m_jobID;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_url;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_target;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortMethod;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortOrder;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_limit;
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
  void OnPVRManagerEvent(const PVR::PVREvent& event);
  void OnFavouritesEvent(const CFavouritesService::FavouritesUpdated& event);
};
