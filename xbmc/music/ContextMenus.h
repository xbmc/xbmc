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

struct CMusicInfoBase : CStaticContextMenuAction
{
  explicit CMusicInfoBase(KODI::MEDIA::TYPE mediaType);
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;

private:
  const KODI::MEDIA::TYPE m_mediaType;
};

struct CMusicInfo : CMusicInfoBase
{
  CMusicInfo() : CMusicInfoBase(KODI::MEDIA::TYPE::MUSIC) {}
  bool IsVisible(const CFileItem& item) const override;
};

struct CAlbumInfo : CMusicInfoBase
{
  CAlbumInfo() : CMusicInfoBase(KODI::MEDIA::TYPE::ALBUM) {}
};

struct CArtistInfo : CMusicInfoBase
{
  CArtistInfo() : CMusicInfoBase(KODI::MEDIA::TYPE::ARTIST) {}
};

struct CSongInfo : CMusicInfoBase
{
  CSongInfo() : CMusicInfoBase(KODI::MEDIA::TYPE::SONG) {}
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
