#pragma once
#include "GUIViewState.h"


class CGUIViewStateWindowVideo : public CGUIViewState
{
public:
  CGUIViewStateWindowVideo(const CFileItemList& items) : CGUIViewState(items) {}

protected:
  virtual CStdString GetLockType();
  virtual bool UnrollArchives();
  virtual CStdString GetExtensions();
};

class CGUIViewStateWindowVideoFiles : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoFiles(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual VECSHARES& GetShares();
};

class CGUIViewStateWindowVideoNav : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoNav(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual VECSHARES& GetShares();
};

class CGUIViewStateWindowVideoPlaylist : public CGUIViewStateWindowVideo
{
public:
  CGUIViewStateWindowVideoPlaylist(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual int GetPlaylist();
  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual VECSHARES& GetShares();
};
