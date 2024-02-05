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
#include "media/MediaType.h"

#include <memory>

namespace CONTEXTMENU
{

class CVideoInfo : public CStaticContextMenuAction
{
public:
  explicit CVideoInfo(MediaType mediaType);
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;

private:
  const MediaType m_mediaType;
};

struct CTVShowInfo : CVideoInfo
{
  CTVShowInfo() : CVideoInfo(MediaTypeTvShow) {}
};

struct CSeasonInfo : CVideoInfo
{
  CSeasonInfo() : CVideoInfo(MediaTypeSeason) {}
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

struct CMovieSetInfo : CVideoInfo
{
  CMovieSetInfo() : CVideoInfo(MediaTypeVideoCollection) {}
};

struct CVideoRemoveResumePoint : CStaticContextMenuAction
{
  CVideoRemoveResumePoint() : CStaticContextMenuAction(38209) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CVideoMarkWatched : CStaticContextMenuAction
{
  CVideoMarkWatched() : CStaticContextMenuAction(16103) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CVideoMarkUnWatched : CStaticContextMenuAction
{
  CVideoMarkUnWatched() : CStaticContextMenuAction(16104) {}
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CVideoBrowse : CStaticContextMenuAction
{
  CVideoBrowse() : CStaticContextMenuAction(37015) {} // Browse into
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CVideoChooseVersion : CStaticContextMenuAction
{
  CVideoChooseVersion() : CStaticContextMenuAction(40221) {} // Choose version
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CVideoPlayVersionUsing : CStaticContextMenuAction
{
  CVideoPlayVersionUsing() : CStaticContextMenuAction(40209) {} // Play version using...
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& _item) const override;
};

struct CVideoResume : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& _item) const override;
};

struct CVideoPlay : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& _item) const override;
};

struct CVideoPlayUsing : CStaticContextMenuAction
{
  CVideoPlayUsing() : CStaticContextMenuAction(15213) {} // Play using...
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& _item) const override;
};

struct CVideoQueue : CStaticContextMenuAction
{
  CVideoQueue() : CStaticContextMenuAction(13347) {} // Queue item
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CVideoPlayNext : CStaticContextMenuAction
{
  CVideoPlayNext() : CStaticContextMenuAction(10008) {} // Play next
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

struct CVideoPlayAndQueue : IContextMenuItem
{
  std::string GetLabel(const CFileItem& item) const override;
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;
};

}
