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
  void SetMovie(CIMDBMovie& movie, const CStdString &thumbNail);
  bool NeedRefresh() const;
  const CStdString &GetThumbnail() const { return m_thumbNail; };

protected:
  virtual void OnInitWindow();
  void Refresh();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  // link cast to movies
  void AddItemsToList(const vector<CStdString> &vecStr);
  void OnSearch(CStdString& strSearch);
  void DoSearch(CStdString& strSearch, CFileItemList& items);
  void OnSearchItemFound(const CFileItem* pItem);
  void Play(bool resume = false);
  void DownloadThumbnail(const CStdString &thumb);

  CIMDBMovie* m_pMovie;
  CStdString m_thumbNail;
  CIMDBMovie m_Movie;
  bool m_bViewReview;
  bool m_bRefresh;
  vector<CStdString> m_vecStrCast;
  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
};
