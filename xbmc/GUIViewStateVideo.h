#pragma once
#include "GUIViewState.h"


class CGUIViewStateWindowVideoFiles : public CGUIViewState
{
public:
  CGUIViewStateWindowVideoFiles(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoGenre : public CGUIViewState
{
public:
  CGUIViewStateWindowVideoGenre(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoTitle : public CGUIViewState
{
public:
  CGUIViewStateWindowVideoTitle(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoYear : public CGUIViewState
{
public:
  CGUIViewStateWindowVideoYear(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoActor : public CGUIViewState
{
public:
  CGUIViewStateWindowVideoActor(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateWindowVideoPlaylist : public CGUIViewState
{
public:
  CGUIViewStateWindowVideoPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};
