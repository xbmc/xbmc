#include "cddadirectory.h"
#include "../url.h"
#include "../util.h"
#include "../sectionloader.h"
#include "../lib/libCDRip/cdripxlib.h"
#include "../settings.h"
#include "../xbox/iosupport.h"
#include "cddb.h"

using namespace CDDB;

CCDDADirectory::CCDDADirectory(void)
{
//CSectionLoader::Load("LIBCDRIP");
}

CCDDADirectory::~CCDDADirectory(void)
{
//CSectionLoader::Unload("LIBCDRIP");
}


bool  CCDDADirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
	
	{

    CIoSupport helper;
    helper.Remount("D:","Cdrom0");
		CCDRipX cdr;
		
		Xcddb cddb;
		CStdString strDir;
		strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
		cddb.setCDDBIpAdress(g_stSettings.m_szCDDBIpAdres);
		cddb.setCacheDir(strDir);
		bool b_cddb_names_ok(false);

		cdr.Init();
		int nTracks = cdr.GetNumTocEntries();
		if (nTracks > 0)
		{
			toc cdtoc[100];
			for (int i=0;i<=(nTracks);i++)
			{
				// stupid but it works
				cdtoc[i].min   = cdr.oCDCon[i].min;
				cdtoc[i].sec   = cdr.oCDCon[i].sec;
				cdtoc[i].frame = cdr.oCDCon[i].frame;
			}

			if (g_stSettings.m_bUseCDDB)
			{
				if ( !cddb.isCDCached( nTracks, cdtoc ) ) 
				{
						//g_dialog.DoModalLess();
						//g_dialog.SetCaption(0, "CDDB" );
						//g_dialog.SetMessage(0, "Quering CD at " );
						//g_dialog.SetMessage(1, g_playerSettings.szCddbServer );
						//g_dialog.Render();
				}
				if ( !cddb.queryCDinfo((nTracks), cdtoc) )
				{
					int lasterror=cddb.getLastError();

					if (lasterror == E_WAIT_FOR_INPUT)
					{
						//	Inexact match found in cddb
						//	How to handle?
					}
					else 
					{
						//show error message 1.5 seconds
						//g_dialog.SetCaption(0, "Error" );
						//g_dialog.SetMessage(0, cddb.getLastErrorText() );
						//g_dialog.Render();
						//Sleep(1500);
						return false;
					}
				}
				else
				{
					// Jep, tracks are received
					b_cddb_names_ok=true;
				}
			} // if (g_stSettings.m_bUseCDDB)
		
	
			for ( int i = 0; i < nTracks; i++ )
			{
				//	Skip Datatracks for display, 
				//	but needed to query cddb
				if ( !cdr.IsAudioTrack( i ) )
					continue;
				CStdString strArtist=cddb.getTrackArtist(i+1);
				CStdString strTitle=cddb.getTrackTitle(i+1);
				CStdString strPath;
				strPath.Format("cdda://local/%i.cdda",i);
				if (b_cddb_names_ok && strArtist.size() > 0 && strTitle.size() > 0)
				{
					
					CFileItem* pItem = new CFileItem(strTitle);
					pItem->m_strPath=strPath;
					pItem->m_bIsFolder=false;
					pItem->m_musicInfoTag.SetTitle(strTitle);
					pItem->m_musicInfoTag.SetArtist(strArtist);
					pItem->m_musicInfoTag.SetTrackNumber(i+1);
					pItem->m_musicInfoTag.SetLoaded(true);
					items.push_back(pItem);
				}
				else 
				{
					CStdString strLabel;
					strLabel.Format("Track %02.2i", i+1);
					CFileItem* pItem = new CFileItem(strLabel);
					pItem->m_bIsFolder=false;
					pItem->m_strPath=strPath;
					items.push_back(pItem);
				}
			}
		}//if (nTracks>0)
	}
	
	return true;
}
