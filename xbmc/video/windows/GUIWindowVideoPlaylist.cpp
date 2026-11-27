/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowVideoPlaylist.h"

#include "FileItemList.h"
#include "application/ApplicationPlayLists.h"
#include "playlists/PlayList.h"
#include "settings/MediaSourceSettings.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#include "video/guilib/VideoPlayActionProcessor.h"

using namespace KODI;
using KODI::MEDIA::MediaSection;

CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist() : CGUIWindowPlayList(PLAYLIST::Video)
{
}

CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist() = default;

void CGUIWindowVideoPlaylist::OnPrepareFileItems(CFileItemList& items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);

  if (items.IsEmpty())
    return;

  if (!VIDEO::IsVideoDb(items) && !items.IsVirtualDirectoryRoot())
  { // load info from the database
    std::string label;
    if (items.GetLabel().empty() &&
        m_rootDir.IsSource(items.GetPath(),
                           &CMediaSourceSettings::GetInstance().GetSources(MediaSection::VIDEO),
                           &label))
      items.SetLabel(label);
    if (!items.IsSourcesPath() && !items.IsLibraryFolder())
      LoadVideoInfo(items, m_database);
  }
}

bool CGUIWindowVideoPlaylist::OnSelect(int iItem)
{
  // selecting an entry plays it, whatever the default select action says
  return OnPlayMedia(iItem);
}

namespace
{
class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessor
{
public:
  CVideoPlayActionProcessor(const std::shared_ptr<CFileItem>& item,
                            CApplicationPlayLists& playLists,
                            int itemIndex,
                            const std::string& player)
    : VIDEO::GUILIB::CVideoPlayActionProcessor(item),
      m_playLists(playLists), m_itemIndex(itemIndex), m_player(player)
  {
  }

protected:
  bool OnResumeSelected() override
  {
    const auto playlistItem{m_playLists.GetPlayList(PLAYLIST::Video)[m_itemIndex]};
    if (!playlistItem)
      return false;
    playlistItem->SetStartOffset(STARTOFFSET_RESUME);
    if (playlistItem->HasVideoInfoTag() && GetItem()->HasVideoInfoTag())
      playlistItem->GetVideoInfoTag()->SetResumePoint(
          GetItem()->GetVideoInfoTag()->GetResumePoint());

    m_playLists.PlayFrom(PLAYLIST::Video, m_itemIndex, {.player = m_player});
    return true;
  }

  bool OnPlaySelected() override
  {
    m_playLists.PlayFrom(PLAYLIST::Video, m_itemIndex, {.player = m_player});
    return true;
  }

private:
  CApplicationPlayLists& m_playLists;
  const int m_itemIndex{-1};
  const std::string m_player;
};
} // namespace

void CGUIWindowVideoPlaylist::PlayEntry(int iItem, const std::string& player)
{
  CVideoPlayActionProcessor proc{m_vecItems->Get(iItem), *m_playLists, ListPosition(iItem), player};
  proc.ProcessDefaultAction();
}