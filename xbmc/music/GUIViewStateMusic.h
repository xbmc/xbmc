#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIViewState.h"

class CGUIViewStateWindowMusic : public CGUIViewState
{
public:
  CGUIViewStateWindowMusic(const CFileItemList& items) : CGUIViewState(items) {}
protected:
  virtual VECSOURCES& GetSources();
  virtual int GetPlaylist();
  virtual bool AutoPlayNextItem();
  virtual CStdString GetLockType();
  virtual CStdString GetExtensions();
};

class CGUIViewStateMusicSearch : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicSearch(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateMusicDatabase : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicDatabase(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateMusicSmartPlaylist : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicSmartPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateMusicPlaylist : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowMusicNav : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateWindowMusicNav(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual VECSOURCES& GetSources();

private:
  void AddOnlineShares();
};

class CGUIViewStateWindowMusicSongs : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateWindowMusicSongs(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual VECSOURCES& GetSources();
};

class CGUIViewStateWindowMusicPlaylist : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateWindowMusicPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual int GetPlaylist();
  virtual bool AutoPlayNextItem();
  virtual bool HideParentDirItems();
  virtual VECSOURCES& GetSources();
};

class CGUIViewStateMusicLastFM : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicLastFM(const CFileItemList& items);

protected:
  virtual bool AutoPlayNextItem();
  virtual void SaveViewState();
};
