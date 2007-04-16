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
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);
  bool DownloadThumbnail(const CStdString &thumbFile);
  void OnGetThumb();

  CAlbum m_album;
  VECSONGS m_songs;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_hasUpdatedThumb;
  CFileItem m_albumItem;
};
