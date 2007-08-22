#pragma once
#include "GUIViewState.h"


class CGUIViewStateWindowMusic : public CGUIViewState
{
public:
  CGUIViewStateWindowMusic(const CFileItemList& items) : CGUIViewState(items) {}
protected:
  virtual int GetPlaylist();
  virtual bool UnrollArchives();
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
  virtual VECSHARES& GetShares();

private:
  void AddOnlineShares();
};

class CGUIViewStateWindowMusicSongs : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateWindowMusicSongs(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual VECSHARES& GetShares();
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
  virtual VECSHARES& GetShares();
};

class CGUIViewStateMusicShoutcast : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicShoutcast(const CFileItemList& items);

protected:
  virtual bool AutoPlayNextItem();
  virtual void SaveViewState();
};


class CGUIViewStateMusicLastFM : public CGUIViewStateWindowMusic
{
public:
  CGUIViewStateMusicLastFM(const CFileItemList& items);

protected:
  virtual bool AutoPlayNextItem();
  virtual void SaveViewState();
};
