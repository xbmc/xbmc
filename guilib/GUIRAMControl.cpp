#include "stdafx.h"
#include "guiRAMControl.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"
#include "..\xbmc\Application.h"
#include "..\xbmc\Utils\Log.h"
#include "..\xbmc\PlayListPlayer.h"

extern CApplication g_application;

#define BUTTON_WIDTH_ADJUSTMENT		16
#define BUTTON_HEIGHT_ADJUSTMENT	5
#define CONTROL_POSX_ADJUSTMENT		8

CGUIRAMControl::CGUIRAMControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFontName, const CStdString& strFont2Name, D3DCOLOR dwTitleColor, D3DCOLOR dwNormalColor)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
{
	m_dwTitleColor		= dwTitleColor;
	m_dwTextColor		= dwNormalColor; 
	m_pFont				= g_fontManager.GetFont(strFontName);
	m_pFont2			= g_fontManager.GetFont(strFont2Name);

	m_dwThumbnailWidth	= 80;
	m_dwThumbnailHeight	= 128;
	m_dwThumbnailSpaceX = 6;
	m_dwThumbnailSpaceY = 25;
	m_dwTextSpaceY		= 12;

	m_pMonitor	= NULL;

	FLOAT fTextW;
	m_pFont->GetTextExtent(L"X", &fTextW, &m_fFontHeight);
	m_pFont2->GetTextExtent(L"X", &fTextW, &m_fFont2Height);

	for(int i=0;i<RECENT_MOVIES;i++)
	{
		m_pTextButton[i] = new CGUIButtonControl(m_dwControlID,0,0,0,0,0,"button-focus.png","");
		m_pTextButton[i]->SetLabel(strFont2Name,"",m_dwTextColor);
	}

	m_iSelection = 0;
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

	DWORD dwImageX;
	
	// current images
	dwImageX = m_dwPosX + m_dwWidth + m_dwThumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

	for(int i=RECENT_MOVIES-1; i>=0; i--)
	{
		Movie& movie = m_current[i];

		bool bIsNewImageAvailable = m_new[i].bValid;
	
		if (movie.bValid && movie.pImage)
		{
			dwImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;

			movie.nAlpha += bIsNewImageAvailable ? -4:4;

			if (movie.nAlpha<64)
			{
				movie.pImage->FreeResources();
				m_current[i] = m_new[i];
				m_new[i].bValid = false;
			}
			else if (movie.nAlpha>255)
			{
				movie.nAlpha = 255;
			}
			
			movie.pImage->SetAlpha((DWORD)movie.nAlpha);
			movie.pImage->SetPosition(dwImageX,m_dwPosY);
			movie.pImage->Render();		
		}
		else
		{
			m_current[i] = m_new[i];
			m_new[i].bValid = false;
		}
	}

	// new images
	dwImageX = m_dwPosX + m_dwWidth + m_dwThumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

	for(int i=RECENT_MOVIES-1; i>=0; i--)
	{
		Movie& movie = m_new[i];

		if (movie.bValid && movie.pImage)
		{
			dwImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;	

			movie.nAlpha+=4;

			if (movie.nAlpha>255)
			{
				movie.nAlpha = 255;
			}

			movie.pImage->SetAlpha((DWORD)movie.nAlpha);
			movie.pImage->SetPosition(dwImageX,m_dwPosY);
			movie.pImage->Render();		
		}
		else if (m_current[i].bValid)
		{
			dwImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;	
		}
	}




	WCHAR wszText[256];
	FLOAT fTextX = (FLOAT) m_dwPosX + m_dwWidth;
	FLOAT fTextY = (FLOAT) m_dwPosY + m_dwThumbnailHeight + m_dwThumbnailSpaceY;

	if (m_pFont)
	{
		swprintf(wszText,L"Recently Added to My Videos");

		m_pFont->DrawText(	fTextX - CONTROL_POSX_ADJUSTMENT, fTextY, m_dwTitleColor, wszText, XBFONT_RIGHT);

		fTextY += m_fFontHeight + (FLOAT) m_dwTextSpaceY;
	}
/*
	if (m_pFont2)
	{
		for(int i=0; i<RECENT_MOVIES; i++)
		{			
			Movie& movie = m_new[i].bValid ? m_new[i] : m_current[i];
			if(movie.bValid)
			{
				swprintf(wszText, L"%S", movie.strTitle.c_str() );
				m_pFont2->DrawText(	fTextX, fTextY, m_dwTextColor, wszText, XBFONT_RIGHT);
				fTextY += m_fFont2Height + (FLOAT) m_dwTextSpaceY;
			}
		}
	}
*/

		DWORD dwTextX = m_dwPosX + m_dwWidth;

		for(int i=0; i<RECENT_MOVIES; i++)
		{			
			CGUIButtonControl* pButton = m_pTextButton[i];
			Movie& movie = m_new[i].bValid ? m_new[i] : m_current[i];

			if(movie.bValid)
			{
				FLOAT fTextWidth,fTextHeight;
				swprintf(wszText, L"%S", movie.strTitle.c_str() );
				m_pFont2->GetTextExtent(wszText, &fTextWidth, &fTextHeight);
				
				INT iButtonWidth = (INT) (fTextWidth+BUTTON_WIDTH_ADJUSTMENT);
				INT iButtonHeight= (INT) (fTextHeight+BUTTON_HEIGHT_ADJUSTMENT);

				pButton->SetText(movie.strTitle);
				pButton->SetPosition(dwTextX-(DWORD)iButtonWidth,(DWORD)fTextY),
				pButton->SetWidth(iButtonWidth);
				pButton->SetHeight(iButtonHeight);
				pButton->SetFocus( (i==m_iSelection) && HasFocus() );
				pButton->Render();

				fTextY += m_fFont2Height + (FLOAT) m_dwTextSpaceY;
			}
		}
	
}



