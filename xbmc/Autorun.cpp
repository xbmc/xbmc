#include "autorun.h"
#include "application.h"
#include "DetectDVDType.h"
#include "sectionloader.h"
#include "util.h"
#include "filesystem/factoryDirectory.h"
#include "settings.h"
#include "playlistplayer.h"
#include "GuiUserMessages.h"
#include "guiwindowmanager.h"
#include "stdstring.h"

using namespace PLAYLIST;

CAutorun::CAutorun()
{

}

CAutorun::~CAutorun()
{

}

void CAutorun::ExecuteAutorun()
{
	if ( g_application.IsPlayingAudio() || g_application.IsPlayingVideo() )
		return;

	CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();

	if ( pInfo == NULL )
		return;

	if ( g_stSettings.m_bAutorunCdda && pInfo->IsAudio( 1 ) )
	{
		RunCdda();
	}
	else if (  pInfo->IsUDFX( 1 ) )
	{
		RunXboxCd();
	}
	else if ( pInfo->IsIso9660( 1 ) || pInfo->IsIso9660Interactive( 1 ) ) 
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
			m_gWindowManager.DeInitialize();
			CSectionLoader::UnloadAll();

			g_application.Stop();

			CUtil::LaunchXbe( "Cdrom0", "D:\\default.xbe", NULL );
			return;
		}
	}
	if ( !g_stSettings.m_bAutorunDVD && !g_stSettings.m_bAutorunVCD && !g_stSettings.m_bAutorunVideo && !g_stSettings.m_bAutorunMusic && !g_stSettings.m_bAutorunPictures )
		return;
	VECFILEITEMS	vecItems;

	int nAddedToPlaylist = 0;
	bool bVideoFound = false;

	CFactoryDirectory factory;
	CDirectory* pDir = factory.Create( "D:\\" );
	if ( !pDir->GetDirectory( "D:\\", vecItems ) )
		return;

	for (int i=0; i < (int)vecItems.size(); i++)
	{
		CFileItem* pItem=vecItems[i];
		if ( g_stSettings.m_bAutorunDVD ) 
		{
			if ( pItem->m_bIsFolder && pItem->m_strPath.Find( "VIDEO_TS" ) != -1 ) 
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( "D:\\VIDEO_TS\\VIDEO_TS.IFO" );
				break;
			}
		}
		if ( g_stSettings.m_bAutorunVCD ) 
		{
			if ( pItem->m_bIsFolder && pItem->m_strPath == "MPEGAV" )
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( "D:\\MPEGAV\\AVSEQ01.DAT" );
				break;
			}
			if ( pItem->m_bIsFolder && pItem->m_strPath == "MPEG2" )
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( "D:\\MPEG2\\AVSEQ01.MPG" );
				break;
			}
		}

		if (g_stSettings.m_bAutorunVideo)
		{
			if ( !pItem->m_bIsFolder && CUtil::IsVideo( pItem->m_strPath ) )
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( pItem->m_strPath );
				bVideoFound = true;
				break;
			}
		}

		if (g_stSettings.m_bAutorunMusic)
		{
			if ( !pItem->m_bIsFolder && CUtil::IsAudio( pItem->m_strPath ) )
			{
				nAddedToPlaylist++;
				CPlayList::CPlayListItem playlistItem;
				playlistItem.SetFileName(pItem->m_strPath);
				playlistItem.SetDescription(pItem->GetLabel());
				playlistItem.SetDuration( pItem->m_musicInfoTag.GetDuration() );
				g_playlistPlayer.Add(playlistItem);
			}
		}

		if (g_stSettings.m_bAutorunPictures)
		{
			if ( !pItem->m_bIsFolder && CUtil::IsPicture( pItem->m_strPath ) )
			{
				m_gWindowManager.ActivateWindow( 2 );
				CStdString* strUrl = new CStdString( "D:\\" );
				CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, 0, 0, (void*) strUrl );
				m_gWindowManager.SendMessage( msg );
				delete strUrl;
				break;
			}
		}
	}

	if ( !bVideoFound && nAddedToPlaylist > 0 )
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
		m_gWindowManager.SendMessage( msg );

		int nSize = g_playlistPlayer.size();
		g_playlistPlayer.Play( nSize - nAddedToPlaylist );
	}

	// cleanup
	for (int i=0; i < (int)vecItems.size(); i++)
	{
		CFileItem* pItem=vecItems[i];
		delete pItem;
	}

}

