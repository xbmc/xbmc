
#include "stdafx.h"
#include "CDDADirectory.h"
#include "../DetectDVDType.h"
#include "../MusicDatabase.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace MEDIA_DETECT;

CCDDADirectory::CCDDADirectory(void)
{
}

CCDDADirectory::~CCDDADirectory(void)
{
}


bool CCDDADirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // Reads the tracks from an audio cd

  // This class can only provide .cdda files
  if (!IsAllowed(".cdda"))
    return true;

  if (!CDetectDVDMedia::IsDiscInDrive())
    return false;

  // Get information for the inserted disc
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (pCdInfo == NULL)
    return false;

  //  Preload CDDB info
  CMusicDatabase musicdatabase;
  musicdatabase.LookupCDDBInfo();

  // If the disc has no tracks, we are finished here.
  int nTracks = pCdInfo->GetTrackCount();
  if (nTracks <= 0)
    return false;

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
    if (CFile::Stat(pItem->m_strPath, &s64) == 0)
      pItem->m_dwSize = s64.st_size;

    items.Add(pItem);
  }
  return true;
}
