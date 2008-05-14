#pragma once
#include "GUIViewState.h"


class CGUIViewStateWindowVideo : public CGUIViewState
{
public:
  CGUIViewStateWindowVideo(const CFileItemList& items) : CGUIViewState(items) {}

protected:
  virtual CStdString GetLockType();
  virtual bool UnrollArchives();
  virtual int GetPlaylist();
  virtual CStdString GetExtensions();
};

class CGUIViewStateWindowVideoFiles : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoFiles(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual VECSOURCES& GetSources();
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