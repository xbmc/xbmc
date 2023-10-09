/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"
#include "media/MediaType.h"

#include <memory>

class CFileItem;

namespace CONTEXTMENU
{

struct CMusicInfo : CStaticContextMenuAction
{
  explicit CMusicInfo(MediaType mediaType);
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;

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

struct CMusicBrowse : CStaticContextMenuAction
{
  CMusicBrowse() : CStaticContextMenuAction(37015) {} // Browse into
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CMusicPlay : CStaticContextMenuAction
{
  CMusicPlay() : CStaticContextMenuAction(208) {} // Play
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CMusicPlayUsing : CStaticContextMenuAction
{
  CMusicPlayUsing() : CStaticContextMenuAction(15213) {} // Play using...
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& _item) const override;
};

struct CMusicPlayNext : CStaticContextMenuAction
{
  CMusicPlayNext() : CStaticContextMenuAction(10008) {} // Play next
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CMusicQueue : CStaticContextMenuAction
{
  CMusicQueue() : CStaticContextMenuAction(13347) {} // Queue item
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

} // namespace CONTEXTMENU
