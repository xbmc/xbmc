#include "localizestrings.h"
#include "GUIWindowScreensaver.h"
#include "application.h"
#include "settings.h"
#include "guiFontManager.h"
#include "util.h"
#include "sectionloader.h"

/*
	Original source code for the Matrix Screen Saver from Media X Menu by BenJeremy

	Tweaks/changes to make it work with XBMC by forza @ XBMC

	Matrix Symbol Font by Lexandr (mCode 1.5 - http://www.deviantart.com/deviation/2040700/)
*/

CGUIFont*		m_pFont;	// This is here as it doesn't like being in GUIWindowScreensaver.h

CGUIWindowScreensaver::CGUIWindowScreensaver(void)
:CGUIWindow(0)
{
	srand( timeGetTime() );
	m_bUseBack = true;
	m_dwColor = 0xFF00FF00;
	m_bInit = false;
	m_bDoEffect = true;
}

CGUIWindowScreensaver::~CGUIWindowScreensaver(void)
{
}

bool CGUIWindowScreensaver::IsPlaying() const
{
	return m_bSaverActive;
}

void CGUIWindowScreensaver::Render()
{
	RenderFrame();
}

void CGUIWindowScreensaver::OnAction(const CAction &action)
{
	// We're just a screen saver, nothing to do here ...	
}

void CGUIWindowScreensaver::InitMatrix()
{
	int iX, iY;
	char cLast;

	LPDIRECT3DDEVICE8 pDevice = g_graphicsContext.Get3DDevice();

	m_bDoEffect = true;
	m_dwFrameCount=0;
	
	for( iX = 0; iX<80; iX++ )
	{
		cLast = 0;
		for( iY = 0; iY<60; iY++ )
		{
			do
			{
				m_mtrxGrid[iX][iY] = 'A'+(char)(rand() % 25);
			} while(m_mtrxGrid[iX][iY] == cLast);	// prevent repeats
			cLast = m_mtrxGrid[iX][iY];
		}
		// Pos will be (0-120)/4
		// Speed will depend on how many "pos" it will skip on the way through
		m_iMatrixPos[iX] = 0-((rand()+iX)%(g_graphicsContext.GetHeight()/2));
		m_iMatrixSpeed[iX] = 1+((rand()+iX)%4);
	}

	m_pFont=g_fontManager.GetFont("matrix8");
	if (!m_pFont) m_pFont=g_fontManager.GetFont("font13");	// matrix font not found, fall back on standard font

	// Grab copy of screen...
	if (m_pTexture) m_pTexture->Release();
	GetBackBufferTexture( pDevice, &m_pTexture );
	
	m_bInit = true;
	return;
}

void CGUIWindowScreensaver::RenderFrame()
{
	char chText[2];
	int iPos, iCol, iX, iY, iColMax, iRowMax;
	DWORD dwGreen;
	DWORD dwLeadColor;
	CStdStringW wszText;
	
	LPDIRECT3DDEVICE8 pDevice = g_graphicsContext.Get3DDevice();
	bool bFade = false;

	if ( !m_bInit )	InitMatrix();
	if ( !m_pFont ) return;			// couldn't even find the standard font ...

	iPos = (int)(m_dwFrameCount%32)*8;

	if ( iPos >= 128 )
	{
		dwGreen = iPos;
	}
	else
	{
		dwGreen = 255-iPos;
	}

	iColMax = int (g_graphicsContext.GetWidth() / 16);
	iRowMax = int (g_graphicsContext.GetHeight() / 16);
	if (iColMax > 80) iColMax = 80;
	if (iRowMax > 60) iRowMax = 60;

	dwGreen |= dwGreen<<8;
	dwGreen |= dwGreen<<8;
	dwGreen |= 0xff000000;
	dwGreen &= m_dwColor;
	dwLeadColor = 0xDDDDDDDD;	// Original source used bright white, we'll go a bit darker

	// Render the PREVIOUS screen, which displays fading symbols
	// as new symbol is drawn below it. The essence of the Matrix Effect	
	
	if (m_pTexture) RenderQuad(m_pTexture, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), (FLOAT)0.94, 1);		// fade was 0.90	
	chText[1] = 0;

	if ( m_bDoEffect )
	{
		// Render falling symbols, one per column...
		for( iCol = 0; iCol<iColMax; iCol++ )
		{
			m_iMatrixPos[iCol] += m_iMatrixSpeed[iCol];

			// Reset only if we are not fading out...
			if ( (m_iMatrixPos[iCol] > (g_graphicsContext.GetHeight()/2)) && (!bFade) )
			{
				m_iMatrixPos[iCol] = 0-(rand()%(g_graphicsContext.GetHeight()/2));
			}

			// Get our grid Y coord
			iPos = m_iMatrixPos[iCol]/8;

			// Draw character only if the pos is visible
			if ( (iPos >= 0) && (iPos<iRowMax) )
			{
				iX = iCol*16;
				iY = iPos*16;
				chText[0] = m_mtrxGrid[iCol][iPos];
				wszText.Format(L"%c", chText[0]);
				m_pFont->DrawText( (FLOAT)iX, (FLOAT)iY, m_dwColor, wszText );
				if ( iPos<iRowMax )
				{
					iX = iCol*16;
					iY = (iPos+1)*16;
					chText[0] = m_mtrxGrid[iCol][iPos+1];
					wszText.Format(L"%c", chText[0]);
					m_pFont->DrawText( (FLOAT)iX, (FLOAT)iY, dwLeadColor, wszText );
				}
			}
		}
	}

	// Grab a copy of screen...
	if (m_pTexture) m_pTexture->Release();
	GetBackBufferTexture( pDevice, &m_pTexture );
	m_dwFrameCount++;
}

