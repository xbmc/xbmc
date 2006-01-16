#pragma once

typedef enum {
  VIEW_METHOD_NONE=-1,
  VIEW_METHOD_LIST,
  VIEW_METHOD_ICONS,
  VIEW_METHOD_LARGE_ICONS,
  VIEW_METHOD_LARGE_LIST,
  VIEW_METHOD_MAX
} VIEW_METHOD;

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


  VIEW_METHOD SetNextViewAsControl();
  VIEW_METHOD GetViewAsControl() const;
  int GetViewAsControlButtonLabel() const;

  SORT_METHOD SetNextSortMethod();
  SORT_METHOD GetSortMethod() const;
  int GetSortMethodLabel() const;
  void GetSortMethodLabelMasks(LABEL_MASKS& masks) const;

  SORT_ORDER SetNextSortOrder();
  SORT_ORDER GetSortOrder() const { return m_sortOrder; }
  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual int GetPlaylist();
  const CStdString& GetPlaylistDirectory();
  void SetPlaylistDirectory(const CStdString& strDirectory);
  virtual bool HandleArchives();
  virtual bool AutoPlayNextItem();
  virtual CStdString GetLockType();

protected:
  CGUIViewState(const CFileItemList& items);  // no direct object creation, use GetViewState()
  virtual void SaveViewState()=0;

  void AddViewAsControl(VIEW_METHOD viewAsControl, int buttonLabel);
  void SetViewAsControl(VIEW_METHOD viewAsControl);
  void AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, LABEL_MASKS labelmasks);
  void SetSortMethod(SORT_METHOD sortMethod);
  void SetSortOrder(SORT_ORDER sortOrder) { m_sortOrder=sortOrder; }
  const CFileItemList& m_items;
  bool m_hideParentDirItems;

private:
  typedef struct _VIEW
  {
    VIEW_METHOD m_viewAsControl;
    int m_buttonLabel;
  } VIEW;
  vector<VIEW> m_viewAsControls;
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
