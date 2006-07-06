#include "stdafx.h"
#include "autorun.h"
#include "application.h"
#include "DetectDVDType.h"
#include "util.h"
#include "guipassword.h"
#include "playlistplayer.h"
#include "xbox/xbeheader.h"
#include "FileSystem/StackDirectory.h"
#include "utils/kaiclient.h"
#include "programdatabase.h"
#include "utils/trainer.h"

using namespace PLAYLIST;
using namespace MEDIA_DETECT;

CAutorun::CAutorun()
{
  m_bEnable = true;
}

CAutorun::~CAutorun()
{}
void CAutorun::ExecuteAutorun()
{
  if ( g_application.IsPlayingAudio() || g_application.IsPlayingVideo() || m_gWindowManager.IsRouted())
    return ;

  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();

  if ( pInfo == NULL )
    return ;

  g_application.ResetScreenSaverWindow();  // turn off the screensaver if it's active

  if ( g_guiSettings.GetBool("autorun.cdda") && pInfo->IsAudio( 1 ) )
  {
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].musicLocked())
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return ;

    RunCdda();
  }
  else if (pInfo->IsUDFX( 1 ) || pInfo->IsUDF(1))
  {
    RunXboxCd();
  }
  else if (pInfo->IsISOUDF(1) || pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1))
  {
    RunISOMedia();
  }
  else
  {
    RunXboxCd();
  }
}

void CAutorun::ExecuteXBE(const CStdString &xbeFile)
{
  int iRegion;
  if (g_guiSettings.GetBool("myprograms.gameautoregion"))
  {
    CXBE xbe;
    iRegion = xbe.ExtractGameRegion(xbeFile);
    if (iRegion < 1 || iRegion > 7)
      iRegion = 0;
    iRegion = xbe.FilterRegion(iRegion);
  }
  else
    iRegion = 0;

  CProgramDatabase database;
  database.Open();
  DWORD dwTitleId = CUtil::GetXbeID(xbeFile);
  CStdString strTrainer = database.GetActiveTrainer(dwTitleId);
  if (strTrainer != "")
  {
      bool bContinue=false;
      if (CKaiClient::GetInstance()->IsEngineConnected())
      {
        CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
        pDialog->SetHeading(714);
        pDialog->SetLine(0,"Use trainer or KAI?");
        pDialog->SetLine(1, "Yes for trainer");
        pDialog->SetLine(2, "No for KAI");
        pDialog->DoModal();
        if (pDialog->IsConfirmed())
        {
          while (CKaiClient::GetInstance()->GetCurrentVector().size() > 1)
            CKaiClient::GetInstance()->ExitVector();
        }
        else
          bContinue = true;
      }
      if (!bContinue)
      {
        CTrainer trainer;
        if (trainer.Load(strTrainer))
        {
          database.GetTrainerOptions(strTrainer,dwTitleId,trainer.GetOptions(),trainer.GetNumberOfOptions());
          CUtil::InstallTrainer(trainer);
        }
      }
  }

  database.Close();
  CUtil::RunXBE(xbeFile.c_str(), NULL,F_VIDEO(iRegion));
}

void CAutorun::RunXboxCd()
{
  if ( CFile::Exists("D:\\default.xbe") )
  {
    if (!g_guiSettings.GetBool("autorun.xbox"))
      return;
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked())
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return;

    ExecuteXBE("D:\\default.xbe");
    return;
  }

  if ( !g_guiSettings.GetBool("autorun.dvd") && !g_guiSettings.GetBool("autorun.vcd") && !g_guiSettings.GetBool("autorun.video") && !g_guiSettings.GetBool("autorun.music") && !g_guiSettings.GetBool("autorun.pictures") )
    return ;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;
  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "D:\\" ) );
  bool bPlaying = RunDisc(pDir.get(), "D:\\", nAddedToPlaylist, true);
  if ( !bPlaying && nAddedToPlaylist > 0 )
  {
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
    m_gWindowManager.SendMessage( msg );
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    // Start playing the items we inserted
    g_playlistPlayer.Play( nSize );
  }
}

void CAutorun::RunCdda()
{
  CFileItemList vecItems;

  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "cdda://local/" ) );
  if ( !pDir->GetDirectory( "cdda://local/", vecItems ) )
    return ;

  if ( vecItems.Size() <= 0 )
    return ;

  //int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();

  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  for (int i = 0; i < vecItems.Size(); i++)
  {
    CFileItem* pItem = vecItems[i];
    CPlayList::CPlayListItem playlistItem;
    playlistItem.SetFileName(pItem->m_strPath);
    playlistItem.SetDescription(pItem->GetLabel());
    playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
    g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Add(playlistItem);
  }

  //CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
  //m_gWindowManager.SendMessage( msg );

  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.Play();
}

