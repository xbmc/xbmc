#include "stdafx.h"
#include "guiRAMControl.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"
#include "..\xbmc\Util.h"
#include "..\xbmc\Utils\Log.h"
#include "..\xbmc\Utils\fstrcmp.h"
#include "..\xbmc\Application.h"
#include "..\xbmc\PlayListPlayer.h"

extern CApplication g_application;

#define BUTTON_WIDTH_ADJUSTMENT		16
#define BUTTON_HEIGHT_ADJUSTMENT	5
#define CONTROL_POSX_ADJUSTMENT		8
#define BUTTON_Y_SPACING			12

CGUIRAMControl::CGUIRAMControl(DWORD dwParentID, DWORD dwControlId, 
							   int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
							   const CStdString& strFontName, const CStdString& strFont2Name,
							   D3DCOLOR dwTitleColor, D3DCOLOR dwNormalColor, D3DCOLOR dwSelectedColor,
							   DWORD dwTextOffsetX, DWORD dwTextOffsetY)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
{
	m_dwTitleColor		= dwTitleColor;
	m_dwTextColor		= dwNormalColor; 
	m_dwTextSelectColor = dwSelectedColor;
	m_pFont				= g_fontManager.GetFont(strFontName);
	m_pFont2			= g_fontManager.GetFont(strFont2Name);

	m_dwThumbnailWidth	= 80;
	m_dwThumbnailHeight	= 128;
	m_dwThumbnailSpaceX = 6;
	m_dwThumbnailSpaceY = 25;
	m_dwTextOffsetX		= dwTextOffsetX;
	m_dwTextOffsetY		= dwTextOffsetY; // text offset from button

	m_dwTextSpaceY		= BUTTON_Y_SPACING;	// space between buttons

	m_pMonitor	= NULL;

	FLOAT fTextW;
	if (m_pFont)
		m_pFont->GetTextExtent(L"X", &fTextW, &m_fFontHeight);
	if (m_pFont2)
		m_pFont2->GetTextExtent(L"X", &fTextW, &m_fFont2Height);

	for(int i=0;i<RECENT_MOVIES;i++)
	{
		m_pTextButton[i] = new CGUIButtonControl(m_dwControlID,0,0,0,0,0,"button-focus.png","",dwTextOffsetX,dwTextOffsetY);
		m_pTextButton[i]->SetLabel(strFont2Name,"",m_dwTextColor);
	}

	m_iSelection = 0;
	m_dwCounter = 0;
	ControlType = GUICONTROL_RAM;
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

	int iImageX;
	
	// current images
	iImageX = m_iPosX + m_dwWidth + m_dwThumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

	for(int i=RECENT_MOVIES-1; i>=0; i--)
	{
		Movie& movie = m_current[i];

		bool bIsNewTitleAvailable = m_new[i].bValid;
	
		if (movie.bValid && movie.pImage)
		{
			iImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;

			movie.nAlpha += bIsNewTitleAvailable ? -4:4;
			
			int nLowWatermark = 64;
			if (bIsNewTitleAvailable && !m_new[i].pImage)
			{
				nLowWatermark = 1;
			}

			if (movie.nAlpha<nLowWatermark)
			{
				movie.pImage->FreeResources();
				m_current[i] = m_new[i];
				m_new[i].bValid = false;
			}
			else if (movie.nAlpha>255)
			{
				movie.nAlpha = 255;
			}
			
			if (movie.pImage)
			{
				movie.pImage->SetAlpha((DWORD)movie.nAlpha);
				movie.pImage->SetPosition(iImageX,m_iPosY);
				movie.pImage->Render();		
			}
		}
		else if (bIsNewTitleAvailable)
		{
			m_current[i] = m_new[i];
			m_new[i].bValid = false;
		}
	}

	// new images
	iImageX = m_iPosX + m_dwWidth + m_dwThumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

	for(int i=RECENT_MOVIES-1; i>=0; i--)
	{
		Movie& movie = m_new[i];

		if (movie.bValid && movie.pImage)
		{
			iImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;	

			movie.nAlpha+=4;

			if (movie.nAlpha>255)
			{
				movie.nAlpha = 255;
			}

			movie.pImage->SetAlpha((DWORD)movie.nAlpha);
			movie.pImage->SetPosition(iImageX,m_iPosY);
			movie.pImage->Render();		
		}
		else if (m_current[i].bValid)
		{
			iImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;	
		}
	}

	WCHAR wszText[256];
	FLOAT fTextX = (FLOAT) m_iPosX + m_dwWidth;
	FLOAT fTextY = (FLOAT) m_iPosY + m_dwThumbnailHeight + m_dwThumbnailSpaceY;

	if (m_pFont)
	{
		bool bBusy = (m_pMonitor && m_pMonitor->IsBusy() && (++m_dwCounter%50>25) );

		swprintf(wszText,L"Recently Added to My Videos");

		m_pFont->DrawText(	fTextX - CONTROL_POSX_ADJUSTMENT, fTextY, m_dwTitleColor, wszText, XBFONT_RIGHT);

		fTextY += m_fFontHeight + (FLOAT) m_dwTextSpaceY;
	}

	int iTextX = m_iPosX + m_dwWidth;

	for(int i=0; i<RECENT_MOVIES; i++)
	{			
		CGUIButtonControl* pButton = m_pTextButton[i];
		Movie& movie = m_new[i].bValid ? m_new[i] : m_current[i];

		if(movie.bValid)
		{
			float fTextWidth=0;
			float fTextHeight=0;
			swprintf(wszText, L"%S", movie.strTitle.c_str() );
			if (m_pFont2)
				m_pFont2->GetTextExtent(wszText, &fTextWidth, &fTextHeight);

			int iButtonWidth = (int) (fTextWidth+BUTTON_WIDTH_ADJUSTMENT);
			int iButtonHeight= (int) (fTextHeight+BUTTON_HEIGHT_ADJUSTMENT);
			bool itemHasFocus = (i==m_iSelection) && HasFocus();

			pButton->SetText(movie.strTitle);
			pButton->SetTextColor(itemHasFocus?m_dwTextSelectColor:m_dwTextColor);
			pButton->SetPosition(iTextX-iButtonWidth,(int)fTextY),
			pButton->SetWidth(iButtonWidth);
			pButton->SetHeight(iButtonHeight);
			pButton->SetFocus(itemHasFocus);
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
			CFileItem item;
			item.m_strPath = m_current[m_iSelection].strFilepath;
			PlayMovie( item );
			break;
		}

		case ACTION_SHOW_INFO:
		{
			UpdateTitle(m_current[m_iSelection].strFilepath, m_iSelection);
			break;
		}

		case ACTION_SHOW_GUI:
		{
			UpdateAllTitles();
			break;
		}

		default:
		{
			CGUIControl::OnAction(action);
			break;
		}
	}
}

