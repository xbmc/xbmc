#pragma once

#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "graphiccontext.h"
#include "key.h"

#include <vector>
#include "stdstring.h"
using namespace std;

#define SCREENSAVER_FADE   1
#define SCREENSAVER_BLACK  2
#define SCREENSAVER_MATRIX 3

class CGUIWindowScreensaver : public CGUIWindow
{
public:
	CGUIWindowScreensaver(void);
	virtual ~CGUIWindowScreensaver(void);
	
	bool			IsPlaying() const;
	virtual bool	OnMessage(CGUIMessage& message);
	virtual void	OnAction(const CAction &action);
	virtual void	OnMouse();
	virtual void	Render();
	virtual void	SetColor( DWORD dwColor ) { m_dwColor = dwColor; };
	DWORD			GetColor( void ) { return m_dwColor; };

private:
	void TimerTick();
	void RenderFrame();
	void InitMatrix();

	HRESULT CGUIWindowScreensaver::GetBackBufferTexture(IDirect3DDevice8* pDevice, LPDIRECT3DTEXTURE8 *ppTexture);
	HRESULT CGUIWindowScreensaver::RenderQuad( LPDIRECT3DTEXTURE8 pTexture, int iScreenWidth, int iScreenHeight, FLOAT fAlpha, FLOAT fDepth );

	bool m_bSaverActive;

	LPDIRECT3DTEXTURE8	m_pTexture;
	bool			m_bDoEffect;
	char			m_mtrxGrid[120][67];
	int				m_iMatrixPos[120];
	int				m_iMatrixSpeed[120];
	DWORD			m_dwFrameCount;
	bool			m_bInit;
	CStdString		m_sMessage1;
	CStdString		m_sMessage2;
	DWORD			m_dwColor;
	bool			m_bUseBack;
	int				m_iColMax;
	int				m_iRowMax;
};