void CAutorun::RunISOMedia()
{
  if ( !g_guiSettings.GetBool("autorun.dvd") && !g_guiSettings.GetBool("autorun.vcd") && !g_guiSettings.GetBool("autorun.video") && !g_guiSettings.GetBool("autorun.music") && !g_guiSettings.GetBool("autorun.pictures") )
    return ;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;
  auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "iso9660://" ));
  bool bPlaying = RunDisc(pDir.get(), "iso9660://", nAddedToPlaylist, true);
  if ( !bPlaying && nAddedToPlaylist > 0 )
  {
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
    m_gWindowManager.SendMessage( msg );
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    // Start playing the items we inserted
    g_playlistPlayer.Play(nSize);
  }
}
bool CAutorun::RunDisc(IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings /* = false */)
{
  bool bPlaying(false);
  CFileItemList vecItems;
  CFileItemList itemlist(vecItems);
  char szSlash = '\\';
  if (strDrive.Find("iso9660") != -1) szSlash = '/';

  if ( !pDir->GetDirectory( strDrive, vecItems ) )
  {
    return false;
  }

  bool bAllowVideo = true;
  bool bAllowPictures = true;
  bool bAllowMusic = true;
  if (!g_passwordManager.IsMasterLockUnlocked(true))
  {
    if( g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].videoLocked() )
      bAllowVideo = false;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].picturesLocked())
      bAllowPictures = false;

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].musicLocked())
      bAllowMusic = false;
  }

  if( bRoot )
  {
    // check root folders first, for normal structured dvd's
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItem* pItem = vecItems[i];

      if (pItem->m_bIsFolder && pItem->m_strPath != "." && pItem->m_strPath != "..")
      {
        if (pItem->m_strPath.Find( "VIDEO_TS" ) != -1 && bAllowVideo
        && (bypassSettings || g_guiSettings.GetBool("autorun.dvd")))
        {
          CUtil::PlayDVD();
          bPlaying = true;
          return true;
        }
        else if (pItem->m_strPath.Find("MPEGAV") != -1 && bAllowVideo 
             && (bypassSettings || g_guiSettings.GetBool("autorun.vcd")))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->m_strPath, items, ".dat");
          if (items.Size())
          {
            g_playlistPlayer.ClearPlaylist( PLAYLIST_VIDEO_TEMP );
            items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
            for (int i=0; i<items.Size(); ++i)
            {
              CFileItem* pItem=items[i];
              CPlayList::CPlayListItem playlistItem;
              CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
              g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO_TEMP ).Add(playlistItem);
            }

            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
            g_playlistPlayer.Play(0);
            bPlaying = true;
            return true;
          }
        }
        else if (pItem->m_strPath.Find("MPEG2") != -1 && bAllowVideo
              && (bypassSettings || g_guiSettings.GetBool("autorun.vcd")))
        {
          CFileItemList items;
          CDirectory::GetDirectory(pItem->m_strPath, items, ".mpg");
          if (items.Size())
          {
            g_playlistPlayer.ClearPlaylist( PLAYLIST_VIDEO_TEMP );
            items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
            for (int i=0; i<items.Size(); ++i)
            {
              CFileItem* pItem=items[i];
              CPlayList::CPlayListItem playlistItem;
              CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
              g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO_TEMP ).Add(playlistItem);
            }

            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
            g_playlistPlayer.Play(0);
            bPlaying = true;
            return true;
          }          
        }
        else if (pItem->m_strPath.Find("PICTURES") != -1 && bAllowPictures
              && (bypassSettings || g_guiSettings.GetBool("autorun.pictures")))
        {
          bPlaying = true;
          CStdString strExec;
          strExec.Format("XBMC.RecursiveSlideShow(%s)", pItem->m_strPath.c_str());
          CUtil::ExecBuiltIn(strExec);
          return true;
        }
      }
    }
  }

  // check for special case stuff next
  for (int i = 0; i < vecItems.Size(); i++)
  {
    // check video first
    if (!nAddedToPlaylist && !bPlaying && (bypassSettings || g_guiSettings.GetBool("autorun.video")) && bAllowVideo)
    {
      // stack video files
      CFileItemList tempItems;
      tempItems.Append(vecItems);
      tempItems.Stack();
      for (int i = 0; i < tempItems.Size(); i++)
      {
        CFileItem *pItem = tempItems[i];
        if (!pItem->m_bIsFolder && pItem->IsVideo())
        {
          bPlaying = true;
          if (pItem->IsStack())
          {
            // TODO: remove this once the app/player is capable of handling stacks immediately
            g_playlistPlayer.ClearPlaylist( PLAYLIST_VIDEO_TEMP );
            CStackDirectory dir;
            CFileItemList items;
            dir.GetDirectory(pItem->m_strPath, items);
            for (int i = 0; i < items.Size(); i++)
            {
              CPlayList::CPlayListItem playlistItem;
              CUtil::ConvertFileItemToPlayListItem(items[i], playlistItem);
              g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO_TEMP ).Add(playlistItem);
            }
            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
            g_playlistPlayer.Play(0);
          }
          else
            g_application.PlayMedia(*pItem, PLAYLIST_VIDEO_TEMP);
          break;
        }
      }
    }
    // then music
    if (!bPlaying && (bypassSettings || g_guiSettings.GetBool("autorun.music")) && bAllowMusic)
    {
      for (int i = 0; i < vecItems.Size(); i++)
      {
        CFileItem *pItem = vecItems[i];
        if (!pItem->m_bIsFolder && pItem->IsAudio())
        {
          bPlaying = true;
          nAddedToPlaylist++;
          CPlayList::CPlayListItem playlistItem;
          playlistItem.SetFileName(pItem->m_strPath);
          playlistItem.SetDescription(pItem->GetLabel());
          playlistItem.SetDuration( pItem->m_musicInfoTag.GetDuration() );
          g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).Add(playlistItem);
        }
      }
    }
    // and finally pictures
    if (!nAddedToPlaylist && !bPlaying && (bypassSettings || g_guiSettings.GetBool("autorun.pictures")) && bAllowPictures)
    {
      for (int i = 0; i < vecItems.Size(); i++)
      {
        CFileItem *pItem = vecItems[i];
        if (!pItem->m_bIsFolder && pItem->IsPicture())
        {
          bPlaying = true;
          CStdString strExec;
          strExec.Format("XBMC.RecursiveSlideShow(%s)", strDrive.c_str());
          CUtil::ExecBuiltIn(strExec);
          break;
        }
      }
    }
  }

  // check subdirs if we are not playing yet
  if (!bPlaying)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItem* pItem = vecItems[i];
      if (pItem->m_bIsFolder)
      {
        if (pItem->m_strPath != "." && pItem->m_strPath != ".." )
        {
          if (RunDisc(pDir, pItem->m_strPath, nAddedToPlaylist, false, bypassSettings))
          {
            bPlaying = true;
            break;
          }
        }
      }
    }
  }

  return bPlaying;
}