void CGUIRAMControl::OnAction(const CAction &action)
{
	switch (action.wID)
	{
		case ACTION_MOVE_DOWN:
		{
			m_iSelection++;
			if (m_iSelection>=RECENT_MOVIES)
			{
				m_iSelection = RECENT_MOVIES-1;
			}
			break;
		}
    
		case ACTION_MOVE_UP:
		{
			m_iSelection--;
			if (m_iSelection<0)
			{
				m_iSelection = 0;
			}
			break;
		}

		case ACTION_SELECT_ITEM:
		{
			//int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
			//CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
			//m_gWindowManager.SendMessage( msg );
			//g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
			//g_playlistPlayer.Play(nSize);
			g_application.PlayFile( m_current[m_iSelection].strFilepath );
			break;
		}

		default:
		{
			CGUIControl::OnAction(action);
			break;
		}
	}
}

void CGUIRAMControl::OnMediaUpdate(	INT nIndex, CStdString& strFilepath,
								CStdString& strTitle, CStdString& strImagePath)
{
	CLog::Log( "OnMediaUpdate: " );
	CLog::Log( strFilepath.c_str() );
	
	if (m_current[nIndex].strFilepath.Equals(strFilepath))
	{
		CLog::Log( "OnMediaUpdate complete." );
		return;		
	}

	Movie movie;
	movie.strFilepath	= strFilepath;
	movie.strTitle		= strTitle;
	movie.pImage		= NULL;
	movie.nAlpha		= 64;
	movie.bValid		= true;

	if (!strImagePath.IsEmpty())
	{
		movie.pImage = new CGUIImage(0,0,0,0,m_dwThumbnailWidth,m_dwThumbnailHeight,strImagePath);
		movie.pImage->AllocResources();
	}

	m_new[nIndex] = movie;

	CLog::Log( "OnMediaUpdate complete." );
}


void CGUIRAMControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();

	for(int i=0;i<RECENT_MOVIES;i++)
	{
		m_pTextButton[i]->PreAllocResources();
	}
}

void CGUIRAMControl::AllocResources()
{
	CGUIControl::AllocResources(); 

	for(int i=0;i<RECENT_MOVIES;i++)
	{
		m_pTextButton[i]->AllocResources();
	}
}

void CGUIRAMControl::FreeResources()
{
	CGUIControl::FreeResources();

	for(int i=0;i<RECENT_MOVIES;i++)
	{
		m_pTextButton[i]->FreeResources();
	}
}
