/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IListProvider.h"
#include "addons/AddonEvents.h"
#include "addons/RepositoryUpdater.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIStaticItem.h"
#include "interfaces/IAnnouncer.h"
#include "threads/CriticalSection.h"
#include "utils/Job.h"

#include <string>
#include <vector>

class CFileItem;
class TiXmlElement;
class CVariant;

namespace PVR
{
  enum class PVREvent;
}

enum class InfoTagType
{
  VIDEO,
  AUDIO,
  PICTURE,
  PROGRAM,
  PVR,
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

  enum class BrowseMode
  {
    NEVER,
    AUTO, // add browse item if list is longer than given limit
    ALWAYS
  };

  CDirectoryProvider(const TiXmlElement *element, int parentID);
  explicit CDirectoryProvider(const CDirectoryProvider& other);
  ~CDirectoryProvider() override;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override;
  void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) override;
  void Reset() override;
  bool OnClick(const std::shared_ptr<CGUIListItem>& item) override;
  bool OnPlay(const std::shared_ptr<CGUIListItem>& item) override;
  bool OnInfo(const std::shared_ptr<CFileItem>& item);
  bool OnContextMenu(const std::shared_ptr<CFileItem>& item);
  bool OnInfo(const std::shared_ptr<CGUIListItem>& item) override;
  bool OnContextMenu(const std::shared_ptr<CGUIListItem>& item) override;
  bool IsUpdating() const override;
  void FreeResources(bool immediately) override;

  // callback from directory job
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;
private:
  UpdateState m_updateState = OK;
  unsigned int m_jobID = 0;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_url;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_target;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortMethod;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortOrder;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_limit;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_browse;
  std::string      m_currentUrl;
  std::string      m_currentTarget;   ///< \brief node.target property on the list as a whole
  SortDescription  m_currentSort;
  unsigned int m_currentLimit{0};
  BrowseMode m_currentBrowse{BrowseMode::AUTO};
  std::vector<CGUIStaticItemPtr> m_items;
  std::vector<InfoTagType> m_itemTypes;
  mutable CCriticalSection m_section;

  bool UpdateURL();
  bool UpdateLimit();
  bool UpdateSort();
  bool UpdateBrowse();
  void OnAddonEvent(const ADDON::AddonEvent& event);
  void OnAddonRepositoryEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event);
  void OnPVRManagerEvent(const PVR::PVREvent& event);
  void OnFavouritesEvent(const CFavouritesService::FavouritesUpdated& event);
  std::string GetTarget(const CFileItem& item) const;

  CCriticalSection m_subscriptionSection;
  bool m_isSubscribed{false};
};
