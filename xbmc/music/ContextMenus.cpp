/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/MusicUtils.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "tags/MusicInfoTag.h"

#include <utility>

using namespace CONTEXTMENU;

CMusicInfo::CMusicInfo(MediaType mediaType)
  : CStaticContextMenuAction(19033), m_mediaType(std::move(mediaType))
{
}

bool CMusicInfo::IsVisible(const CFileItem& item) const
{
  return (item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == m_mediaType) ||
         (m_mediaType == MediaTypeArtist && item.IsVideoDb() && item.HasProperty("artist_musicid")) ||
         (m_mediaType == MediaTypeAlbum && item.IsVideoDb() && item.HasProperty("album_musicid"));
}

bool CMusicInfo::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CGUIDialogMusicInfo::ShowFor(item.get());
  return true;
}

bool CMusicPlay::IsVisible(const CFileItem& item) const
{
  return MUSIC_UTILS::IsItemPlayable(item);
}

bool CMusicPlay::Execute(const std::shared_ptr<CFileItem>& item) const
{
  MUSIC_UTILS::PlayItem(item);
  return true;
}

bool CMusicPlayNext::IsVisible(const CFileItem& item) const
{
  return MUSIC_UTILS::IsItemPlayable(item);
}

bool CMusicPlayNext::Execute(const std::shared_ptr<CFileItem>& item) const
{
  MUSIC_UTILS::QueueItem(item, MUSIC_UTILS::QueuePosition::POSITION_BEGIN);
  return true;
}

bool CMusicQueue::IsVisible(const CFileItem& item) const
{
  return MUSIC_UTILS::IsItemPlayable(item);
}

bool CMusicQueue::Execute(const std::shared_ptr<CFileItem>& item) const
{
  MUSIC_UTILS::QueueItem(item, MUSIC_UTILS::QueuePosition::POSITION_END);
  return true;
}
