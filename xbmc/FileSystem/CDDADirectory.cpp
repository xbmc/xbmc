#include "cddadirectory.h"
#include "../url.h"
#include "../util.h"
#include "../sectionloader.h"
#include "../lib/libCDRip/cdripxlib.h"
#include "../settings.h"
#include "../xbox/iosupport.h"
#include "../application.h"
#include "cddb.h"
#include "../GUIDialogProgress.h"
#include "../guidialogok.h"
#include "../GUIDialogSelect.h"
using namespace CDDB;

CCDDADirectory::CCDDADirectory(void)
{

}

CCDDADirectory::~CCDDADirectory(void)
{

}


bool  CCDDADirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
	if (!g_application.m_DetectDVDType.IsDiscInDrive()) return false;
	{
		Xcddb cddb;
		CStdString strDir;
		strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
		cddb.setCDDBIpAdress(g_stSettings.m_szCDDBIpAdres);
		cddb.setCacheDir(strDir);
		bool b_cddb_names_ok(false);
		CGUIDialogProgress* pDialogProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(101);
		CGUIDialogOK* pDialogOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
		CGUIDialogSelect *pDlgSelect= (CGUIDialogSelect*)m_gWindowManager.GetWindow(2000);

		int nTracks = CDetectDVDMedia::GetCdInfo()->GetTrackCount();
		CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
		if ( nTracks > 0)
		{
			if (g_stSettings.m_bUseCDDB)
			{
				bool bCloseProgress(false);
				if ( !cddb.isCDCached( pCdInfo ) ) 
				{
					pDialogProgress->SetHeading(255);//CDDB
					pDialogProgress->SetLine(0,"");// Querying freedb for CDDB info
					pDialogProgress->SetLine(1,256);
					pDialogProgress->SetLine(2,"");
					pDialogProgress->StartModal(m_gWindowManager.GetActiveWindow());
					pDialogProgress->Progress();
					bCloseProgress=true;
				}
				
				if ( !cddb.queryCDinfo( pCdInfo ) )
				{
					if (bCloseProgress)
					{
						pDialogProgress->Close();
					}
					int lasterror=cddb.getLastError();

					if (lasterror == E_WAIT_FOR_INPUT)
					{
						//	Inexact match found in cddb
						//	How to handle?
						pDlgSelect->Reset();
						pDlgSelect->SetHeading(255);
						int i=1;
						while (1)
						{
							string strTitle=cddb.getInexactTitle(i++) ;
							if (strTitle =="") break;
							pDlgSelect->Add(strTitle);
						}
						pDlgSelect->DoModal(m_gWindowManager.GetActiveWindow());
						int iSelectedCD= pDlgSelect->GetSelectedLabel();
						if (iSelectedCD>= 0) 
						{
							if ( cddb.queryCDinfo(1+iSelectedCD))
							{
								b_cddb_names_ok=true;
							}
						}
					}
					else 
					{
						//show error message 1.5 seconds
						pDialogOK->SetHeading(255);
						pDialogOK->SetLine(0,257);//ERROR
						pDialogOK->SetLine(1,cddb.getLastErrorText() );
						pDialogOK->SetLine(2,"");
						pDialogOK->DoModal(m_gWindowManager.GetActiveWindow() );
					}
				}
				else
				{
					// Jep, tracks are received
					if (bCloseProgress)
					{
						pDialogProgress->Close();
					}
					b_cddb_names_ok=true;
				}
			} // if (g_stSettings.m_bUseCDDB)
		
	
			for ( int i = 0; i < nTracks; i++ )
			{
				//	Skip Datatracks for display, 
				//	but needed to query cddb
				if ( !pCdInfo->IsAudio( i + 1 ) )
					continue;
				CStdString strTitle=cddb.getTrackTitle(i+1);
				CStdString strPath;
				strPath.Format("cdda://local/%i.cdda",i);
				if (b_cddb_names_ok && strTitle.size() > 0)
				{
					CStdString strLabel;
					CStdString strArtist=cddb.getTrackArtist(i+1);
					if ( strArtist.IsEmpty() )
						cddb.getDiskArtist(strArtist);

					if ( !strArtist.IsEmpty() )
						strLabel.Format("%02.2i. %s - %s", i+1, strArtist.c_str(), strTitle.c_str() );
					else
						strLabel.Format("%02.2i. %s", i+1, strTitle.c_str() );

					CFileItem* pItem = new CFileItem(strLabel);
					pItem->m_strPath=strPath;
					pItem->m_bIsFolder=false;
					pItem->m_musicInfoTag.SetTitle(strTitle);
					pItem->m_musicInfoTag.SetArtist(strArtist);
					CStdString strAlbum;
					cddb.getDiskTitle( strAlbum );
					pItem->m_musicInfoTag.SetAlbum(strAlbum);
					pItem->m_musicInfoTag.SetTrackNumber(i+1);
					pItem->m_musicInfoTag.SetLoaded(true);
					pItem->m_musicInfoTag.SetDuration( ( pCdInfo->GetTrackInformation( i+1 ).nMins * 60 ) 
						+ pCdInfo->GetTrackInformation( i+1 ).nSecs );
					
					SYSTEMTIME dateTime;
					dateTime.wYear=atoi(cddb.getYear().c_str());
					pItem->m_musicInfoTag.SetReleaseDate( dateTime );
					pItem->m_musicInfoTag.SetGenre( cddb.getGenre() );
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
					pItem->m_musicInfoTag.SetDuration( ( pCdInfo->GetTrackInformation( i+1 ).nMins * 60 ) 
						+ pCdInfo->GetTrackInformation( i+1 ).nSecs );
				}
			}
		}//if (nTracks>0)
	}
	
	return true;
}
