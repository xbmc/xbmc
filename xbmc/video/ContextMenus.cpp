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
  auto window = static_cast<CGUIWindowVideoNav*>(g_windowManager.GetWindow(WINDOW_VIDEO_NAV));
  if (window)
  {
    ADDON::ScraperPtr info;
    window->OnItemInfo(item.get(), info);
    return true;
  }
  return false;
}

}
