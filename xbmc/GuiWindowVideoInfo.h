#pragma once
#include "GUIDialog.h"
#include "guiwindowmanager.h"
#include "utils/imdb.h"

#include "stdstring.h"

class CGUIWindowVideoInfo :
  public CGUIDialog
{
public:
  CGUIWindowVideoInfo(void);
  virtual ~CGUIWindowVideoInfo(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();
	void						SetMovie(CIMDBMovie& movie);
  bool            NeedRefresh() const;

protected:
	void										Refresh();
	void										Update();
	void										SetLabel(int iControl, const CStdString& strLabel);
	IDirect3DTexture8* 			m_pTexture;
	CIMDBMovie*							m_pMovie;
	int											m_iTextureWidth;
	int											m_iTextureHeight;
	bool										m_bViewReview;
  bool                    m_bRefresh;
};
