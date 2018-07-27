/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"


namespace CONTEXTMENU
{

struct CMusicInfo : CStaticContextMenuAction
{
  explicit CMusicInfo(MediaType mediaType);
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
private:
  const MediaType m_mediaType;
};

struct CAlbumInfo : CMusicInfo
{
  CAlbumInfo() : CMusicInfo(MediaTypeAlbum) {}
};

struct CArtistInfo : CMusicInfo
{
  CArtistInfo() : CMusicInfo(MediaTypeArtist) {}
};

struct CSongInfo : CMusicInfo
{
  CSongInfo() : CMusicInfo(MediaTypeSong) {}
};

}
