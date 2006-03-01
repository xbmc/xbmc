// Todo:
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoYear.h"
#include "Util.h"
#include "Utils/imdb.h"
#include "GUIWindowVideoInfo.h"
#include "NFOFile.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_IMDB        9
#define CONTROL_BTNSHOWMODE       10
#define LABEL_YEAR              100


//****************************************************************************************************************************
CGUIWindowVideoYear::CGUIWindowVideoYear()
: CGUIWindowVideoBase(WINDOW_VIDEO_YEAR, "MyVideoYear.xml")
{
  m_vecItems.m_strPath = "";
}

//****************************************************************************************************************************
CGUIWindowVideoYear::~CGUIWindowVideoYear()
{
}

//****************************************************************************************************************************
bool CGUIWindowVideoYear::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_iShowMode = g_stSettings.m_iMyVideoYearShowMode;
    }
    break;
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSHOWMODE)
	    {
        m_iShowMode++;
		    if (m_iShowMode > VIDEO_SHOW_WATCHED) m_iShowMode = VIDEO_SHOW_ALL;
		    g_stSettings.m_iMyVideoYearShowMode = m_iShowMode;
        g_settings.Save();
		    Update(m_vecItems.m_strPath);
        return true;
      }
      else
        return CGUIWindowVideoBase::OnMessage(message);
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

bool CGUIWindowVideoYear::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.m_strPath = strDirectory;
  if (m_vecItems.IsVirtualDirectoryRoot())
  {
    VECMOVIEYEARS years;
    m_database.GetYears(years, m_iShowMode);
    // Display an error message if the database doesn't contain any years
    DisplayEmptyDatabaseMessage(years.empty());
    for (int i = 0; i < (int)years.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(years[i]);
      pItem->m_strPath = years[i];
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
    }
  }
  else
  {
    if (m_guiState.get() && !m_guiState->HideParentDirItems())
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.Add(pItem);
    }
    VECMOVIES movies;
    m_database.GetMoviesByYear(m_vecItems.m_strPath, movies);
    SetDatabaseDirectory(movies, items);
  }
  return true;
}

void CGUIWindowVideoYear::OnPrepareFileItems(CFileItemList &items)
{
  items.SetThumbs();

  // Fill in default icons
  // normally this is done by the base class for us
  // but the m_strPath only contains the database id
  // of the movie, so we must fake a videofile to fill
  // in default icons
  CStdString strPath;
  for (int i = 0; i < (int)items.Size(); i++)
  {
    CFileItem* pItem = items[i];

    if (pItem->m_bIsFolder)
      continue;

    strPath = pItem->m_strPath;
    // Fake a videofile
    pItem->m_strPath = pItem->m_strPath + ".avi";
    pItem->FillInDefaultIcon();
    pItem->m_strPath = strPath;
  }
}

void CGUIWindowVideoYear::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  SET_CONTROL_LABEL(LABEL_YEAR, m_vecItems.m_strPath);
}

void CGUIWindowVideoYear::OnInfo(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  if ( m_vecItems.IsVirtualDirectoryRoot() ) return ;
  CGUIWindowVideoBase::OnInfo(iItem);
}
