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

class CVideoInfoBase : public CStaticContextMenuAction
{
public:
  explicit CVideoInfoBase(MediaType mediaType);
  bool IsVisible(const CFileItem& item) const override;
  bool Execute(const std::shared_ptr<CFileItem>& item) const override;

private:
  const MediaType m_mediaType;
};

struct CVideoInfo : CVideoInfoBase
{
  CVideoInfo() : CVideoInfoBase(MediaTypeVideo) {}
  bool IsVisible(const CFileItem& item) const override;
};

struct CTVShowInfo : CVideoInfoBase
{
  CTVShowInfo() : CVideoInfoBase(MediaTypeTvShow) {}
};

struct CSeasonInfo : CVideoInfoBase
{
  CSeasonInfo() : CVideoInfoBase(MediaTypeSeason) {}
};

struct CEpisodeInfo : CVideoInfoBase
{
  CEpisodeInfo() : CVideoInfoBase(MediaTypeEpisode) {}
};

struct CMusicVideoInfo : CVideoInfoBase
{
  CMusicVideoInfo() : CVideoInfoBase(MediaTypeMusicVideo) {}
};

struct CMovieInfo : CVideoInfoBase
{
  CMovieInfo() : CVideoInfoBase(MediaTypeMovie) {}
};

struct CMovieSetInfo : CVideoInfoBase
{
  CMovieSetInfo() : CVideoInfoBase(MediaTypeVideoCollection) {}
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

struct CVideoPlayStackPart : CStaticContextMenuAction
{
  CVideoPlayStackPart() : CStaticContextMenuAction(20324) {} // Play part
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
