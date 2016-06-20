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
  CGUIViewStateWindowVideo(const CFileItemList& items) : CGUIViewState(items) {}

protected:
  virtual VECSOURCES& GetSources();
  virtual std::string GetLockType();
  virtual int GetPlaylist();
  virtual std::string GetExtensions();
};

class CGUIViewStateWindowVideoNav : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoNav(const CFileItemList& items);
  virtual bool AutoPlayNextItem();

protected:
  virtual void SaveViewState();
  virtual VECSOURCES& GetSources();
};

class CGUIViewStateWindowVideoPlaylist : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual VECSOURCES& GetSources();
};

class CGUIViewStateVideoMovies : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateVideoMovies(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

class CGUIViewStateVideoMusicVideos : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateVideoMusicVideos(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

class CGUIViewStateVideoTVShows : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateVideoTVShows(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

class CGUIViewStateVideoEpisodes : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateVideoEpisodes(const CFileItemList& items);
protected:
  virtual void SaveViewState();
};

