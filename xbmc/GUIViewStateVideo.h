#pragma once
#include "GUIViewState.h"


class CGUIViewStateWindowVideo : public CGUIViewState
{
public:
  CGUIViewStateWindowVideo(const CFileItemList& items);

protected:
  virtual CStdString GetLockType();
  virtual bool HandleArchives();
};

class CGUIViewStateWindowVideoFiles : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoFiles(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoGenre : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoGenre(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoTitle : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoTitle(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoYear : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoYear(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoActor : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoActor(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoPlaylist : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual int GetPlaylist();
};
