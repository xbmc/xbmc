/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "view/GUIViewState.h"

class CGUIViewStateWindowMusic : public CGUIViewState
{
public:
  explicit CGUIViewStateWindowMusic(const CFileItemList& items) : CGUIViewState(items) {}
protected:
  VECSOURCES& GetSources() override;
  PLAYLIST::Id GetPlaylist() const override;
  bool AutoPlayNextItem() override;
  std::string GetLockType() override;
  std::string GetExtensions() override;
};

class CGUIViewStateMusicSearch : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicSearch(const CFileItemList& items);

protected:
  void SaveViewState() override;
};

class CGUIViewStateMusicDatabase : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicDatabase(const CFileItemList& items);

protected:
  void SaveViewState() override;
};

class CGUIViewStateMusicSmartPlaylist : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicSmartPlaylist(const CFileItemList& items);

protected:
  void SaveViewState() override;
};

class CGUIViewStateMusicPlaylist : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateMusicPlaylist(const CFileItemList& items);

protected:
  void SaveViewState() override;
};

class CGUIViewStateWindowMusicNav : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateWindowMusicNav(const CFileItemList& items);

protected:
  void SaveViewState() override;
  VECSOURCES& GetSources() override;

private:
  void AddOnlineShares();
};

class CGUIViewStateWindowMusicPlaylist : public CGUIViewStateWindowMusic
{
public:
  explicit CGUIViewStateWindowMusicPlaylist(const CFileItemList& items);

protected:
  void SaveViewState() override;
  PLAYLIST::Id GetPlaylist() const override;
  bool AutoPlayNextItem() override;
  bool HideParentDirItems() override;
  VECSOURCES& GetSources() override;
};
