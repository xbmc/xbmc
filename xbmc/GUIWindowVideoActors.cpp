// Todo:
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoActors.h"
#include "Util.h"
#include "GUIWindowVideoInfo.h"
#include "application.h"
#include "nfofile.h"
#include "GUIPassword.h"

#define CONTROL_BTNVIEWASICONS   2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_BTNTYPE            5
#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_IMDB        9
#define CONTROL_LIST       50
#define CONTROL_THUMBS      51
#define CONTROL_LABELFILES        12
#define LABEL_ACTOR              100

//****************************************************************************************************************************
struct SSortVideoActorByName
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..") return true;
    if (rpEnd.GetLabel() == "..") return false;
    bool bGreater = true;
    if (m_bSortAscending) bGreater = false;
    if ( rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];

      switch ( m_iSortMethod )
      {
      case 0:  // Sort by Filename
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      case 1:  // Sort by year
        if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
        if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
        return true;
        break;

      case 2:  // sort by rating
        if ( rpStart.m_fRating < rpEnd.m_fRating) return bGreater;
        if ( rpStart.m_fRating > rpEnd.m_fRating) return !bGreater;
        return true;
        break;

      default:  // Sort by Filename by default
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      }


      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (i = 0; i < (int)strlen(szfilename2); i++)
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      //return (rpStart.strPath.compare( rpEnd.strPath )<0);

      if (m_bSortAscending)
        return (strcmp(szfilename1, szfilename2) < 0);
      else
        return (strcmp(szfilename1, szfilename2) >= 0);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }

  static bool m_bSortAscending;
  static int m_iSortMethod;
};
bool SSortVideoActorByName::m_bSortAscending;
int SSortVideoActorByName::m_iSortMethod;

//****************************************************************************************************************************
CGUIWindowVideoActors::CGUIWindowVideoActors()
{
  m_Directory.m_strPath = "";
}

//****************************************************************************************************************************
CGUIWindowVideoActors::~CGUIWindowVideoActors()
{
}

//****************************************************************************************************************************
bool CGUIWindowVideoActors::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (!m_Directory.IsVirtualDirectoryRoot())
        {
          g_stSettings.m_iMyVideoActorSortMethod++;
          if (g_stSettings.m_iMyVideoActorSortMethod >= 3)
            g_stSettings.m_iMyVideoActorSortMethod = 0;
          g_settings.Save();
        }
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_Directory.IsVirtualDirectoryRoot())
          g_stSettings.m_bMyVideoActorRootSortAscending = !g_stSettings.m_bMyVideoActorRootSortAscending;
        else
          g_stSettings.m_bMyVideoActorSortAscending = !g_stSettings.m_bMyVideoActorSortAscending;

        g_settings.Save();

        UpdateButtons();
        OnSort();
      }
      else
        return CGUIWindowVideoBase::OnMessage(message);
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

//****************************************************************************************************************************
void CGUIWindowVideoActors::FormatItemLabels()
{
  for (int i = 0; i < m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iMyVideoActorSortMethod == 0 || g_stSettings.m_iMyVideoActorSortMethod == 2)
    {
      if (pItem->m_bIsFolder) pItem->SetLabel2("");
      else
      {
        CStdString strRating;
        strRating.Format("%2.2f", pItem->m_fRating);
        pItem->SetLabel2(strRating);
      }
    }
    else
    {
      if (pItem->m_stTime.wYear)
      {
        CStdString strDateTime;
        strDateTime.Format("%i", pItem->m_stTime.wYear);
        pItem->SetLabel2(strDateTime);
      }
      else
        pItem->SetLabel2("");
    }
  }
}

void CGUIWindowVideoActors::SortItems(CFileItemList& items)
{
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    SSortVideoActorByName::m_iSortMethod = g_stSettings.m_iMyVideoActorRootSortMethod;
    SSortVideoActorByName::m_bSortAscending = g_stSettings.m_bMyVideoActorRootSortAscending;
  }
  else
  {
    SSortVideoActorByName::m_iSortMethod = g_stSettings.m_iMyVideoActorSortMethod;
    SSortVideoActorByName::m_bSortAscending = g_stSettings.m_bMyVideoActorSortAscending;
  }
  items.Sort(SSortVideoActorByName::Sort);
}

