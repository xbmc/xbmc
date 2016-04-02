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
#include "guilib/GUIWindowManager.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "tags/MusicInfoTag.h"


namespace CONTEXTMENU
{

CMusicInfo::CMusicInfo(MediaType mediaType)
      : CStaticContextMenuAction(19033), m_mediaType(mediaType) {}

bool CMusicInfo::IsVisible(const CFileItem& item) const
{
  return item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == m_mediaType;
}

bool CMusicInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogMusicInfo::ShowFor(*item);
  return true;
}

}
