/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
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
  CCriticalSection m_critical;

  void AddViewState(const std::string& strTagName, int defaultView = DEFAULT_VIEW_LIST, SortBy defaultSort = SortByLabel);
};