void CAutorun::HandleAutorun()
{
  if (!m_bEnable)
  {
    CDetectDVDMedia::m_evAutorun.Reset();
    return ;
  }

  if ( ::WaitForSingleObject( CDetectDVDMedia::m_evAutorun.GetHandle(), 10 ) == WAIT_OBJECT_0 )
  {
    ExecuteAutorun();
  }
}

void CAutorun::Enable()
{
  m_bEnable = true;
}

void CAutorun::Disable()
{
  m_bEnable = false;
}

bool CAutorun::IsEnabled()
{
  return m_bEnable;
}
// GeminiServer: PlayDisc()
// Will Play, if a disc is inserted! Has nothing todo with Autorun, therefore no autorun settings will be used!
// Can be executed from the Context menu!
bool CAutorun::PlayDisc()
{
  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
  if ( pInfo == NULL )
    return false ;
  if ( pInfo->IsAudio( 1 ) )
  {
    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].musicLocked())
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;

    CLog::Log(LOGDEBUG, "Playdisc called on an audio disk");
    CFileItemList vecItems;

    auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "cdda://local/" ) );
    if ( !pDir->GetDirectory( "cdda://local/", vecItems ) ) return false;
    if ( vecItems.Size() <= 0 ) return false;
    int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItem* pItem = vecItems[i];
      CPlayList::CPlayListItem playlistItem;
      playlistItem.SetFileName(pItem->m_strPath);
      playlistItem.SetDescription(pItem->GetLabel());
      playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
      g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Add(playlistItem);
    }
    CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
    m_gWindowManager.SendMessage( msg );

    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Play(nSize);
  }
  else 
  {
    if (!pInfo->IsISOHFS(1) && !pInfo->IsIso9660(1) && !pInfo->IsIso9660Interactive(1))
    { // pInfo->IsISOHFS(1) || pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1)
      CLog::Log(LOGDEBUG, "Playdisc called on an UDFX/UDF/ISOUDFn or Unknown disk, attempting to run XBE");
      if ( CFile::Exists("D:\\default.xbe") )
      {
        if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].programsLocked())
          if (!g_passwordManager.IsMasterLockUnlocked(true))
            return false;

        ExecuteXBE("D:\\default.xbe");
        return false;
      }
    }
    CLog::Log(LOGDEBUG, "Playdisc called on an ISOHFS/iso9660/iso9660Interactive disk, or no xbe found, searching for music files");
    int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
    int nAddedToPlaylist = 0;
    auto_ptr<IDirectory> pDir ( CFactoryDirectory::Create( "D:\\" ) );
    bool bPlaying = RunDisc(pDir.get(), "D:\\", nAddedToPlaylist, true, true);
    if ( !bPlaying && nAddedToPlaylist > 0 )
    {
      CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
      m_gWindowManager.SendMessage( msg );
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
      // Start playing the items we inserted
      g_playlistPlayer.Play( nSize );
    }
  }
  return true;
}