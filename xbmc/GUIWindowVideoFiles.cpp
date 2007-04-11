#include "stdafx.h"
#include "GUIWindowVideoFiles.h"
#include "Util.h"
#include "Picture.h"
#include "Utils/IMDB.h"
#include "Utils/HTTP.h"
#include "Utils/GUIInfoManager.h"
#include "GUIWindowVideoInfo.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NFOFile.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogContentSettings.h"
#include "GUIDialogVideoScan.h"
#include "FileSystem/StackDirectory.h"
#include "utils/RegExp.h"

using namespace XFILE;
using namespace PLAYLIST;

#define CONTROL_LIST              50
#define CONTROL_THUMBS            51

#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_BTNPLAYLISTS  13

CGUIWindowVideoFiles::CGUIWindowVideoFiles()
: CGUIWindowVideoBase(WINDOW_VIDEO_FILES, "MyVideo.xml")
{
}

CGUIWindowVideoFiles::~CGUIWindowVideoFiles()
{
}

bool CGUIWindowVideoFiles::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    OutputDebugString("activate guiwindowvideoplaylist!\n");
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return true;
  }

  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        g_stSettings.m_iVideoStartWindow = GetID();
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
        // reset directory path, as we have effectively cleared it here
        m_history.ClearPathHistory();
      }

      // is this the first time accessing this window?
      // a quickpath overrides the a default parameter
      if (m_vecItems.m_strPath == "?" && strDestination.IsEmpty())
      {
        m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultVideos;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open root
        if (strDestination.Equals("$ROOT"))
        {
          m_vecItems.m_strPath = "";
          CLog::Log(LOGINFO, "  Success! Opening root listing.");
        }
        // open playlists location
        else if (strDestination.Equals("$PLAYLISTS"))
        {
          m_vecItems.m_strPath = "special://videoplaylists/";
          CLog::Log(LOGINFO, "  Success! Opening destination path: %s", m_vecItems.m_strPath.c_str());
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems.m_strPath = "";

          bool bIsBookmarkName = false;

          SetupShares();
          VECSHARES shares;
          m_rootDir.GetShares(shares);
          int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsBookmarkName);
          if (iIndex > -1)
          {
            bool bDoStuff = true;
            if (shares[iIndex].m_iHasLock == 2)
            {
              CFileItem item(shares[iIndex]);
              if (!g_passwordManager.IsItemUnlocked(&item,"video"))
              {
                m_vecItems.m_strPath = ""; // no u don't
                bDoStuff = false;
                CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
              }
            }
            // set current directory to matching share
            if (bDoStuff)
            {
              if (bIsBookmarkName)
                m_vecItems.m_strPath=shares[iIndex].strPath;
              else
                m_vecItems.m_strPath=strDestination;
              CUtil::RemoveSlashAtEnd(m_vecItems.m_strPath);
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
            }
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
          }
        }

        SetHistoryForPath(m_vecItems.m_strPath);
      }

      return CGUIWindowVideoBase::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      //if (iControl == CONTROL_BTNSCAN)
      //{
      //  OnScan();
     // }
      /*else*/ if (iControl == CONTROL_STACK)
      {
        // toggle between the following states:
        //   0 : no stacking
        //   1 : stacking
        g_stSettings.m_iMyVideoStack++;

        if (g_stSettings.m_iMyVideoStack > STACK_SIMPLE)
          g_stSettings.m_iMyVideoStack = STACK_NONE;

        if (g_stSettings.m_iMyVideoStack != STACK_NONE)
          g_stSettings.m_bMyVideoCleanTitles = true;
        else
          g_stSettings.m_bMyVideoCleanTitles = false;
        g_settings.Save();
        UpdateButtons();
        Update( m_vecItems.m_strPath );
      }
      else if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems.m_strPath.Equals("special://videoplaylists/"))
        {
          CStdString strParent = m_vecItems.m_strPath;
          UpdateButtons();
          Update("special://videoplaylists/");
        }
      }
      // list/thumb panel
      else if (m_viewControl.HasControl(iControl))
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        const CFileItem* pItem = m_vecItems[iItem];

        // use play button to add folders of items to temp playlist
        if (iAction == ACTION_PLAYER_PLAY && pItem->m_bIsFolder && !pItem->IsParentFolder())
        {
          if (pItem->IsDVD())
            return CAutorun::PlayDisc();

          if (pItem->m_bIsShareOrDrive)
            return false;
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingVideo())
          {
            if (g_application.m_pPlayer->IsPaused()) return false;
            if (g_application.GetPlaySpeed() != 1) return false;
          }
          // not playing, or playback speed == 1
          // queue folder or playlist into temp playlist and play
          if ((pItem->m_bIsFolder && !pItem->IsParentFolder()) || pItem->IsPlayList())
            PlayItem(iItem);
          return true;
        }
      }
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}


