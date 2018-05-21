#pragma once
/*
 *      Copyright (C) 2016-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "ContextMenuItem.h"
#include "guilib/GUIWindowManager.h"
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
