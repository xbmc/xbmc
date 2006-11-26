#include "include.h"
#ifdef HAS_RAM_CONTROL
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
                               float posX, float posY, float width, float height,
                               const CLabelInfo &labelInfo, const CLabelInfo& titleInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_label = labelInfo;
  m_title = titleInfo;

  m_thumbnailWidth = 80;
  m_thumbnailHeight = 128;
  m_thumbnailSpaceX = 6;
  m_thumbnailSpaceY = 25;

  m_textSpaceY = BUTTON_Y_SPACING; // space between buttons

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
  if (!IsVisible()) return;

  if (g_guiSettings.GetBool("network.enableinternet"))
  {
    if (m_pMonitor == NULL)
    {
      // Create monitor background/worker thread
      m_pMonitor = CMediaMonitor::GetInstance(this);
    }


    // current images
    float imageX = m_posX + m_width + m_thumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

    for (int i = RECENT_MOVIES - 1; i >= 0; i--)
    {
      Movie& movie = m_current[i];

      bool bIsNewTitleAvailable = m_new[i].bValid;

      if (movie.bValid && movie.pImage)
      {
        imageX -= m_thumbnailWidth + m_thumbnailSpaceX;

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
          movie.pImage->SetPosition(imageX, m_posY);
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
    imageX = m_posX + m_width + m_thumbnailSpaceX - CONTROL_POSX_ADJUSTMENT;

    for (int i = RECENT_MOVIES - 1; i >= 0; i--)
    {
      Movie& movie = m_new[i];

      if (movie.bValid && movie.pImage)
      {
        imageX -= m_thumbnailWidth + m_thumbnailSpaceX;

        movie.nAlpha += 4;

        if (movie.nAlpha > 255)
        {
          movie.nAlpha = 255;
        }

        movie.pImage->SetAlpha((DWORD)movie.nAlpha);
        movie.pImage->SetPosition(imageX, m_posY);
        movie.pImage->Render();
      }
      else if (m_current[i].bValid)
      {
        imageX -= m_thumbnailWidth + m_thumbnailSpaceX;
      }
    }

    WCHAR wszText[256];
    float fTextX = m_posX + m_width;
    float fTextY = m_posY + m_thumbnailHeight + m_thumbnailSpaceY;

    if (m_title.font)
    {
      bool bBusy = (m_pMonitor && m_pMonitor->IsBusy() && (++m_dwCounter % 50 > 25) );

      swprintf(wszText, L"Recently Added to My Videos");

      m_title.font->DrawText( fTextX - CONTROL_POSX_ADJUSTMENT, fTextY, m_title.textColor, m_label.shadowColor, wszText, XBFONT_RIGHT);

      fTextY += m_fFontHeight + m_textSpaceY;
    }

    float textX = m_posX + m_width;

    for (int i = 0; i < RECENT_MOVIES; i++)
    {
      CGUIButtonControl* pButton = m_pTextButton[i];
      Movie& movie = m_new[i].bValid ? m_new[i] : m_current[i];

      if (movie.bValid)
      {
        float fTextWidth = 0;
        float fTextHeight = 0;
        CStdStringW movieTitle = movie.strTitle;
        if (m_title.font)
          m_title.font->GetTextExtent(movieTitle.c_str(), &fTextWidth, &fTextHeight);

        float buttonWidth = fTextWidth + BUTTON_WIDTH_ADJUSTMENT;
        float buttonHeight = fTextHeight + BUTTON_HEIGHT_ADJUSTMENT;
        bool itemHasFocus = (i == m_iSelection) && HasFocus();

        pButton->SetLabel(movie.strTitle);
        pButton->RAMSetTextColor(itemHasFocus ? m_label.selectedColor : m_label.textColor);
        pButton->SetPosition(textX - buttonWidth, fTextY),
        pButton->SetWidth(buttonWidth);
        pButton->SetHeight(buttonHeight);
        pButton->SetFocus(itemHasFocus);
        pButton->Render();

        fTextY += m_fFont2Height + m_textSpaceY;
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
      dlg->DoModal();
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
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  playlist.Clear();
  for (int i = iSelectedFile - 1; i < (int)movies.size(); ++i)
  {
    CStdString strFileName = movies[i];
    CPlayList::CPlayListItem item;
    item.SetFileName(strFileName);
    playlist.Add(item);
  }
  // play the first file on the playlist
  g_playlistPlayer.Play(0);
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
    movie.pImage = new CGUIImage(0, 0, 0, 0, m_thumbnailWidth, m_thumbnailHeight, strImagePath);
    movie.pImage->AllocResources();
  }
  else if ((!m_strDefaultThumb.IsEmpty()) && (m_strDefaultThumb[0] != '-'))
  {
    movie.pImage = new CGUIImage(0, 0, 0, 0, m_thumbnailWidth, m_thumbnailHeight, m_strDefaultThumb);
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
      m_pTextButton[i] = new CGUIButtonControl(m_dwControlID, 0, 0, 0, 0, 0, (CStdString)"button-focus.png", (CStdString)"", m_label);
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

#endif