void CGUIWindowVideoFiles::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  const CGUIControl *stack = GetControl(CONTROL_STACK);
  if (stack)
  {
    if ((g_stSettings.m_iMyVideoStack & STACK_UNAVAILABLE) != STACK_UNAVAILABLE)
    {
      CONTROL_ENABLE(CONTROL_STACK)
      if (stack->GetControlType() == CGUIControl::GUICONTROL_RADIO)
      {
        SET_CONTROL_SELECTED(GetID(), CONTROL_STACK, g_stSettings.m_iMyVideoStack == STACK_SIMPLE);
        SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
      }
      else
      {
        SET_CONTROL_LABEL(CONTROL_STACK, g_stSettings.m_iMyVideoStack + 14000);
      }
    }
    else
    {
      if (stack->GetControlType() == CGUIControl::GUICONTROL_RADIO)
      {
        SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
      }

      CONTROL_DISABLE(CONTROL_STACK)
    }
  }
}

bool CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowVideoBase::GetDirectory(strDirectory, items))
    return false;

  SScraperInfo info2;

  g_stSettings.m_iMyVideoStack &= ~STACK_UNAVAILABLE;
  g_infoManager.m_content = "files";

  if (m_database.GetScraperForPath(strDirectory,info2.strPath,info2.strContent) && info2.strContent.Equals("tvshows"))
  { // dont stack in tv dirs
    g_stSettings.m_iMyVideoStack |= STACK_UNAVAILABLE;
  }
  else if (!items.IsStack() && g_stSettings.m_iMyVideoStack != STACK_NONE)
    items.Stack();

  items.SetThumbnailImage("");
  items.SetVideoThumb();

  return true;
}

void CGUIWindowVideoFiles::OnPrepareFileItems(CFileItemList &items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);
  if (g_stSettings.m_bMyVideoCleanTitles && ((g_stSettings.m_iMyVideoStack & STACK_UNAVAILABLE) != STACK_UNAVAILABLE))
    items.CleanFileNames();
}

bool CGUIWindowVideoFiles::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);

  if ( strcmpi(strExtension.c_str(), ".nfo") == 0) // WTF??
  {
    SScraperInfo info;
    info.strPath = "imdb.xml";
    info.strContent = "movies";
    info.strTitle = "IMDb";
    OnInfo(iItem,info);
    return true;
  }

  return CGUIWindowVideoBase::OnClick(iItem);
}

bool CGUIWindowVideoFiles::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];

  if (pItem->IsDVD())
    return CAutorun::PlayDisc();

  if (pItem->m_bIsShareOrDrive)
  	return false;

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("video"))
    {
      Update("");
      return true;
    }
    return false;
  }
  else
  {
    AddFileToDatabase(pItem);

    return CGUIWindowVideoBase::OnPlayMedia(iItem);
  }
}