bool CGUIWindowScreensaver::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_WINDOW_DEINIT:
		{
			if ( m_bInit )
			{
				if (m_pTexture) m_pTexture->Release();
				m_pTexture = NULL;
				m_bInit = false;
			}
			g_application.EnableOverlay();
			m_bSaverActive = false;
		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			g_application.DisableOverlay();
			InitMatrix();
			m_bSaverActive = true;
			CGUIWindow::OnMessage(message);
			return true;
		}
	}
	return CGUIWindow::OnMessage(message);
}

HRESULT CGUIWindowScreensaver::GetBackBufferTexture(IDirect3DDevice8* pDevice, LPDIRECT3DTEXTURE8 *ppTexture )
{
	HRESULT hr = E_FAIL;
	D3DSURFACE_DESC descSurface;
	bool bReleaseSurface = false;

    // Create a texture surface for the persisted surface
    LPDIRECT3DSURFACE8 pPersistedSurface = NULL;
    LPDIRECT3DSURFACE8 pPersistedSurfaceTgt = NULL;
	
	// We don't have a persisted surface, can we get the currently used
	// one from the D3D device?
	hr = pDevice->GetPersistedSurface( &pPersistedSurface );
	if ( SUCCEEDED( hr ) && pPersistedSurface )
	{
		pPersistedSurface->AddRef();
	}
	else
	{
		if ( !SUCCEEDED(hr = pDevice->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pPersistedSurface)))
		{
			pPersistedSurface = NULL;
		}
	}
	if ( pPersistedSurface )
	{
		const POINT ptSrc = { 0, 0 };
		const RECT rc = { 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight() };
		const POINT ptDest = { 0, 0 };
		pPersistedSurface->GetDesc( &descSurface );
		if ( SUCCEEDED(hr = pDevice->CreateTexture( rc.right, rc.bottom, 1, 0, descSurface.Format, 0, ppTexture )))
		{
			if ( SUCCEEDED(hr = (*ppTexture)->GetSurfaceLevel( 0, &pPersistedSurfaceTgt )))
			{
				try
				{
					if ( SUCCEEDED(hr = pDevice->CopyRects( pPersistedSurface, &rc, 1, 
										pPersistedSurfaceTgt, &ptDest )))
					{
					}
				}
				catch( ... )
				{
				}
			}
			pPersistedSurfaceTgt->GetDesc( &descSurface );
			pPersistedSurfaceTgt->Release();
		}
		pPersistedSurface->Release();
	}
	return hr;
}

HRESULT CGUIWindowScreensaver::RenderQuad( LPDIRECT3DTEXTURE8 pTexture, int iScreenWidth, int iScreenHeight, FLOAT fAlpha, FLOAT fDepth )
{
    // Set up the vertices (notice the pixel centers are shifted by -0.5f to
    // line them up with the texel centers). The texture coordinates assume
    // a linear texture will be used.
    struct VERTEX { D3DXVECTOR4 p; FLOAT tu, tv; };
    VERTEX v[4];
    v[0].p = D3DXVECTOR4(   0 - 0.5f,   0 - 0.5f, fDepth, 0.0f ); v[0].tu =   0; v[0].tv =   0;
    v[1].p = D3DXVECTOR4( (FLOAT)iScreenWidth - 0.5f,   0 - 0.5f, fDepth, 0.0f ); v[1].tu = (FLOAT)iScreenWidth; v[1].tv =   0;
    v[2].p = D3DXVECTOR4( (FLOAT)iScreenWidth - 0.5f, (FLOAT)iScreenHeight - 0.5f, fDepth, 0.0f ); v[2].tu = (FLOAT)iScreenWidth; v[2].tv = (FLOAT)iScreenHeight;
    v[3].p = D3DXVECTOR4(   0 - 0.5f, (FLOAT)iScreenHeight - 0.5f, fDepth, 0.0f ); v[3].tu =   0; v[3].tv = (FLOAT)iScreenHeight;


    // Set state to render the image
	LPDIRECT3DDEVICE8 pDevice = g_graphicsContext.Get3DDevice();
    pDevice->SetTexture( 0, pTexture );
    pDevice->SetPixelShader( NULL );
    pDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
    pDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    pDevice->SetRenderState( D3DRS_TEXTUREFACTOR, (DWORD)(255.0f*fAlpha)<<24L );
    pDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    pDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    pDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    pDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    pDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );

    // Render the quad
    pDevice->SetVertexShader( D3DFVF_XYZRHW|D3DFVF_TEX1 );
    pDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, v, sizeof(v[0]) );

    pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_WRAP );
    pDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_WRAP );
    pDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    pDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
	// clean up
    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_TEXTUREFACTOR, (DWORD)(255)<<24L );

    pDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    pDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    pDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    pDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

	pDevice->SetTexture( 0, NULL );
    return S_OK;
}
