#pragma once
#include "guiwindow.h"

#include "stdstring.h"
#include <vector>
using namespace std;

class CGUIWindowMusicOverlay: 	public CGUIWindow
{
public:
	CGUIWindowMusicOverlay(void);
	virtual ~CGUIWindowMusicOverlay(void);
  virtual bool				OnMessage(CGUIMessage& message);
  virtual void				OnKey(const CKey& key);
	virtual void				Render();
	void								SetCurrentFile(const CStdString& strFile);
	IDirect3DTexture8* 	m_pTexture;
	int									m_iTextureWidth;
	int									m_iTextureHeight;
};