void CGUIWindowVideoFiles::OnInfo(int iItem, const SScraperInfo& info)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  bool bFolder(false);
  if (info.strContent.Equals("tvshows"))
  {
    CGUIWindowVideoBase::OnInfo(iItem,info);
    return;
  }
  CStdString strFolder = "";
  int iSelectedItem = m_viewControl.GetSelectedItem();
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strFile = pItem->m_strPath;
  if (pItem->m_bIsFolder && pItem->IsParentFolder()) return ;
  if (pItem->m_bIsShareOrDrive) // oh no u don't
    return ;
  if (pItem->m_bIsFolder)
  {
    // IMDB is done on a folder
    // stack and then find first file in folder
    bFolder = true;
    CFileItemList vecitems;
    GetStackedDirectory(pItem->m_strPath, vecitems);
    bool bFoundFile(false);
    for (int i = 0; i < (int)vecitems.Size(); ++i)
    {
      CFileItem *item = vecitems[i];
      if (!item->m_bIsFolder)
      {
        if (item->IsVideo() && !item->IsNFO() && !item->IsPlayList() )
        {
          bFoundFile = true;
          strFile = item->m_strPath;
          break;
        }
      }
      else
      { // check for a dvdfolder
        if (item->GetLabel().CompareNoCase("VIDEO_TS") == 0)
        { // found a dvd folder - grab the main .ifo file
          CUtil::AddFileToFolder(item->m_strPath, "VIDEO_TS.IFO", strFile);
          if (CFile::Exists(strFile))
          {
            bFoundFile = true;
            break;
          }
        }
        // check for a "CD1" folder
        if (item->GetLabel().CompareNoCase("CD1") == 0)
        {
          CFileItemList items;
          GetStackedDirectory(item->m_strPath, items);
          for (int i = 0; i < items.Size(); i++)
          {
            CFileItem *item = items[i];
            if (!item->m_bIsFolder && item->IsVideo() && !item->IsNFO() && !item->IsPlayList() )
            {
              bFoundFile = true;
              strFile = item->m_strPath;
              break;
            }
          }
          if (bFoundFile)
            break;
        }
      }
    }
    if (!bFoundFile)
    {
      // no video file in this folder?
      if (info.strContent.Equals("movies"))
        CGUIDialogOK::ShowAndGetInput(13346,20349,20022,20022);

      m_viewControl.SetSelectedItem(iSelectedItem);
      return ;
    }
  }

  // setup our item with the label and thumb information
  CFileItem item(strFile, false);
  item.SetLabel(pItem->GetLabel());

  // hack since our label sometimes contains extensions
  if(!pItem->m_bIsFolder && !g_guiSettings.GetBool("filelists.hideextensions") && !pItem->IsLabelPreformated())
    item.RemoveExtension();
  
  item.SetCachedVideoThumb();
  if (!item.HasThumbnail()) // inherit from the original item if it exists
    item.SetThumbnailImage(pItem->GetThumbnailImage());

  AddFileToDatabase(&item);

  ShowIMDB(&item,info);
  // apply any IMDb icon to our item
  if (pItem->m_bIsFolder)
    ApplyIMDBThumbToFolder(pItem->m_strPath, item.GetThumbnailImage());
  Update(m_vecItems.m_strPath);
}

void CGUIWindowVideoFiles::AddFileToDatabase(const CFileItem* pItem)
{
  if (!pItem->IsVideo()) return ;
  if ( pItem->IsNFO()) return ;
  if ( pItem->IsPlayList()) return ;

  /* subtitles are assumed not to exist here, if we need it  */
  /* player should update this when it figures out if it has */
  m_database.AddFile(pItem->m_strPath);

  if (pItem->IsStack())
  { // get stacked items
    // TODO: This should be removed as soon as we no longer need the individual
    // files for saving settings etc.
    vector<CStdString> movies;
    GetStackedFiles(pItem->m_strPath, movies);
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      CFileItem item(movies[i], false);
      m_database.AddFile(item.m_strPath);
    }
  }
}

