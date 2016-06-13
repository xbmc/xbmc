/*
 *      Copyright (C) 2016 Team Kodi
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

#include "ContextMenus.h"
#include "Application.h"
#include "Autorun.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"


namespace CONTEXTMENU
{

CVideoInfo::CVideoInfo(MediaType mediaType)
    : CStaticContextMenuAction(19033), m_mediaType(mediaType) {}

bool CVideoInfo::IsVisible(const CFileItem& item) const
{
  return item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_type == m_mediaType;
}

bool CVideoInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogVideoInfo::ShowFor(*item);
  return true;
}

bool CMarkWatched::IsVisible(const CFileItem& item) const
{
  if (!item.HasVideoInfoTag())
    return false;
  if (item.m_bIsFolder) //Only allow db content to be updated recursively
    return item.IsVideoDb();
  return item.GetVideoInfoTag()->m_playCount == 0;
}

bool CMarkWatched::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, true);
  return true;
}

bool CMarkUnWatched::IsVisible(const CFileItem& item) const
{
  if (!item.HasVideoInfoTag())
    return false;
  if (item.m_bIsFolder) //Only allow db content to be updated recursively
    return item.IsVideoDb();
  return item.GetVideoInfoTag()->m_playCount > 0;
}

bool CMarkUnWatched::Execute(const CFileItemPtr& item) const
{
  CVideoLibraryQueue::GetInstance().MarkAsWatched(item, false);
  return true;
}

std::string CResume::GetLabel(const CFileItem& item) const
{
  return CGUIWindowVideoBase::GetResumeString(item);
}

bool CResume::IsVisible(const CFileItem& item) const
{
  return CGUIWindowVideoBase::HasResumeItemOffset(&item);
}

static void SetPathAndPlay(CFileItem&& item)
{
  if (item.IsVideoDb())
  {
    item.SetProperty("original_listitem_url", item.GetPath());
    item.SetPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
  }
  g_application.PlayFile(item, "");
}

bool CResume::Execute(const CFileItemPtr& item) const
{
#ifdef HAS_DVD_DRIVE
  if (item->IsDVD() || item->IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item->GetPath(), true, false);
#endif

  CFileItem cpy(*item);
  cpy.m_lStartOffset = STARTOFFSET_RESUME;
  SetPathAndPlay(std::move(cpy));
  return true;
};

std::string CPlay::GetLabel(const CFileItem& item) const
{
  if (CGUIWindowVideoBase::HasResumeItemOffset(&item))
    return g_localizeStrings.Get(12023);
  return g_localizeStrings.Get(208);
}

bool CPlay::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder)
    return false; //! @todo implement
  return item.IsVideo() || item.IsDVD() || item.IsCDDA();
}

bool CPlay::Execute(const CFileItemPtr& item) const
{
#ifdef HAS_DVD_DRIVE
  if (item->IsDVD() || item->IsCDDA())
    return MEDIA_DETECT::CAutorun::PlayDisc(item->GetPath(), true, true);
#endif
  SetPathAndPlay(CFileItem(*item));
  return true;
};

}
