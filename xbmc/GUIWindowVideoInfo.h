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
  void SetMovie(CVideoInfoTag& movie, const CFileItem *item);
  bool NeedRefresh() const;
  const CStdString &GetThumbnail() const { return m_movieItem.GetThumbnailImage(); };

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
  bool DownloadThumbnail(const CStdString &thumb);
  void OnGetThumb();
  CVideoInfoTag m_Movie;
  CFileItem m_movieItem;
  bool m_bViewReview;
  bool m_bRefresh;
  vector<CStdString> m_vecStrCast;
  CGUIDialogProgress* m_dlgProgress;
  CVideoDatabase m_database;
};