void CGUIWindowVideoFiles::OnScan(const CStdString& strPath, const SScraperInfo& info, int iDirNames, int iScanRecursively)
{
  // GetStackedDirectory() now sets and restores the stack state!
  SScanSettings settings = {};

  if(iDirNames>0)
  {
    settings.parent_name = true;
    settings.recurse = 1; /* atleast one, otherwise this makes no sence */
  }
  else if (info.strContent.Equals("movies") && iDirNames == -1)
  {
    bool bCanceled;
    if (!CGUIDialogYesNo::ShowAndGetInput(13346,20332,-1,-1,20334,20331,bCanceled))
    {
      settings.parent_name = true;
      settings.recurse = 1; /* atleast one, otherwise this makes no sence */
    }

    if (bCanceled)
      return;
  }

  if(iScanRecursively > 0)
    settings.recurse = INT_MAX;
  else if (iScanRecursively == -1 && info.strContent.Equals("movies"))
  {
    bool bCanceled;
    if( CGUIDialogYesNo::ShowAndGetInput(13346,20335,-1,-1,bCanceled) )
      settings.recurse = INT_MAX;

    if (bCanceled)
      return;
  }

  if (info.strContent.Equals("tvshows"))
  {
    settings.recurse = 1;
    settings.parent_name = true;
    settings.parent_name_root = true;
  }

  CGUIDialogVideoScan* pDialog = (CGUIDialogVideoScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
  if (pDialog)
    pDialog->StartScanning(strPath,info,settings,false);
}

void CGUIWindowVideoFiles::OnUnAssignContent(int iItem)
{
  bool bCanceled;
  if (CGUIDialogYesNo::ShowAndGetInput(20338,20340,20341,20022,bCanceled))
  {
    m_database.RemoveContentForPath(m_vecItems[iItem]->m_strPath);
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
  else
  {
    if (!bCanceled)
    {
      m_database.SetScraperForPath(m_vecItems[iItem]->m_strPath,"","");
    }
  }
}

void CGUIWindowVideoFiles::OnAssignContent(int iItem, int iFound, SScraperInfo& info)
{
  bool bScan=false, bScanRecursive=true, bUseDirNames=false;
  if (iFound == 0)
  {
    m_database.GetScraperForPath(m_vecItems[iItem]->m_strPath,info.strPath,info.strContent);
  }
  SScraperInfo info2 = info;
  
  if (CGUIDialogContentSettings::Show(info2,bScan,bScanRecursive,bUseDirNames))
  {
    if (info2.strContent.IsEmpty())
    {
      if (!info.strContent.IsEmpty())
        OnUnAssignContent(iItem);
      return;
    }

    m_database.Open();
    m_database.SetScraperForPath(m_vecItems[iItem]->m_strPath,info2.strPath,info2.strContent);
    m_database.Close();
    
    if (bScan)
      OnScan(m_vecItems[iItem]->m_strPath,info2,bUseDirNames?1:0,bScanRecursive?1:0);
  }
}

void CGUIWindowVideoFiles::GetStackedDirectory(const CStdString &strPath, CFileItemList &items)
{
  items.Clear();
  m_rootDir.GetDirectory(strPath, items);

  // force stacking to be enabled
  // save stack state
  int iStack = g_stSettings.m_iMyVideoStack;
  g_stSettings.m_iMyVideoStack = STACK_SIMPLE;

  /*bool bUnroll = m_guiState->UnrollArchives();
  g_guiSettings.SetBool("filelists.unrollarchives",true);*/

  items.Stack();

  // restore stack
  g_stSettings.m_iMyVideoStack = iStack;
  //g_guiSettings.SetBool("VideoFiles.FileLists",bUnroll);
}

void CGUIWindowVideoFiles::LoadPlayList(const CStdString& strPlayList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_VIDEO))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
    // activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow() && iSize > 1)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
  }
}

void CGUIWindowVideoFiles::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  // calculate our position
  float posX = 200;
  float posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // TODO: We should add the option here for shares to be added if there aren't any
      return ;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("video", m_vecItems[iItem], posX, posY))
    {
      Update(m_vecItems.m_strPath);
      return ;
    }
    m_vecItems[iItem]->Select(false);
    return ;
  }
  CGUIWindowVideoBase::OnPopupMenu(iItem, bContextDriven);
}

void CGUIWindowVideoFiles::OnQueueItem(int iItem)
{
  CGUIWindowVideoBase::OnQueueItem(iItem);
}