struct CGUIRAMControl::SSortVideoListByName
{
	bool operator()(CStdString& strItem1, CStdString& strItem2)
	{
    return strcmp(strItem1.c_str(),strItem2.c_str())<0;
  }
};

void CGUIRAMControl::PlayMovie(CFileItem& item)
{
	// is video stacking enabled?	
	if (g_stSettings.m_bMyVideoVideoStack)
	{
		CStdString strDirectory;
		CUtil::GetDirectory(item.m_strPath,strDirectory);

		// create a list of associated movie files (some movies span across multiple files!)
		vector<CStdString> movies;
		{
			// create a list of all the files in the same directory
			VECFILEITEMS items;
			CVirtualDirectory dir;
			dir.SetShares(g_settings.m_vecMyVideoShares);
			dir.SetMask(g_stSettings.m_szMyVideoExtensions);
			dir.GetDirectory(strDirectory,items);

			// iterate through all the files, adding any files that appear similar to the selected movie file
			for (int i=0; i < (int)items.size(); ++i)
			{
				CFileItem *pItemTmp=items[i];
				if (!CUtil::IsNFO( pItemTmp->m_strPath) && !CUtil::IsPlayList(pItemTmp->m_strPath) )
				{
					double fPercentage=fstrcmp(CUtil::GetFileName(pItemTmp->m_strPath),CUtil::GetFileName(item.m_strPath),COMPARE_PERCENTAGE_MIN);
					if (fPercentage >=COMPARE_PERCENTAGE)
					{
						if (CUtil::IsVideo(pItemTmp->m_strPath))
						{
							movies.push_back(pItemTmp->m_strPath);
						}
					}
				}
			}
			
			CFileItemList itemlist(items); // will clean up everything
		}
		
		// if for some strange reason we couldn't find any matching files (can't think of any reason why!)
		if (movies.size() <=0)
		{
			CLog::Log(LOGWARNING, "*WARNING* Wibble! CGUIRAMControl was unable to find matching file!");

			// might as well play the file that we do know about!
			g_application.PlayFile( item );
			return;
		}
    
		// if this movie was split into multiple files
		int iSelectedFile=1;
		if (movies.size()>1)
		{
			// sort the files in alphabetical order
			sort(movies.begin(), movies.end(), SSortVideoListByName());

			// prompt the user to select a file from which to start playback (playback then continues across the
			// selected file and the remainder of the files).
			CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
			if (dlg)
			{
				dlg->SetNumberOfFiles(movies.size());
				dlg->DoModal(m_dwParentID);
				iSelectedFile = dlg->GetSelectedFile();
				if (iSelectedFile < 1)
				{
					// the user decided to cancel playback
					return;
				}
			}
		}

		// create a playlist containing the appropriate movie files
		g_playlistPlayer.Reset();
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
		CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
		playlist.Clear();
		for (int i=iSelectedFile-1; i < (int)movies.size(); ++i)
		{
			CStdString strFileName = movies[i];
			CPlayList::CPlayListItem item;
			item.SetFileName(strFileName);
			playlist.Add(item);
		}

		// play the first file on the playlist
		g_playlistPlayer.PlayNext();
		return;
	}

	// stacking disabled, just play the movie
	g_application.PlayFile(item);
}


