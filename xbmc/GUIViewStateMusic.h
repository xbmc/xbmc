#pragma once
#include "GUIViewState.h"


class CGUIViewStateWindowMusic : public CGUIViewState
{
public:
  CGUIViewStateWindowMusic(const CFileItemList& items) : CGUIViewState(items) {}
protected:
  virtual bool HideParentDirItems();
};

class CGUIViewStateMusicDatabase : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicDatabase(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowMusicNav : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateWindowMusicNav(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowMusicSongs : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateWindowMusicSongs(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowMusicPlaylist : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateWindowMusicPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};
