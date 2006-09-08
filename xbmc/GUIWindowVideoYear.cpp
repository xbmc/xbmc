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

bool CGUIWindowVideoYear::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.m_strPath = strDirectory;
  if (m_vecItems.IsVirtualDirectoryRoot() || items.m_strPath.Equals("?"))
  {
    VECMOVIEYEARS years;
    m_database.GetYears(years, g_stSettings.m_iMyVideoWatchMode);
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
  items.SetCachedVideoThumbs();
  return true;
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
