#pragma once
#include "GUIDialog.h"
#include "guiwindowmanager.h"
#include "utils/MusicInfoScraper.h"
using namespace MUSIC_GRABBER;

#include "stdstring.h"

class CGUIWindowMusicInfo :
  public CGUIDialog
{
public:
  CGUIWindowMusicInfo(void);
  virtual ~CGUIWindowMusicInfo(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();
	void						SetAlbum(CMusicAlbumInfo& album);
  bool            NeedRefresh() const;
protected:
	void										Refresh();
	void										Update();
	void										SetLabel(int iControl, const CStdString& strLabel);
	CMusicAlbumInfo*				m_pAlbum;
	bool										m_bViewReview;
  bool                    m_bRefresh;
};