void CAutorun::RunCdda()
{
	VECFILEITEMS	vecItems;

	CFactoryDirectory factory;
	CDirectory* pDir = factory.Create( "cdda://local/" );
	if ( !pDir->GetDirectory( "cdda://local/", vecItems ) )
		return;

	if ( vecItems.size() <= 0 )
		return;

	int nSize = g_playlistPlayer.size();

	for (int i=0; i < (int)vecItems.size(); i++)
	{
		CFileItem* pItem=vecItems[i];
		CPlayList::CPlayListItem playlistItem;
		playlistItem.SetFileName(pItem->m_strPath);
		playlistItem.SetDescription(pItem->GetLabel());
		playlistItem.SetDuration( pItem->m_musicInfoTag.GetDuration() );
		g_playlistPlayer.Add(playlistItem);
	}

	CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
	m_gWindowManager.SendMessage( msg );

	g_playlistPlayer.Play( nSize );

	// cleanup
	for (int i=0; i < (int)vecItems.size(); i++)
	{
		CFileItem* pItem=vecItems[i];
		delete pItem;
	}

}

void CAutorun::RunISOMedia()
{
	if ( !g_stSettings.m_bAutorunDVD && !g_stSettings.m_bAutorunVCD && !g_stSettings.m_bAutorunVideo && !g_stSettings.m_bAutorunMusic && !g_stSettings.m_bAutorunPictures )
		return;

	VECFILEITEMS	vecItems;

	int nAddedToPlaylist = 0;
	bool bVideoFound = false;

	CFactoryDirectory factory;
	CDirectory* pDir = factory.Create( "iso9660://" );
	if ( !pDir->GetDirectory( "iso9660://", vecItems ) )
		return;

  for (int i=0; i < (int)vecItems.size(); i++)
  {
		CFileItem* pItem=vecItems[i];
		if ( g_stSettings.m_bAutorunDVD ) 
		{
			if ( pItem->m_bIsFolder && pItem->m_strPath.Find( "VIDEO_TS" ) != -1 ) 
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( "iso9660://VIDEO_TS/VIDEO_TS.IFO" );
				break;
			}
		}
		if ( g_stSettings.m_bAutorunVCD ) 
		{
			if ( pItem->m_bIsFolder && pItem->m_strPath == "MPEGAV" )
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( "iso9660://MPEGAV/AVSEQ01.DAT" );
				break;
			}
			if ( pItem->m_bIsFolder && pItem->m_strPath == "MPEG2" )
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( "iso9660://MPEG2/AVSEQ01.MPG" );
				break;
			}
		}

		if (g_stSettings.m_bAutorunVideo)
		{
			if ( !pItem->m_bIsFolder && CUtil::IsVideo( pItem->m_strPath ) )
			{
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( pItem->m_strPath );
				bVideoFound = true;
				break;
			}
		}

		if (g_stSettings.m_bAutorunMusic)
		{
			if ( !pItem->m_bIsFolder && CUtil::IsAudio( pItem->m_strPath ) )
			{
				nAddedToPlaylist++;
				CPlayList::CPlayListItem playlistItem;
				playlistItem.SetFileName(pItem->m_strPath);
				playlistItem.SetDescription(pItem->GetLabel());
				playlistItem.SetDuration( pItem->m_musicInfoTag.GetDuration() );
				g_playlistPlayer.Add(playlistItem);
			}
		}

		if (g_stSettings.m_bAutorunPictures)
		{
			if ( !pItem->m_bIsFolder && CUtil::IsPicture( pItem->m_strPath ) )
			{
				m_gWindowManager.ActivateWindow( 2 );
				CStdString* strUrl = new CStdString( "iso9660://" );
				CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, 0, 0, (void*) strUrl );
				m_gWindowManager.SendMessage( msg );
				delete strUrl;
				break;
			}
		}
  }

	if ( !bVideoFound && nAddedToPlaylist > 0 )
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
		m_gWindowManager.SendMessage( msg );

		int nSize = g_playlistPlayer.size();
		g_playlistPlayer.Play( nSize - nAddedToPlaylist );
	}
	// cleanup
	for (int i=0; i < (int)vecItems.size(); i++)
	{
		CFileItem* pItem=vecItems[i];
		delete pItem;
	}
}

void CAutorun::HandleAutorun()
{
	if ( ::WaitForSingleObject( CDetectDVDMedia::m_evAutorun.GetHandle(), 10 ) == WAIT_OBJECT_0 ) 
	{
		ExecuteAutorun();
	}
}
