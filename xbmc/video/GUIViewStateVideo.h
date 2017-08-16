#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "view/GUIViewState.h"

class CGUIViewStateWindowVideo : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowVideo(const CFileItemList& items) : CGUIViewState(items) {}

protected:
  VECSOURCES& GetSources() override;
  std::string GetLockType() override;
  int GetPlaylist() override;
  std::string GetExtensions() override;
};

class CGUIViewStateWindowVideoNav : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateWindowVideoNav(const CFileItemList& items);
  bool AutoPlayNextItem() override;

protected:
  void SaveViewState() override;
  VECSOURCES& GetSources() override;
};

class CGUIViewStateWindowVideoPlaylist : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateWindowVideoPlaylist(const CFileItemList& items);

protected:
  void SaveViewState() override;
  bool HideExtensions() override;
  bool HideParentDirItems() override;
  VECSOURCES& GetSources() override;
};

class CGUIViewStateVideoMovies : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoMovies(const CFileItemList& items);
protected:
  void SaveViewState() override;
};

class CGUIViewStateVideoMusicVideos : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoMusicVideos(const CFileItemList& items);
protected:
  void SaveViewState() override;
};

class CGUIViewStateVideoTVShows : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoTVShows(const CFileItemList& items);
protected:
  void SaveViewState() override;
};

class CGUIViewStateVideoEpisodes : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoEpisodes(const CFileItemList& items);
protected:
  void SaveViewState() override;
};

