#pragma once
#include "GUIDialog.h"
#include "utils/MusicInfoScraper.h"
using namespace MUSIC_GRABBER;

class CGUIWindowMusicInfo :
      public CGUIDialog
{
public:
  CGUIWindowMusicInfo(void);
  virtual ~CGUIWindowMusicInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void Render();
  void SetAlbum(CMusicAlbumInfo& album, const CStdString &path);
  bool NeedRefresh() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };
  void RefreshThumb();
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);
  bool DownloadThumbnail(const CStdString &thumbFile);
  void OnGetThumb();

  CMusicAlbumInfo m_album;
  bool m_bViewReview;
  bool m_bRefresh;
  bool m_hasUpdatedThumb;
  CFileItem m_albumItem;
};
