/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CGUIViewStateWindowVideo : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowVideo(const CFileItemList& items) : CGUIViewState(items) {}

protected:
  VECSOURCES& GetSources() override;
  std::string GetLockType() override;
  PLAYLIST::Id GetPlaylist() const override;
  std::string GetExtensions() override;
  bool AutoPlayNextItem() override;
};

class CGUIViewStateVideoPlaylist : public CGUIViewStateWindowVideo
{
public:
  explicit CGUIViewStateVideoPlaylist(const CFileItemList& items);

protected:
  void SaveViewState() override;
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
  bool AutoPlayNextItem() override { return false; }
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

