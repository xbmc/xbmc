/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/UnwatchedGUIInfo.h"

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;

bool CUnwatchedGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CUnwatchedGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  if (!item->HasVideoInfoTag())
    return false;
  
  const CVideoInfoTag* tag = item->GetVideoInfoTag();
  
  if (tag->GetPlayCount() > 0)
    return false;
  
  if (tag->m_type != MediaTypeMovie && tag->m_type != MediaTypeEpisode)
    return false;
  
  std::shared_ptr<CSettingList> setting(std::dynamic_pointer_cast<CSettingList>(
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS)));
  
  if (!setting)
    return false;
  
  switch (info.m_info)
  {
    case PLAYER_TITLE:
    case VIDEOPLAYER_TITLE:
    case LISTITEM_LABEL:
    case LISTITEM_TITLE:
    case VIDEOPLAYER_ORIGINALTITLE:
    case LISTITEM_ORIGINALTITLE:
      if (tag->m_type == MediaTypeEpisode && !setting->FindIntInList(CSettings::VIDEOLIBRARY_NAMES_SHOW_UNWATCHED_EPISODE))
      {
        std::string label = item->GetLabel();
        value = label.substr(0, label.find('.'));
        return true;
      }
      break;
    case LISTITEM_PLOT:
      if ((tag->m_type == MediaTypeMovie && !setting->FindIntInList(CSettings::VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_MOVIES))
        || (tag->m_type == MediaTypeEpisode && !setting->FindIntInList(CSettings::VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_TVSHOWEPISODES))
      )
      {
        value = g_localizeStrings.Get(20370);
        return true;
      }
      break;
  }
  
  return false;
}

bool CUnwatchedGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CUnwatchedGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}
