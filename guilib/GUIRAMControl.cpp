#include "include.h"
#include "GUIRAMControl.h"
#include "GUIWindowManager.h"
#include "..\xbmc\Util.h"
#include "..\xbmc\Utils\fstrcmp.h"
#include "..\xbmc\Application.h"
#include "..\xbmc\PlayListPlayer.h"
#include "..\xbmc\GUIsettings.h"
#include "..\xbmc\GUIDialogFileStacking.h"
#include "..\xbmc\GUIDialogKeyboard.h"
#include "..\xbmc\FileSystem\VirtualDirectory.h"
#include "..\xbmc\FileSystem\StackDirectory.h"
#include "LocalizeStrings.h"

extern CApplication g_application;

#define BUTTON_WIDTH_ADJUSTMENT  16
#define BUTTON_HEIGHT_ADJUSTMENT 5
#define CONTROL_POSX_ADJUSTMENT  8
#define BUTTON_Y_SPACING   12

CGUIRAMControl::CGUIRAMControl(DWORD dwParentID, DWORD dwControlId,
                               int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                               const CLabelInfo &labelInfo, const CLabelInfo& titleInfo)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
{
  m_label = labelInfo;
  m_title = titleInfo;

  m_dwThumbnailWidth = 80;
  m_dwThumbnailHeight = 128;
  m_dwThumbnailSpaceX = 6;
  m_dwThumbnailSpaceY = 25;

  m_dwTextSpaceY = BUTTON_Y_SPACING; // space between buttons

  m_pMonitor = NULL;

  FLOAT fTextW;
  if (m_label.font)
    m_label.font->GetTextExtent(L"X", &fTextW, &m_fFontHeight);
  if (m_title.font)
    m_title.font->GetTextExtent(L"X", &fTextW, &m_fFont2Height);

  for (int i = 0;i < RECENT_MOVIES;i++)
  {
    m_pTextButton[i]=NULL;
    m_current[i].pImage=NULL;
    m_new[i].pImage=NULL;
 }

  m_iSelection = 0;
  m_dwCounter = 0;
  ControlType = GUICONTROL_RAM;
}

CGUIRAMControl::~CGUIRAMControl(void)
{
  for (int i = 0;i < RECENT_MOVIES;i++)
  {
    CGUIImage* pImage=m_current[i].pImage;
    if (m_current[i].bValid && pImage)
    {
      pImage->FreeResources();
      delete pImage;
    }
      
    pImage=m_new[i].pImage;
    if (m_new[i].bValid && pImage)
    {
      pImage->FreeResources();
      delete pImage;
    }

    if (m_pTextButton[i])
      delete m_pTextButton[i];
    m_pTextButton[i]=NULL;
 }
}

