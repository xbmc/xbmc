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
	//	Reads the tracks from an audio cd and looks for cddb information on the internet

	if (!CDetectDVDMedia::IsDiscInDrive()) 
		return false;

	//	Prepare cddb
	Xcddb cddb;
	CStdString strDir;
	strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
	cddb.setCacheDir(strDir);
	//cddb.setCDDBIpAdress(g_stSettings.m_szCDDBIpAdres);

	CGUIDialogProgress* pDialogProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
	CGUIDialogOK* pDialogOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
	CGUIDialogSelect *pDlgSelect= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);

  if (!pDialogProgress) return false;
  if (!pDialogOK) return false;
  if (!pDlgSelect) return false;

	//	Get information for the inserted disc
	CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
	if (pCdInfo==NULL)
		return false;

	//	If the disc has no tracks, we are finished here.
	int nTracks = pCdInfo->GetTrackCount();
	if (nTracks <= 0)
		return false;

	bool bCddbInfoLoaded=false;

	//	Do we have to look for cddb information
	if (pCdInfo->HasCDDBInfo() && g_stSettings.m_bUseCDDB)
	{
		bool bCloseProgress(false);
		//	Show progress dialog if we have to connect to freedb.org
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
		
		//	get cddb information
		if ( !cddb.queryCDinfo( pCdInfo ) )
		{
			//	error getting cddb information
			if (bCloseProgress)
			{
				pDialogProgress->Close();
			}
			int lasterror=cddb.getLastError();

			//	Have we found more then on match in cddb for this disc,...
			if (lasterror == E_WAIT_FOR_INPUT)
			{
				//	...yes, show the matches found in a select dialog
				//	and let the user choose an entry.
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

				//	Has the user selected a match...
				int iSelectedCD= pDlgSelect->GetSelectedLabel();
				if (iSelectedCD>= 0) 
				{
					//	...query cddb for the inexact match
					if ( cddb.queryCDinfo(1+iSelectedCD))
					{
						//	cddb info loaded
						bCddbInfoLoaded=true;
					}
					else
						pCdInfo->SetNoCDDBInfo();
				}
				else
					pCdInfo->SetNoCDDBInfo();
			}
			else 
			{
				pCdInfo->SetNoCDDBInfo();
				//	..no, an error occured, display it to the user
				pDialogOK->SetHeading(255);
				pDialogOK->SetLine(0,257);//ERROR
				pDialogOK->SetLine(1,cddb.getLastErrorText() );
				pDialogOK->SetLine(2,"");
				pDialogOK->DoModal(m_gWindowManager.GetActiveWindow() );
			}
		}	//	if ( !cddb.queryCDinfo( pCdInfo ) )
		else
		{
			// We got cddb information for this disc
			if (bCloseProgress)
			{
				pDialogProgress->Close();
			}
			bCddbInfoLoaded=true;
		}
	} // if (pCdInfo->HasCDDBInfo() && g_stSettings.m_bUseCDDB)


	//	Generate fileitems
	for ( int i = 0; i < nTracks; i++ )
	{
		//	Skip Datatracks for display, 
		//	but needed to query cddb
		if ( !pCdInfo->IsAudio( i + 1 ) )
			continue;

		//	Format standard cdda item label
		CStdString strLabel;
		strLabel.Format("Track %02.2i", i+1);

		CFileItem* pItem = new CFileItem(strLabel);
		pItem->m_bIsFolder=false;

		CStdString strPath;
		strPath.Format("cdda://local/%i.cdda",i);
		pItem->m_strPath=strPath;

		//	Tracknumber and duration is always available
		pItem->m_musicInfoTag.SetTrackNumber(i+1);
		pItem->m_musicInfoTag.SetDuration( ( pCdInfo->GetTrackInformation( i+1 ).nMins * 60 ) 
			+ pCdInfo->GetTrackInformation( i+1 ).nSecs );

		//	Fill the fileitems music tag with cddb information, if available
		CStdString strTitle=cddb.getTrackTitle(i+1);
		if (bCddbInfoLoaded && strTitle.size() > 0)
		{
			//	Title
			pItem->m_musicInfoTag.SetTitle(strTitle);

			//	Artist: Use track artist or disc artist
			CStdString strArtist=cddb.getTrackArtist(i+1);
			if (strArtist.IsEmpty())
				cddb.getDiskArtist(strArtist);
			pItem->m_musicInfoTag.SetArtist(strArtist);

			// Album
			CStdString strAlbum;
			cddb.getDiskTitle( strAlbum );
			pItem->m_musicInfoTag.SetAlbum(strAlbum);
			
			//	Year
			SYSTEMTIME dateTime;
			dateTime.wYear=atoi(cddb.getYear().c_str());
			pItem->m_musicInfoTag.SetReleaseDate( dateTime );

			//	Genre
			pItem->m_musicInfoTag.SetGenre( cddb.getGenre() );

			pItem->m_musicInfoTag.SetLoaded(true);
		}

		items.push_back(pItem);
	}
	
	return true;
}
