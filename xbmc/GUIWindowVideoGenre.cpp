// Todo:
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoGenre.h"
#include "Util.h"
#include "Utils/imdb.h"
#include "GUIWindowVideoInfo.h"
#include "NFOFile.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"

#define LABEL_GENRE              100


//****************************************************************************************************************************
CGUIWindowVideoGenre::CGUIWindowVideoGenre()
: CGUIWindowVideoBase(WINDOW_VIDEO_GENRE, "MyVideoGenre.xml")
{
  m_vecItems.m_strPath = "";
}

//****************************************************************************************************************************
CGUIWindowVideoGenre::~CGUIWindowVideoGenre()
{
}

bool CGUIWindowVideoGenre::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  items.m_strPath = strDirectory;
  if (items.IsVirtualDirectoryRoot())
  {
    VECMOVIEGENRES genres;
    m_database.GetGenres(genres, g_stSettings.m_iMyVideoWatchMode);
    // Display an error message if the database doesn't contain any genres
    DisplayEmptyDatabaseMessage(genres.empty());
    for (int i = 0; i < (int)genres.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(genres[i]);
      pItem->m_strPath = genres[i];
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.Add(pItem);
    }
  }
  else
  {
    // add the parent item
    if (m_guiState.get() && !m_guiState->HideParentDirItems())
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      items.Add(pItem);
    }
    VECMOVIES movies;
    m_database.GetMoviesByGenre(items.m_strPath, movies);
    SetDatabaseDirectory(movies, items);
    items.SetCachedVideoThumbs();
  }
  return true;
}

void CGUIWindowVideoGenre::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  SET_CONTROL_LABEL(LABEL_GENRE, m_vecItems.m_strPath);
}

void CGUIWindowVideoGenre::OnInfo(int iItem)
{
  if ( m_vecItems.IsVirtualDirectoryRoot() ) return ;
  CGUIWindowVideoBase::OnInfo(iItem);
}