void CGUIRAMControl::Render()
{
  if (!UpdateEffectState())
  {
    return ;
  }

  if (g_guiSettings.GetBool("Network.EnableInternet"))
  {
    if (m_pMonitor == NULL)
    {
      // Create monitor background/worker thread
      m_pMonitor = CMediaMonitor::GetInstance(this);
    }

    int iImageX;

    // current images
    iImageX = m_iPosX + m_dwWidth + m_dwThumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

    for (int i = RECENT_MOVIES - 1; i >= 0; i--)
    {
      Movie& movie = m_current[i];

      bool bIsNewTitleAvailable = m_new[i].bValid;

      if (movie.bValid && movie.pImage)
      {
        iImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;

        movie.nAlpha += bIsNewTitleAvailable ? -4 : 4;

        int nLowWatermark = 64;
        if (bIsNewTitleAvailable && !m_new[i].pImage)
        {
          nLowWatermark = 1;
        }

        if (movie.nAlpha < nLowWatermark)
        {
          movie.pImage->FreeResources();
          delete movie.pImage;
          movie.pImage=NULL;
          m_current[i] = m_new[i];
          m_new[i].bValid = false;
          m_new[i].pImage=NULL;
        }
        else if (movie.nAlpha > 255)
        {
          movie.nAlpha = 255;
        }

        if (movie.pImage)
        {
          movie.pImage->SetAlpha((DWORD)movie.nAlpha);
          movie.pImage->SetPosition(iImageX, m_iPosY);
          movie.pImage->Render();
        }
      }
      else if (bIsNewTitleAvailable)
      {
        m_current[i] = m_new[i];
        m_new[i].bValid = false;
        m_new[i].pImage=NULL;
      }
    }

    // new images
    iImageX = m_iPosX + m_dwWidth + m_dwThumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

    for (int i = RECENT_MOVIES - 1; i >= 0; i--)
    {
      Movie& movie = m_new[i];

      if (movie.bValid && movie.pImage)
      {
        iImageX -= m_dwThumbnailWidth + m_dwThumbnailSpaceX;

        movie.nAlpha += 4;

        if (movie.nAlpha > 255)
        {
          movie.nAlpha = 255;
        }

        movie.pImage->SetAlpha((DWORD)movie.nAlpha);
        movie.pImage->SetPosition(iImageX, m_iPosY);
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

    if (m_title.font)
    {
      bool bBusy = (m_pMonitor && m_pMonitor->IsBusy() && (++m_dwCounter % 50 > 25) );

      swprintf(wszText, L"Recently Added to My Videos");

      m_title.font->DrawText( fTextX - CONTROL_POSX_ADJUSTMENT, fTextY, m_title.textColor, m_label.shadowColor, wszText, XBFONT_RIGHT);

      fTextY += m_fFontHeight + (FLOAT) m_dwTextSpaceY;
    }

    int iTextX = m_iPosX + m_dwWidth;

    for (int i = 0; i < RECENT_MOVIES; i++)
    {
      CGUIButtonControl* pButton = m_pTextButton[i];
      Movie& movie = m_new[i].bValid ? m_new[i] : m_current[i];

      if (movie.bValid)
      {
        float fTextWidth = 0;
        float fTextHeight = 0;
        swprintf(wszText, L"%S", movie.strTitle.c_str() );
        if (m_title.font)
          m_title.font->GetTextExtent(wszText, &fTextWidth, &fTextHeight);

        int iButtonWidth = (int) (fTextWidth + BUTTON_WIDTH_ADJUSTMENT);
        int iButtonHeight = (int) (fTextHeight + BUTTON_HEIGHT_ADJUSTMENT);
        bool itemHasFocus = (i == m_iSelection) && HasFocus();

        pButton->SetText(movie.strTitle);
        pButton->RAMSetTextColor(itemHasFocus ? m_label.selectedColor : m_label.textColor);
        pButton->SetPosition(iTextX - iButtonWidth, (int)fTextY),
        pButton->SetWidth(iButtonWidth);
        pButton->SetHeight(iButtonHeight);
        pButton->SetFocus(itemHasFocus);
        pButton->Render();

        fTextY += m_fFont2Height + (FLOAT) m_dwTextSpaceY;
      }
    }
  }
  CGUIControl::Render();
}



bool CGUIRAMControl::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_MOVE_DOWN:
    {
      m_iSelection++;
      if (m_iSelection >= RECENT_MOVIES)
      {
        m_iSelection = RECENT_MOVIES - 1;
      }
      return true;
      break;
    }

  case ACTION_MOVE_UP:
    {
      m_iSelection--;
      if (m_iSelection < 0)
      {
        m_iSelection = 0;
      }
      return true;
      break;
    }

  case ACTION_SELECT_ITEM:
    {
      CFileItem item(m_current[m_iSelection].strFilepath, false);
      PlayMovie( item );
      return true;
      break;
    }

  case ACTION_SHOW_INFO:
    {
      UpdateTitle(m_current[m_iSelection].strFilepath, m_iSelection);
      return true;
      break;
    }

  case ACTION_SHOW_GUI:
    {
      UpdateAllTitles();
      return true;
      break;
    }

  default:
    {
      return CGUIControl::OnAction(action);
      break;
    }
  }
}