//****************************************************************************************************************************
void CGUIWindowVideoActors::Update(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems.Size())
  {
    CFileItem* pItem = m_vecItems[iItem];
    if (pItem->GetLabel() != "..")
    {
      strSelectedItem = pItem->m_strPath;
      m_history.Set(strSelectedItem, m_Directory.m_strPath);
    }
  }
  ClearFileItems();
  m_Directory.m_strPath = strDirectory;
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    VECMOVIEACTORS actors;
    m_database.GetActors( actors);
    // Display an error message if the database doesn't contain any actors
    DisplayEmptyDatabaseMessage(actors.empty());
    for (int i = 0; i < (int)actors.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(actors[i]);
      pItem->m_strPath = actors[i];
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
    }
    SET_CONTROL_LABEL(LABEL_ACTOR, "");
  }
  else
  {
    if (!g_guiSettings.GetBool("VideoLists.HideParentDirItems"))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = "";
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;
      m_vecItems.Add(pItem);
    }
    m_strParentPath = "";
    VECMOVIES movies;
    m_database.GetMoviesByActor(m_Directory.m_strPath, movies);
    for (int i = 0; i < (int)movies.size(); ++i)
    {
      CIMDBMovie movie = movies[i];
      CFileItem *pItem = new CFileItem(movie.m_strTitle);
      pItem->m_strPath = movie.m_strSearchString;
      pItem->m_bIsFolder = false;
      pItem->m_bIsShareOrDrive = false;

      CStdString strThumb;
      CUtil::GetVideoThumbnail(movie.m_strIMDBNumber, strThumb);
      if (CFile::Exists(strThumb))
        pItem->SetThumbnailImage(strThumb);
      pItem->m_fRating = movie.m_fRating;
      pItem->m_stTime.wYear = movie.m_iYear;
      m_vecItems.Add(pItem);
    }
    SET_CONTROL_LABEL(LABEL_ACTOR, m_Directory.m_strPath);
  }
  m_vecItems.SetThumbs();
  SetIMDBThumbs(m_vecItems);

  // Fill in default icons
  CStdString strPath;
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    strPath = pItem->m_strPath;
    // Fake a videofile
    pItem->m_strPath = pItem->m_strPath + ".avi";

    pItem->FillInDefaultIcon();
    pItem->m_strPath = strPath;
  }

  OnSort();
  UpdateButtons();
  // update selected item
  strSelectedItem = m_history.Get(m_Directory.m_strPath);
  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (pItem->m_strPath == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }
}

//****************************************************************************************************************************
void CGUIWindowVideoActors::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);

  if (pItem->m_bIsFolder)
  {
    m_iItemSelected = -1;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !g_passwordManager.IsItemUnlocked( pItem, "video" ) )
        return ;

      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    Update(strPath);
  }
  else
  {
    m_iItemSelected = m_viewControl.GetSelectedItem();
    int iSelectedFile = 1;
    VECMOVIESFILES movies;
    m_database.GetFiles(atol(pItem->m_strPath), movies);
    PlayMovies(movies, pItem->m_lStartOffset);
  }
}

void CGUIWindowVideoActors::OnInfo(int iItem)
{
  if ( m_Directory.IsVirtualDirectoryRoot() ) return ;
  CGUIWindowVideoBase::OnInfo(iItem);
}

void CGUIWindowVideoActors::LoadViewMode()
{
  m_iViewAsIconsRoot = g_stSettings.m_iMyVideoActorRootViewAsIcons;
  m_iViewAsIcons = g_stSettings.m_iMyVideoActorViewAsIcons;
}

void CGUIWindowVideoActors::SaveViewMode()
{
  g_stSettings.m_iMyVideoActorRootViewAsIcons = m_iViewAsIconsRoot;
  g_stSettings.m_iMyVideoActorViewAsIcons = m_iViewAsIcons;
  g_settings.Save();
}

int CGUIWindowVideoActors::SortMethod()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_iMyVideoActorRootSortMethod + 365;
  else
    return g_stSettings.m_iMyVideoActorSortMethod + 365;
}

bool CGUIWindowVideoActors::SortAscending()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_bMyVideoActorRootSortAscending;
  else
    return g_stSettings.m_bMyVideoActorSortAscending;
}
