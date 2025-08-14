/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IListProvider.h"
#include "guilib/GUIStaticItem.h"
#include "jobs/IJobCallback.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

class CFileItem;
class TiXmlElement;
class CVariant;

enum class InfoTagType
{
  VIDEO,
  AUDIO,
  PICTURE,
  PROGRAM,
  PVR,
};

class ISubscriberCallback
{
public:
  enum class Topic
  {
    UNSPECIFIED,
    VIDEO_LIBRARY,
    AUDIO_LIBRARY,
    PLAYER,
  };
  virtual ~ISubscriberCallback() = default;
  virtual bool OnEventPublished(Topic topic = Topic::UNSPECIFIED) = 0;
};

class CDirectoryProvider : public IListProvider,
                           public IJobCallback,
                           public ISubscriberCallback,
                           private ITimerCallback
{
public:
  enum class BrowseMode
  {
    NEVER,
    AUTO, // add browse item if list is longer than given limit
    ALWAYS
  };

  CDirectoryProvider(const TiXmlElement* element, int parentID);
  explicit CDirectoryProvider(const CDirectoryProvider& other);
  ~CDirectoryProvider() override;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) override;
  void Reset() override;
  bool OnClick(const std::shared_ptr<CGUIListItem>& item) override;
  bool OnPlay(const std::shared_ptr<CGUIListItem>& item) override;
  bool OnInfo(const std::shared_ptr<CGUIListItem>& item) override;
  bool OnContextMenu(const std::shared_ptr<CGUIListItem>& item) override;
  bool IsUpdating() const override;
  void FreeResources(bool immediately) override;

  // callback from directory job
  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;

  // ISubscriberCallback implementation
  bool OnEventPublished(Topic topic = Topic::UNSPECIFIED) override;

  // Implementation detail.
  class CSubscriber;

private:
  enum class UpdateState
  {
    OK,
    INVALIDATED,
    DONE
  };

  void StartDirectoryJob();

  // ITimerCallback implementation
  void OnTimeout() override;

  UpdateState m_updateState{UpdateState::OK};
  unsigned int m_jobID = 0;
  bool m_jobPending{false};
  std::chrono::time_point<std::chrono::system_clock> m_lastJobStartedAt;
  CTimer m_nextJobTimer;

  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_url;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_target;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortMethod;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_sortOrder;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_limit;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_browse;
  std::string m_currentUrl;
  std::string m_currentTarget; ///< \brief node.target property on the list as a whole
  SortDescription m_currentSort;
  unsigned int m_currentLimit{0};
  BrowseMode m_currentBrowse{BrowseMode::AUTO};
  std::vector<CGUIStaticItemPtr> m_items;
  std::vector<InfoTagType> m_itemTypes;
  mutable CCriticalSection m_section;

  bool UpdateURL();
  bool UpdateLimit();
  bool UpdateSort();
  bool UpdateBrowse();

  std::string GetTarget(const CFileItem& item) const;

  mutable CCriticalSection m_subscriptionSection;
  std::unique_ptr<CSubscriber> m_subscriber;
};
