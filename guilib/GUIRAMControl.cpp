#include "stdafx.h"
#include "guiRAMControl.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"
#include "..\xbmc\Application.h"
#include "..\xbmc\Utils\Log.h"

extern CApplication g_application;

CGUIRAMControl::CGUIRAMControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFontName, const CStdString& strFont2Name, D3DCOLOR dwTitleColor, D3DCOLOR dwNormalColor)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
{
	m_dwTitleColor		= dwTitleColor;
	m_dwTextColor		= dwNormalColor; 
	m_pFont				= g_fontManager.GetFont(strFontName);
	m_pFont2			= g_fontManager.GetFont(strFont2Name);

	m_dwThumbnailWidth	= 120;
	m_dwThumbnailHeight	= 192;
	m_dwThumbnailSpaceX = 6;
	m_dwThumbnailSpaceY = 25;
	m_dwTextSpaceY		= 12;

	m_pMonitor	= NULL;

	FLOAT fTextW;
	m_pFont->GetTextExtent(L"X", &fTextW, &m_fFontHeight);
	m_pFont2->GetTextExtent(L"X", &fTextW, &m_fFont2Height);
}

CGUIRAMControl::~CGUIRAMControl(void)
{
}

void CGUIRAMControl::Render()
{
	if (!IsVisible())
	{
		return;
	}

	if (m_pMonitor==NULL)
	{
		// Create monitor background/worker thread
		m_pMonitor = new CMediaMonitor();
		m_pMonitor->Create(this);
	}

	int nRecentMovies = (int) m_movies.size();

	DWORD dwImageX = m_dwPosX;

	for(int i=0; i<nRecentMovies; i++)
	{
		Movie& movie = m_movies[i];

		if (movie.pImage!=NULL)
		{
			movie.pImage->SetPosition(dwImageX,m_dwPosY);
			movie.pImage->Render();
			dwImageX += movie.pImage->GetWidth() + m_dwThumbnailSpaceX;
		}
	}

	WCHAR wszText[512];
	FLOAT fTextX = (FLOAT) m_dwPosX + m_dwWidth;
	FLOAT fTextY = (FLOAT) m_dwPosY + m_dwThumbnailHeight + m_dwThumbnailSpaceY;

	if (m_pFont)
	{
		swprintf(wszText,L"Recently Added to My Videos");

		m_pFont->DrawText(	fTextX,	fTextY,	m_dwTitleColor, wszText, XBFONT_RIGHT);

		fTextY += m_fFontHeight + (FLOAT) m_dwTextSpaceY;
	}

	if ((m_pFont2) && (nRecentMovies>0))
	{
		for(int i=0; i<nRecentMovies; i++)
		{
			Movie& movie = m_movies[i];

			swprintf(wszText, L"%S", movie.strTitle.c_str() );
	
			m_pFont2->DrawText(	fTextX, fTextY, m_dwTextColor, wszText, XBFONT_RIGHT);

			fTextY += m_fFont2Height + (FLOAT) m_dwTextSpaceY;
		}
	}

}


void CGUIRAMControl::OnMediaUpdate(	INT nIndex, CStdString& strFilepath,
								CStdString& strTitle, CStdString& strImagePath)
{
	CLog::Log( "OnMediaUpdate: " );
	CLog::Log( strFilepath.c_str() );
	
	Movie movie;
	movie.strFilepath	= strFilepath;
	movie.strTitle		= strTitle;
	movie.pImage		= NULL;

	if (!strImagePath.IsEmpty())
	{
		movie.pImage = new CGUIImage(0,0,0,0,m_dwThumbnailWidth,m_dwThumbnailHeight,strImagePath);
		movie.pImage->AllocResources();
	}

	m_movies.push_back(movie);

	CLog::Log( "OnMediaUpdate complete." );
}