
#include "stdafx.h"
#include "autorun.h"
#include "application.h"
#include "DetectDVDType.h"
#include "sectionloader.h"
#include "util.h"
#include "settings.h"
#include "playlistplayer.h"
#include "texturemanager.h"
#include "GuiUserMessages.h"
#include "guiwindowmanager.h"
#include "stdstring.h"

using namespace PLAYLIST;
using namespace MEDIA_DETECT;

CAutorun::CAutorun()
{
	m_bEnable=true;
}

CAutorun::~CAutorun()
{

}

void CAutorun::ExecuteAutorun()
{
	if ( g_application.IsPlayingAudio() || g_application.IsPlayingVideo() || m_gWindowManager.IsRouted())
		return;

	CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();

	if ( pInfo == NULL )
		return;

	g_application.ResetScreenSaverWindow();		// turn off the screensaver if it's active

	if ( g_stSettings.m_bAutorunCdda && pInfo->IsAudio( 1 ) )
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
	if (g_stSettings.m_bAutorunXbox)
	{
		if ( CUtil::FileExists("D:\\default.xbe") ) 
		{
			g_application.Stop();

			CUtil::LaunchXbe( "Cdrom0", "D:\\default.xbe", NULL );
			return;
		}
	}
	if ( !g_stSettings.m_bAutorunDVD && !g_stSettings.m_bAutorunVCD && !g_stSettings.m_bAutorunVideo && !g_stSettings.m_bAutorunMusic && !g_stSettings.m_bAutorunPictures )
		return;

	int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
	int nAddedToPlaylist = 0;
	CFactoryDirectory factory;
	auto_ptr<CDirectory> pDir ( factory.Create( "D:\\" ) );
	bool bPlaying=RunDisc(pDir.get(), "D:\\",nAddedToPlaylist,true);
	if ( !bPlaying && nAddedToPlaylist > 0 )
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
		m_gWindowManager.SendMessage( msg );
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
		//	Start playing the items we inserted
		g_playlistPlayer.Play( nSize );
	}
}

void CAutorun::RunCdda()
{
	VECFILEITEMS	vecItems;
	CFileItemList itemlist(vecItems);

	if (g_stSettings.m_szExternalCDDAPlayer[0])
	{
		CUtil::RunXBE(g_stSettings.m_szExternalCDDAPlayer);
	}
	else
	{
		CFactoryDirectory factory;
		auto_ptr<CDirectory> pDir ( factory.Create( "cdda://local/" ) );
		if ( !pDir->GetDirectory( "cdda://local/", vecItems ) )
			return;

		if ( vecItems.size() <= 0 )
			return;

		int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();

		for (int i=0; i < (int)vecItems.size(); i++)
		{
			CFileItem* pItem=vecItems[i];
			CPlayList::CPlayListItem playlistItem;
			playlistItem.SetFileName(pItem->m_strPath);
			playlistItem.SetDescription(pItem->GetLabel());
			playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
			g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Add(playlistItem);
		}

		CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
		m_gWindowManager.SendMessage( msg );

		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
		//	Start playing the items we inserted
		g_playlistPlayer.Play(nSize);
	}
}

void CAutorun::RunISOMedia()
{
	if ( !g_stSettings.m_bAutorunDVD && !g_stSettings.m_bAutorunVCD && !g_stSettings.m_bAutorunVideo && !g_stSettings.m_bAutorunMusic && !g_stSettings.m_bAutorunPictures )
		return;

	int nSize = g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size();
	int nAddedToPlaylist = 0;
	CFactoryDirectory factory;
	auto_ptr<CDirectory> pDir ( factory.Create( "iso9660://" ));
	bool bPlaying=RunDisc(pDir.get(), "iso9660://",nAddedToPlaylist,true);
	if ( !bPlaying && nAddedToPlaylist > 0 )
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
		m_gWindowManager.SendMessage( msg );
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
		//	Start playing the items we inserted
		g_playlistPlayer.Play(nSize);
	}
}
bool CAutorun::RunDisc(CDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot)
{
	bool bPlaying(false);
	VECFILEITEMS vecItems;
	CFileItemList itemlist(vecItems);
	char szSlash='\\';
	if (strDrive.Find("iso9660") != -1) szSlash='/';

	if ( !pDir->GetDirectory( strDrive, vecItems ) )
	{
		return false;
	}

  // check root...
  for (int i=0; i < (int)vecItems.size(); i++)
  {
		CFileItem* pItem=vecItems[i];
		if (pItem->m_bIsFolder)
		{
			if (pItem->m_strPath != "." && pItem->m_strPath != ".." )
			{
				if (bRoot&& pItem->m_strPath.Find( "VIDEO_TS" ) != -1 ) 
				{
					if ( g_stSettings.m_bAutorunDVD ) 
					{
            CUtil::PlayDVD();
						bPlaying=true;
						break;
					}
				}
				else if (bRoot && pItem->m_strPath.Find("MPEGAV") != -1 )
				{
					if ( g_stSettings.m_bAutorunVCD ) 
					{
						CFileItem item = *pItem;
						item.m_strPath.Format("%s%cAVSEQ01.DAT",pItem->m_strPath.c_str(),szSlash);
						g_application.PlayFile( item );
						bPlaying=true;
						break;
					}
				}
				else if (bRoot && pItem->m_strPath.Find("MPEG2") != -1 )
				{
					if ( g_stSettings.m_bAutorunVCD ) 
					{
						CFileItem item = *pItem;
						item.m_strPath.Format("%s%cAVSEQ01.MPG",pItem->m_strPath.c_str(),szSlash);
						g_application.PlayFile( item );
						bPlaying=true;
						break;
					}
				}
        else if (bRoot && pItem->m_strPath.Find("PICTURES") != -1 )
				{
          if (g_stSettings.m_bAutorunPictures)
          {
            bPlaying=true;
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
		else
		{
			if ( CUtil::IsVideo( pItem->m_strPath ) && g_stSettings.m_bAutorunVideo)
			{
				bPlaying=true;
				g_application.PlayFile( *pItem );
				break;
			}

			if ( CUtil::IsAudio( pItem->m_strPath ) && g_stSettings.m_bAutorunMusic)
			{
				nAddedToPlaylist++;
				CPlayList::CPlayListItem playlistItem;
				playlistItem.SetFileName(pItem->m_strPath);
				playlistItem.SetDescription(pItem->GetLabel());
				playlistItem.SetDuration( pItem->m_musicInfoTag.GetDuration() );
				g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).Add(playlistItem);
			}
		
			if ( CUtil::IsPicture( pItem->m_strPath ) && g_stSettings.m_bAutorunPictures)
			{
				bPlaying=true;
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
    for (int i=0; i < (int)vecItems.size(); i++)
    {
		  CFileItem* pItem=vecItems[i];
		  if (pItem->m_bIsFolder)
		  {
			  if (pItem->m_strPath != "." && pItem->m_strPath != ".." )
			  {
          if (RunDisc(pDir, pItem->m_strPath, nAddedToPlaylist,false)) 
					{
						bPlaying=true;
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
		return;
	}

	if ( ::WaitForSingleObject( CDetectDVDMedia::m_evAutorun.GetHandle(), 10 ) == WAIT_OBJECT_0 ) 
	{
		ExecuteAutorun();
	}
}

void CAutorun::Enable()
{
	m_bEnable=true;
}

void CAutorun::Disable()
{
	m_bEnable=false;
}

bool CAutorun::IsEnabled()
{
	return m_bEnable;
}