void CGUIRAMControl::PlayMovie(CFileItem& item)
{
  vector<CStdString> movies;
  // is video stacking enabled?
  if (item.IsStack())
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(item.m_strPath, items);
    for (int i = 0; i < items.Size(); ++i)
      movies.push_back(items[i]->m_strPath);
  }

  if (movies.size() <= 0)
  {
    // might as well play the file that we do know about!
    g_application.PlayFile( item );
    return ;
  }

  // if this movie was split into multiple files
  int iSelectedFile = 1;
  if (movies.size() > 1)
  {
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
        return ;
      }
    }
  }

  // create a playlist containing the appropriate movie files
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
  playlist.Clear();
  for (int i = iSelectedFile - 1; i < (int)movies.size(); ++i)
  {
    CStdString strFileName = movies[i];
    CPlayList::CPlayListItem item;
    item.SetFileName(strFileName);
    playlist.Add(item);
  }

  // TODO: there is something funny about the way this works - if the
  // volume we started on was not the first volume in the series, it
  // is not possible to go backwards to previous volumes before this one.

  // play the first file on the playlist
  g_playlistPlayer.PlayNext();
  return ;
}

void CGUIRAMControl::UpdateAllTitles()
{
  CMediaMonitor::Command command;
  command.rCommand = CMediaMonitor::CommandType::Seed;
  if (m_pMonitor) m_pMonitor->QueueCommand(command);
}

void CGUIRAMControl::UpdateTitle(CStdString& strFilepath, INT nIndex)
{
  CStdString strFilename = CUtil::GetFileName(strFilepath);
  if (CGUIDialogKeyboard::ShowAndGetInput(strFilename, (CStdStringW)g_localizeStrings.Get(16009), false))
  {
    CMediaMonitor::Command command = {
                                       CMediaMonitor::CommandType::Update,
                                       strFilename, strFilepath, nIndex };

    if (m_pMonitor) m_pMonitor->QueueCommand(command);
  }
}

void CGUIRAMControl::OnMediaUpdate( INT nIndex, CStdString& strFilepath,
                                    CStdString& strTitle, CStdString& strImagePath)
{
  CLog::Log(LOGINFO, "OnMediaUpdate: " );
  CLog::Log(LOGINFO, strFilepath.c_str() );

  if (strTitle.GetLength() > 64)
  {
    strTitle = strTitle.Mid(0, 63);
  }

  if ( (m_current[nIndex].strFilepath.Equals(strFilepath)) &&
       (m_current[nIndex].strTitle.Equals(strTitle)) )
  {
    CLog::Log(LOGINFO, "OnMediaUpdate complete." );
    return ;
  }

  Movie movie;
  movie.strFilepath = strFilepath;
  movie.strTitle = strTitle;
  movie.pImage = NULL;
  movie.nAlpha = 64;
  movie.bValid = true;

  if (!strImagePath.IsEmpty())
  {
    movie.pImage = new CGUIImage(0, 0, 0, 0, m_dwThumbnailWidth, m_dwThumbnailHeight, strImagePath);
    movie.pImage->AllocResources();
  }
  else if ((!m_strDefaultThumb.IsEmpty()) && (m_strDefaultThumb[0] != '-'))
  {
    movie.pImage = new CGUIImage(0, 0, 0, 0, m_dwThumbnailWidth, m_dwThumbnailHeight, m_strDefaultThumb);
    movie.pImage->AllocResources();
  }

  m_new[nIndex] = movie;

  CLog::Log(LOGINFO, "OnMediaUpdate complete." );
}


void CGUIRAMControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();

  for (int i = 0;i < RECENT_MOVIES;i++)
  {
    if (!m_pTextButton[i])
    {
      m_pTextButton[i] = new CGUIButtonControl(m_dwControlID, 0, 0, 0, 0, 0, "button-focus.png", "", m_label);
      m_pTextButton[i]->SetPulseOnSelect(m_pulseOnSelect);
    }
    m_pTextButton[i]->PreAllocResources();
  }
}

void CGUIRAMControl::AllocResources()
{
  CGUIControl::AllocResources();

  for (int i = 0;i < RECENT_MOVIES;i++)
  {
    if (m_pTextButton[i])
      m_pTextButton[i]->AllocResources();
  }
}

void CGUIRAMControl::FreeResources()
{
  if (m_pMonitor)
    m_pMonitor->SetObserver(NULL);
  m_pMonitor = NULL;
  for (int i = 0;i < RECENT_MOVIES;i++)
  {
    if (m_pTextButton[i])
      m_pTextButton[i]->FreeResources();
  }
  CGUIControl::FreeResources();
}
