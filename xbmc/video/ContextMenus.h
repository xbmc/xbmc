/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContextMenuItem.h"
#include "VideoLibraryQueue.h"

namespace CONTEXTMENU
{

class CVideoInfo : public CStaticContextMenuAction
{
public:
  explicit CVideoInfo(MediaType mediaType);
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
private:
  const MediaType m_mediaType;
};

struct CTVShowInfo : CVideoInfo
{
  CTVShowInfo() : CVideoInfo(MediaTypeTvShow) {}
};

struct CEpisodeInfo : CVideoInfo
{
  CEpisodeInfo() : CVideoInfo(MediaTypeEpisode) {}
};

struct CMusicVideoInfo : CVideoInfo
{
  CMusicVideoInfo() : CVideoInfo(MediaTypeMusicVideo) {}
};

struct CMovieInfo : CVideoInfo
{
  CMovieInfo() : CVideoInfo(MediaTypeMovie) {}
};

struct CRemoveResumePoint : CStaticContextMenuAction
{
  CRemoveResumePoint() : CStaticContextMenuAction(38209) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
};

struct CMarkWatched : CStaticContextMenuAction
{
  CMarkWatched() : CStaticContextMenuAction(16103) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
};

struct CMarkUnWatched : CStaticContextMenuAction
{
  CMarkUnWatched() : CStaticContextMenuAction(16104) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& item) const override;
};

struct CResume : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& _item) const override;
};

struct CPlay : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const CFileItemPtr& _item) const override;
};
}