void CGUIRAMControl::UpdateAllTitles()
{
	CMediaMonitor::Command command;
	command.rCommand = CMediaMonitor::CommandType::Seed;
	m_pMonitor->QueueCommand(command);
}

void CGUIRAMControl::UpdateTitle(CStdString& strFilepath, INT nIndex)
{
	CGUIDialogKeyboard& dialog = *((CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD));
	
	CStdString strFilename = CUtil::GetFileName(strFilepath);

	dialog.CenterWindow();
	dialog.SetText(strFilename);
	dialog.DoModal(m_dwParentID);
	dialog.Close();	

	if (dialog.IsDirty())
	{
		CMediaMonitor::Command command = {
			CMediaMonitor::CommandType::Update, 
			dialog.GetText(), strFilepath, nIndex };

		m_pMonitor->QueueCommand(command);
	}
}

void CGUIRAMControl::OnMediaUpdate(	INT nIndex, CStdString& strFilepath,
								CStdString& strTitle, CStdString& strImagePath)
{
	CLog::Log(LOGINFO, "OnMediaUpdate: " );
	CLog::Log(LOGINFO, strFilepath.c_str() );
	
 	if (strTitle.GetLength()>64)
	{
		strTitle = strTitle.Mid(0,63);
	}

	if ( (m_current[nIndex].strFilepath.Equals(strFilepath)) &&
		(m_current[nIndex].strTitle.Equals(strTitle)) )
	{
		CLog::Log(LOGINFO, "OnMediaUpdate complete." );
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
	else if ((!m_strDefaultThumb.IsEmpty()) && (m_strDefaultThumb[0]!='-'))
	{
		movie.pImage = new CGUIImage(0,0,0,0,m_dwThumbnailWidth,m_dwThumbnailHeight,m_strDefaultThumb);
		movie.pImage->AllocResources();
	}

	m_new[nIndex] = movie;

	CLog::Log(LOGINFO, "OnMediaUpdate complete." );
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
