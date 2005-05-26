
#include "../stdafx.h"
#include "cddadirectory.h"
#include "../util.h"
#include "../xbox/iosupport.h"
#include "../application.h"
#include "cddb.h"

using namespace CDDB;

CCDDADirectory::CCDDADirectory(void)
{
}

CCDDADirectory::~CCDDADirectory(void)
{
}


bool CCDDADirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // Reads the tracks from an audio cd and looks for cddb information on the internet

  if (!CDetectDVDMedia::IsDiscInDrive())
    return false;

  // Prepare cddb
  Xcddb cddb;
  CStdString strDir;
  strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
  cddb.setCacheDir(strDir);

  // Get information for the inserted disc
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (pCdInfo == NULL)
    return false;

  // If the disc has no tracks, we are finished here.
  int nTracks = pCdInfo->GetTrackCount();
  if (nTracks <= 0)
    return false;

  // Do we have to look for cddb information
  if (pCdInfo->HasCDDBInfo() && g_guiSettings.GetBool("MyMusic.UseCDDB") && !cddb.isCDCached(pCdInfo))
  {
    CGUIDialogProgress* pDialogProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    CGUIDialogOK* pDialogOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);

    if (!pDialogProgress) return false;
    if (!pDialogOK) return false;
    if (!pDlgSelect) return false;

    // Show progress dialog if we have to connect to freedb.org
    pDialogProgress->SetHeading(255); //CDDB
    pDialogProgress->SetLine(0, ""); // Querying freedb for CDDB info
    pDialogProgress->SetLine(1, 256);
    pDialogProgress->SetLine(2, "");
    pDialogProgress->StartModal(m_gWindowManager.GetActiveWindow());
    pDialogProgress->Progress();

    // get cddb information
    if (!cddb.queryCDinfo(pCdInfo))
    {
      pDialogProgress->Close();
      int lasterror = cddb.getLastError();

      // Have we found more then on match in cddb for this disc,...
      if (lasterror == E_WAIT_FOR_INPUT)
      {
        // ...yes, show the matches found in a select dialog
        // and let the user choose an entry.
        pDlgSelect->Reset();
        pDlgSelect->SetHeading(255);
        int i = 1;
        while (1)
        {
          CStdString strTitle = cddb.getInexactTitle(i);
          if (strTitle == "") break;

          CStdString strArtist = cddb.getInexactArtist(i);
          if (!strArtist.IsEmpty())
            strTitle += " - " + strArtist;

          pDlgSelect->Add(strTitle);
          i++;
        }
        pDlgSelect->DoModal(m_gWindowManager.GetActiveWindow());

        // Has the user selected a match...
        int iSelectedCD = pDlgSelect->GetSelectedLabel();
        if (iSelectedCD >= 0)
        {
          // ...query cddb for the inexact match
          if (!cddb.queryCDinfo(pCdInfo, 1 + iSelectedCD))
            pCdInfo->SetNoCDDBInfo();
        }
        else
          pCdInfo->SetNoCDDBInfo();

        pDialogProgress->Close();
      }
      else
      {
        pCdInfo->SetNoCDDBInfo();
        pDialogProgress->Close();
        // ..no, an error occured, display it to the user
        pDialogOK->SetHeading(255);
        pDialogOK->SetLine(0, 257); //ERROR
        pDialogOK->SetLine(1, cddb.getLastErrorText() );
        pDialogOK->SetLine(2, "");
        pDialogOK->DoModal(m_gWindowManager.GetActiveWindow() );
      }
    } // if ( !cddb.queryCDinfo( pCdInfo ) )
    pDialogProgress->Close();
  } // if (pCdInfo->HasCDDBInfo() && g_stSettings.m_bUseCDDB)

  // Filling the file items with cddb info happens in CMusicInfoTagLoaderCDDA

  // Generate fileitems
  for (int i = 1;i <= nTracks;++i)
  {
    // Skip Datatracks for display,
    // but needed to query cddb
    if (!pCdInfo->IsAudio(i))
      continue;

    // Format standard cdda item label
    CStdString strLabel;
    strLabel.Format("Track %02.2i", i);

    CFileItem* pItem = new CFileItem(strLabel);
    pItem->m_bIsFolder = false;
    pItem->m_strPath.Format("cdda://local/%02.2i.cdda", i);

    __stat64 s64;
    CFile file;
    if (file.Stat(pItem->m_strPath, &s64) == 0)
      pItem->m_dwSize = s64.st_size;

    items.Add(pItem);
  }
  return true;
}
