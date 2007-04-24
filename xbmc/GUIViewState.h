#pragma once

#include "settings.h"
#include "GUIBaseContainer.h"

class CViewState; // forward

class CGUIViewState
{
public:
  virtual ~CGUIViewState();
  static CGUIViewState* GetViewState(int windowId, const CFileItemList& items);

  typedef struct _LABEL_MASKS
  {
    _LABEL_MASKS(const CStdString& strLabelFile="", const CStdString& strLabel2File="", const CStdString& strLabelFolder="", const CStdString& strLabel2Folder="")
    {
      m_strLabelFile=strLabelFile;
      m_strLabel2File=strLabel2File;
      m_strLabelFolder=strLabelFolder;
      m_strLabel2Folder=strLabel2Folder;
    }
    CStdString m_strLabelFile;
    CStdString m_strLabel2File;
    CStdString m_strLabelFolder;
    CStdString m_strLabel2Folder;
  } LABEL_MASKS;

  void SetViewAsControl(int viewAsControl);
  void SaveViewAsControl(int viewAsControl);
  int GetViewAsControl() const;

  SORT_METHOD SetNextSortMethod();
  SORT_METHOD GetSortMethod() const;
  int GetSortMethodLabel() const;
  void GetSortMethodLabelMasks(LABEL_MASKS& masks) const;
  void GetSortMethods(vector< pair<int,int> > &sortMethods) const;

  SORT_ORDER SetNextSortOrder();
  SORT_ORDER GetSortOrder() const { return m_sortOrder; };
  SORT_ORDER GetDisplaySortOrder() const;
  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual bool DisableAddSourceButtons();
  virtual int GetPlaylist();
  const CStdString& GetPlaylistDirectory();
  void SetPlaylistDirectory(const CStdString& strDirectory);
  bool IsCurrentPlaylistDirectory(const CStdString& strDirectory);
  virtual bool UnrollArchives();
  virtual bool AutoPlayNextItem();
  virtual CStdString GetLockType();
  virtual CStdString GetExtensions();
  virtual VECSHARES& GetShares();

protected:
  CGUIViewState(const CFileItemList& items);  // no direct object creation, use GetViewState()
  virtual void SaveViewState()=0;
  void SaveViewToDb(const CStdString &path, int windowID);
  void SaveViewToDb(const CStdString &path, int windowID, CViewState &viewState, bool saveSettings = true);
  void LoadViewState(const CStdString &path, int windowID);

  void AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, LABEL_MASKS labelmasks);
  void SetSortMethod(SORT_METHOD sortMethod);
  void SetSortOrder(SORT_ORDER sortOrder) { m_sortOrder=sortOrder; }
  const CFileItemList& m_items;

  static VECSHARES m_shares;

private:
  int m_currentViewAsControl;

  typedef struct _SORT
  {
    SORT_METHOD m_sortMethod;
    int m_buttonLabel;
    LABEL_MASKS m_labelMasks;
  } SORT;
  vector<SORT> m_sortMethods;
  int m_currentSortMethod;

  SORT_ORDER m_sortOrder;

  static CStdString m_strPlaylistDirectory;
};

class CGUIViewStateGeneral : public CGUIViewState
{
public:
  CGUIViewStateGeneral(const CFileItemList& items);

protected:
  virtual void SaveViewState() {};
};
