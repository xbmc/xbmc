#pragma once
#include "GUIViewState.h"

class CGUIViewStateWindowPictures : public CGUIViewState
{
public:
  CGUIViewStateWindowPictures(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual CStdString GetLockType();
  virtual bool UnrollArchives();
  virtual CStdString GetExtensions();
  virtual VECSHARES& GetShares();
};

class CGUIViewStateWindowPrograms : public CGUIViewState
{
public:
  CGUIViewStateWindowPrograms(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual CStdString GetLockType();
  virtual CStdString GetExtensions();
  virtual VECSHARES& GetShares();
};

class CGUIViewStateWindowScripts : public CGUIViewState
{
public:
  CGUIViewStateWindowScripts(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual CStdString GetExtensions();
  virtual VECSHARES& GetShares();
};
class CGUIViewStateWindowGameSaves : public CGUIViewState
{
public:
  CGUIViewStateWindowGameSaves(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  //virtual CStdString GetExtensions();
  virtual VECSHARES& GetShares();
};
