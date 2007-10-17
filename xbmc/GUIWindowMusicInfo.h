#pragma once
#include "GUIDialog.h"

class CGUIWindowMusicInfo :
      public CGUIDialog
{
public:
  CGUIWindowMusicInfo(void);
  virtual ~CGUIWindowMusicInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
  void SetAlbum(const CAlbum& album, const VECSONGS &songs, const CStdString &path);
  bool NeedRefresh() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };
  void RefreshThumb();

  virtual bool HasListItems() const { return true; };
  virtual CFileItem *GetCurrentListItem(int offset = 0) { return &m_albumItem; };
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);
  bool DownloadThumbnail(const CStdString &thumbFile);
  void OnGetThumb();
  void SetSongs(const VECSONGS &songs);

  CAlbum m_album;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_hasUpdatedThumb;
  CFileItem     m_albumItem;
  CFileItemList m_albumSongs;
};
