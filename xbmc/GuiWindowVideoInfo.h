#pragma once
#include "GUIDialog.h"
#include "utils/imdb.h"
#include "guilistitem.h"
#include "VideoDatabase.h"
#include "GUIWindowVideoBase.h"
#include "GUIWindowVideoFiles.h"

class CGUIWindowVideoInfo :
      public CGUIDialog
{
public:
  CGUIWindowVideoInfo(void);
  virtual ~CGUIWindowVideoInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
  void SetMovie(CIMDBMovie& movie);
  bool NeedRefresh() const;

protected:
  void Refresh();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  // link cast to movies
  void AddItemsToList(const vector<CStdString> &vecStr);
  void OnSearch(CStdString& strSearch);
  void DoSearch(CStdString& strSearch, CFileItemList& items);
  void OnSearchItemFound(const CFileItem* pItem);
  void Play();

  CIMDBMovie* m_pMovie;
  CIMDBMovie m_Movie;
  bool m_bViewReview;
  bool m_bRefresh;
  vector<CStdString> m_vecStrCast;
  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
};
