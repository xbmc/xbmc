
#include "stdafx.h"
#include "autorun.h"
#include "application.h"
#include "DetectDVDType.h"
#include "util.h"
#include "playlistplayer.h"

using namespace PLAYLIST;
using namespace MEDIA_DETECT;

CAutorun::CAutorun()
{
  m_bEnable = true;
}

CAutorun::~CAutorun()
{
}

void CAutorun::ExecuteAutorun()
{
  if ( g_application.IsPlayingAudio() || g_application.IsPlayingVideo() || m_gWindowManager.IsRouted())
    return ;

  CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();

  if ( pInfo == NULL )
    return ;

  g_application.ResetScreenSaverWindow();  // turn off the screensaver if it's active

  if ( g_guiSettings.GetBool("Autorun.CDDA") && pInfo->IsAudio( 1 ) )
  {
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

void CAutorun::RunXboxCd()
{
  if (g_guiSettings.GetBool("Autorun.Xbox"))
  {
    if ( CUtil::FileExists("D:\\default.xbe") )
    {
      g_application.Stop();

      CUtil::LaunchXbe( "Cdrom0", "D:\\default.xbe", NULL );
      return ;
    }
  }
  if ( !g_guiSettings.GetBool("Autorun.DVD") && !g_guiSettings.GetBool("Autorun.VCD") && !g_guiSettings.GetBool("Autorun.Video") && !g_guiSettings.GetBool("Autorun.Music") && !g_guiSettings.GetBool("Autorun.Pictures") )
    return ;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;
  CFactoryDirectory factory;
  auto_ptr<IDirectory> pDir ( factory.Create( "D:\\" ) );
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

  if (g_stSettings.m_szExternalCDDAPlayer[0])
  {
    CUtil::RunXBE(g_stSettings.m_szExternalCDDAPlayer);
  }
  else
  {
    CFactoryDirectory factory;
    auto_ptr<IDirectory> pDir ( factory.Create( "cdda://local/" ) );
    if ( !pDir->GetDirectory( "cdda://local/", vecItems ) )
      return ;

    if ( vecItems.Size() <= 0 )
      return ;

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
    // Start playing the items we inserted
    g_playlistPlayer.Play(nSize);
  }
}

void CAutorun::RunISOMedia()
{
  if ( !g_guiSettings.GetBool("Autorun.DVD") && !g_guiSettings.GetBool("Autorun.VCD") && !g_guiSettings.GetBool("Autorun.Video") && !g_guiSettings.GetBool("Autorun.Music") && !g_guiSettings.GetBool("Autorun.Pictures") )
    return ;

  int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
  int nAddedToPlaylist = 0;
  CFactoryDirectory factory;
  auto_ptr<IDirectory> pDir ( factory.Create( "iso9660://" ));
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
bool CAutorun::RunDisc(IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot)
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

  // check root...
  for (int i = 0; i < vecItems.Size(); i++)
  {
    CFileItem* pItem = vecItems[i];
    if (pItem->m_bIsFolder)
    {
      if (pItem->m_strPath != "." && pItem->m_strPath != ".." )
      {
        if (bRoot && pItem->m_strPath.Find( "VIDEO_TS" ) != -1 )
        {
          if ( g_guiSettings.GetBool("Autorun.DVD") )
          {
            CUtil::PlayDVD();
            bPlaying = true;
            break;
          }
        }
        else if (bRoot && pItem->m_strPath.Find("MPEGAV") != -1 )
        {
          if ( g_guiSettings.GetBool("Autorun.VCD") )
          {
            CFileItemList items;
            CDirectory::GetDirectory(pItem->m_strPath, items, ".dat");
            if (items.Size())
            {
              CUtil::SortFileItemsByName(items);
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
            }
            break;
          }
        }
        else if (bRoot && pItem->m_strPath.Find("MPEG2") != -1 )
        {
          if ( g_guiSettings.GetBool("Autorun.VCD") )
          {
            CFileItemList items;
            CDirectory::GetDirectory(pItem->m_strPath, items, ".mpg");
            if (items.Size())
            {
              CUtil::SortFileItemsByName(items);
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
            }
            break;
          }
        }
        else if (bRoot && pItem->m_strPath.Find("PICTURES") != -1 )
        {
          if (g_guiSettings.GetBool("Autorun.Pictures"))
          {
            bPlaying = true;
            m_gWindowManager.ActivateWindow(WINDOW_PICTURES);
            CStdString* strUrl = new CStdString( pItem->m_strPath.c_str() );
            CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, 0, 0, (void*) strUrl );
            m_gWindowManager.SendMessage( msg );
            delete strUrl;
            break;
          }
        }
      }
    }
  }
  // check video first
  if (!nAddedToPlaylist && !bPlaying && g_guiSettings.GetBool("Autorun.Video"))
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItem *pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsVideo())
      {
        bPlaying = true;
        g_application.PlayFile( *pItem );
        break;
      }
    }
  }
  // then music
  if (!bPlaying && g_guiSettings.GetBool("Autorun.Music"))
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItem *pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsAudio())
      {
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
  if (!nAddedToPlaylist && !bPlaying && g_guiSettings.GetBool("Autorun.Pictures"))
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItem *pItem = vecItems[i];
      if (!pItem->m_bIsFolder && pItem->IsPicture())
      {
        bPlaying = true;
        m_gWindowManager.ActivateWindow(WINDOW_PICTURES);
        CStdString* strUrl = new CStdString( strDrive );
        CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, 0, 0, (void*) strUrl );
        m_gWindowManager.SendMessage( msg );
        delete strUrl;
        break;
      }
    }
  }

  // check subdirs
  if (!bPlaying)
  {
    for (int i = 0; i < vecItems.Size(); i++)
    {
      CFileItem* pItem = vecItems[i];
      if (pItem->m_bIsFolder)
      {
        if (pItem->m_strPath != "." && pItem->m_strPath != ".." )
        {
          if (RunDisc(pDir, pItem->m_strPath, nAddedToPlaylist, false))
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
