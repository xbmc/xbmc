/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowMusicPlaylist.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "guilib/GUIMessage.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ContentNames.h"
#include "utils/ItemProperties.h"
#include "utils/LabelFormatter.h"
#include "utils/StringUtils.h"

using namespace KODI;

CGUIWindowMusicPlayList::CGUIWindowMusicPlayList() : CGUIWindowPlayList(PLAYLIST::Audio)
{
  m_musicInfoLoader.SetObserver(this);
}

CGUIWindowMusicPlayList::~CGUIWindowMusicPlayList() = default;

bool CGUIWindowMusicPlayList::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    m_musicInfoLoader.UseCacheOnHD("special://temp/archive_cache/MusicPlaylist.fi");
  }
  return CGUIWindowPlayList::OnMessage(message);
}

bool CGUIWindowMusicPlayList::StopLoadingItems()
{
  if (!m_musicInfoLoader.IsLoading())
  {
    return false;
  }
  m_musicInfoLoader.StopThread();
  return true;
}

void CGUIWindowMusicPlayList::StartLoadingItems()
{
  m_musicInfoLoader.Load(*m_vecItems);
}

void CGUIWindowMusicPlayList::OnItemLoaded(CFileItem* pItem)
{
  if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->Loaded())
  { // set label 1+2 from tags
    const std::shared_ptr<CSettings> settings =
        CServiceBroker::GetSettingsComponent()->GetSettings();
    std::string strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_NOWPLAYINGTRACKFORMAT);
    if (strTrack.empty())
      strTrack = settings->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
    CLabelFormatter formatter(strTrack, "%D");
    formatter.FormatLabels(pItem);
  }
  else
  {
    // Our tag may have a duration even if its not loaded
    if (pItem->HasMusicInfoTag() && pItem->GetMusicInfoTag()->GetDuration())
    {
      int nDuration = pItem->GetMusicInfoTag()->GetDuration();
      if (nDuration > 0)
        pItem->SetLabel2(StringUtils::SecondsToTimeString(nDuration));
    }
    else if (pItem->GetLabel().empty()) // pls labels come in preformatted
    {
      // No music info, so show the file name
      std::string str;
      str = CUtil::GetTitleFromPath(pItem->GetPath());
      str = StringUtils::Format("{:02}. {} ", pItem->GetProperty(ITEM::PROPERTY::PLAYLIST_DISPLAY_ORDER).asInteger(), str);
      pItem->SetLabel(str);
    }
  }
}

bool CGUIWindowMusicPlayList::Update(const std::string& strDirectory,
                                     bool updateFilterPath /* = true */)
{
  StopLoadingItems();

  if (!CGUIWindowMusicBase::Update(strDirectory, updateFilterPath))
    return false;

  if (m_vecItems->GetContent().empty())
    m_vecItems->SetContent(MEDIA::CONTENT::SONGS);

  StartLoadingItems();
  return true;
}
