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
	else if (  pInfo->IsUDFX( 1 ) || pInfo->IsUDF(1) )
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

	int nAddedToPlaylist = 0;
	CFactoryDirectory factory;
	CDirectory* pDir = factory.Create( "D:\\" );
	bool bPlaying=RunDisc(pDir, "D:\\",nAddedToPlaylist,true);
	if ( !bPlaying && nAddedToPlaylist > 0 )
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
		m_gWindowManager.SendMessage( msg );
		g_playlistPlayer.Play( 0 );
	}

	delete pDir;
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
	CFactoryDirectory factory;
	CDirectory* pDir = factory.Create( "iso9660://" );
	bool bPlaying=RunDisc(pDir, "iso9660://",nAddedToPlaylist,true);
	if ( !bPlaying && nAddedToPlaylist > 0 )
	{
		CGUIMessage msg( GUI_MSG_PLAYLIST_CHANGED, 0, 0, 0, 0, NULL );
		m_gWindowManager.SendMessage( msg );
		g_playlistPlayer.Play( 0 );
	}

	delete pDir;
}
bool CAutorun::RunDisc(CDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot)
{
	bool bPlaying(false);
	VECFILEITEMS vecItems;
	char szSlash='\\';
	if (strDrive.Find("iso9660") != -1) szSlash='/';

	if ( !pDir->GetDirectory( strDrive, vecItems ) )
	{
		return false;
	}

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
						CStdString strFileName;
						strFileName.Format("%s%cVIDEO_TS.IFO",pItem->m_strPath.c_str(),szSlash);
						g_TextureManager.Flush();
						g_graphicsContext.SetFullScreenVideo(true);
						g_application.PlayFile( strFileName );
						bPlaying=true;
						break;
					}
				}
				else if (bRoot && pItem->m_strPath.Find("MPEGAV") != -1 )
				{
					if ( g_stSettings.m_bAutorunVCD ) 
					{
						CStdString strFileName;
						strFileName.Format("%s%cAVSEQ01.DAT",pItem->m_strPath.c_str(),szSlash);
						g_TextureManager.Flush();
						g_graphicsContext.SetFullScreenVideo(true);
						g_application.PlayFile( strFileName );
						bPlaying=true;
						break;
					}
				}
				else
				{
					if (RunDisc(pDir, pItem->m_strPath, nAddedToPlaylist,false)) 
					{
						bPlaying=true;
						break;
					}
				}
			}
		}
		else
		{
			if ( bRoot && CUtil::IsVideo( pItem->m_strPath ) && g_stSettings.m_bAutorunVideo)
			{
				bPlaying=true;
				g_graphicsContext.SetFullScreenVideo(true);
				g_application.PlayFile( pItem->m_strPath );
				break;
			}

			if ( CUtil::IsAudio( pItem->m_strPath ) && g_stSettings.m_bAutorunMusic)
			{
				nAddedToPlaylist++;
				CPlayList::CPlayListItem playlistItem;
				playlistItem.SetFileName(pItem->m_strPath);
				playlistItem.SetDescription(pItem->GetLabel());
				playlistItem.SetDuration( pItem->m_musicInfoTag.GetDuration() );
				g_playlistPlayer.Add(playlistItem);
			}
		
			if ( CUtil::IsPicture( pItem->m_strPath ) && g_stSettings.m_bAutorunPictures)
			{
				bPlaying=true;
				m_gWindowManager.ActivateWindow( 2 );
				CStdString* strUrl = new CStdString( strDrive );
				CGUIMessage msg( GUI_MSG_START_SLIDESHOW, 0, 0, 0, 0, (void*) strUrl );
				m_gWindowManager.SendMessage( msg );
				delete strUrl;
				break;
			}
		}
  }
/*
*/
	// cleanup
	for (int i=0; i < (int)vecItems.size(); i++)
	{
		CFileItem* pItem=vecItems[i];
		delete pItem;
	}
	return bPlaying;
}

void CAutorun::HandleAutorun()
{
	if ( ::WaitForSingleObject( CDetectDVDMedia::m_evAutorun.GetHandle(), 10 ) == WAIT_OBJECT_0 ) 
	{
		ExecuteAutorun();
	}
}
