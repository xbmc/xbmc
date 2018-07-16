/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>

#include "ViewState.h"
#include "events/IEvent.h"
#include "windowing/GraphicContext.h"
#include "settings/lib/ISubSettings.h"
#include "settings/lib/Setting.h"
#include "threads/CriticalSection.h"

class TiXmlNode;

class CViewStateSettings : public ISubSettings
{
public:
  static CViewStateSettings& GetInstance();

  bool Load(const TiXmlNode *settings) override;
  bool Save(TiXmlNode *settings) const override;
  void Clear() override;

  const CViewState* Get(const std::string &viewState) const;
  CViewState* Get(const std::string &viewState);

  SettingLevel GetSettingLevel() const { return m_settingLevel; }
  void SetSettingLevel(SettingLevel settingLevel);
  void CycleSettingLevel();
  SettingLevel GetNextSettingLevel() const;

  EventLevel GetEventLevel() const { return m_eventLevel; }
  void SetEventLevel(EventLevel eventLevel);
  void CycleEventLevel();
  EventLevel GetNextEventLevel() const;
  bool ShowHigherEventLevels() const { return m_eventShowHigherLevels; }
  void SetShowHigherEventLevels(bool showHigherEventLevels) { m_eventShowHigherLevels = showHigherEventLevels; }
  void ToggleShowHigherEventLevels() { m_eventShowHigherLevels = !m_eventShowHigherLevels; }

protected:
  CViewStateSettings();
  CViewStateSettings(const CViewStateSettings&) = delete;
  CViewStateSettings& operator=(CViewStateSettings const&) = delete;
  ~CViewStateSettings() override;

private:
  std::map<std::string, CViewState*> m_viewStates;
  SettingLevel m_settingLevel = SettingLevel::Standard;
  EventLevel m_eventLevel = EventLevel::Basic;
  bool m_eventShowHigherLevels = true;
  mutable CCriticalSection m_critical;

  void AddViewState(const std::string& strTagName, int defaultView = DEFAULT_VIEW_LIST, SortBy defaultSort = SortByLabel);
};
