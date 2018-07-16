/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
  CGUIDialogMusicInfo::ShowFor(item.get());
  return true;
}

}
