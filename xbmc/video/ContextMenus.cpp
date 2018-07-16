/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"
#include "Application.h"
#include "Autorun.h"
#include "Util.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"


namespace CONTEXTMENU
{

CVideoInfo::CVideoInfo(MediaType mediaType)
    : CStaticContextMenuAction(19033), m_mediaType(mediaType) {}

bool CVideoInfo::IsVisible(const CFileItem& item) const
{
  if (!item.HasVideoInfoTag())
    return false;

  if (item.IsPVRRecording())
    return false; // pvr recordings have its own implementation for this

  return item.GetVideoInfoTag()->m_type == m_mediaType;
}

bool CVideoInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogVideoInfo::ShowFor(*item);
  return true;
}

bool CRemoveResumePoint::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  return CGUIWindowVideoBase::HasResumeItemOffset(&item);
}

bool CRemoveResumePoint::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().ResetResumePoint(item);
  return true;
}

bool CMarkWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder) // Only allow video db content, video and recording folders to be updated recursively
  {
    if (item.HasVideoInfoTag())
      return item.IsVideoDb();
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return CUtil::IsTVRecording(item.GetPath());
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() == 0;
}

bool CMarkWatched::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, true);
  return true;
}

bool CMarkUnWatched::IsVisible(const CFileItem& item) const
{
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder) // Only allow video db content, video and recording folders to be updated recursively
  {
    if (item.HasVideoInfoTag())
      return item.IsVideoDb();
    else if (item.GetProperty("IsVideoFolder").asBoolean())
      return true;
    else
      return CUtil::IsTVRecording(item.GetPath());
  }
  else if (!item.HasVideoInfoTag())
    return false;

  return item.GetVideoInfoTag()->GetPlayCount() > 0;
}

bool CMarkUnWatched::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, false);
  return true;
}

std::string CResume::GetLabel(const CFileItem& item) const
{
  return CGUIWindowVideoBase::GetResumeString(item.GetItemToPlay());
}

bool CResume::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  return CGUIWindowVideoBase::HasResumeItemOffset(&item);
}

static void SetPathAndPlay(CFileItem& item)
{
  if (item.IsVideoDb())
  {
    item.SetProperty("original_listitem_url", item.GetPath());
    item.SetPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
  }
  item.SetProperty("check_resume", false);

  if (item.IsLiveTV()) // pvr tv or pvr radio?
    g_application.PlayMedia(item, "", PLAYLIST_NONE);
  else
    CServiceBroker::GetPlaylistPlayer().Play(std::make_shared<CFileItem>(item), "");
}

bool CResume::Execute(const CFileItemPtr& itemIn) const
{
  CFileItem item(itemIn->GetItemToPlay());
#ifdef HAS_DVD_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, false);
#endif

  item.m_lStartOffset = STARTOFFSET_RESUME;
  SetPathAndPlay(item);
  return true;
};

std::string CPlay::GetLabel(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsLiveTV())
    return g_localizeStrings.Get(19000); // Switch to channel
  if (CGUIWindowVideoBase::HasResumeItemOffset(&item))
    return g_localizeStrings.Get(12021); // Play from beginning
  return g_localizeStrings.Get(208); // Play
}

bool CPlay::IsVisible(const CFileItem& itemIn) const
{
  CFileItem item(itemIn.GetItemToPlay());
  if (item.IsDeleted()) // e.g. trashed pvr recording
    return false;

  if (item.m_bIsFolder)
    return false; //! @todo implement

  return item.IsVideo() || item.IsLiveTV() || item.IsDVD() || item.IsCDDA();
}

bool CPlay::Execute(const CFileItemPtr& itemIn) const
{
  CFileItem item(itemIn->GetItemToPlay());
#ifdef HAS_DVD_DRIVE
  if (item.IsDVD() || item.IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item.GetPath(), true, true);
#endif
  SetPathAndPlay(item);
  return true;
};

}
