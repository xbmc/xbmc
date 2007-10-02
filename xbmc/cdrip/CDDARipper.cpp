#include "stdafx.h"
#include "CDDARipper.h"
#include "CDDAReader.h"
#include "../Util.h"
#include "EncoderLame.h"
#include "EncoderWav.h"
#include "EncoderVorbis.h"
#include "../FileSystem/CDDADirectory.h"
#include "../DetectDVDType.h"
#include "../musicInfoTagLoaderFactory.h"
#include "../utils/LabelFormatter.h"

using namespace XFILE;

CCDDARipper::CCDDARipper()
{
  m_pEncoder = NULL;
}

CCDDARipper::~CCDDARipper()
{
  if (m_pEncoder) delete m_pEncoder;
}

bool CCDDARipper::Init(const CStdString& strTrackFile, const CStdString& strFile, MUSIC_INFO::CMusicInfoTag* infoTag)
{
  m_cdReader.Init(strTrackFile);

  switch (g_guiSettings.GetInt("cddaripper.encoder"))
  {
  case CDDARIP_ENCODER_WAV:
    m_pEncoder = new CEncoderWav();
    break;
  case CDDARIP_ENCODER_VORBIS:
    m_pEncoder = new CEncoderVorbis();
    break;
  default:
    m_pEncoder = new CEncoderLame();
    break;
  }

  // we have to set the tags before we init the Encoder
  if (infoTag)
  {
    CStdString strTrack;
    strTrack.Format("%i", atoi(strTrackFile.substr(13, strTrackFile.size() - 13 - 5).c_str()));

    m_pEncoder->SetComment("Ripped with XBMC");
    m_pEncoder->SetArtist(infoTag->GetArtist().c_str());
    m_pEncoder->SetTitle(infoTag->GetTitle().c_str());
    m_pEncoder->SetAlbum(infoTag->GetAlbum().c_str());
    m_pEncoder->SetGenre(infoTag->GetGenre().c_str());
    m_pEncoder->SetTrack(strTrack.c_str());
    m_pEncoder->SetYear(infoTag->GetYearString().c_str());
  }

  // init encoder
  CStdString strFile2=strFile;
  if (CUtil::IsHD(strFile))
    CUtil::GetFatXQualifiedPath(strFile2);
  if (!m_pEncoder->Init(strFile2.c_str(), 2, 44100, 16))
  {
    m_cdReader.DeInit();
    delete m_pEncoder;
    m_pEncoder = NULL;
    return false;
  }
  return true;
}

bool CCDDARipper::DeInit()
{
  // Close the encoder
  m_pEncoder->Close();

  m_cdReader.DeInit();

  if (m_pEncoder) delete m_pEncoder;
  m_pEncoder = NULL;

  return true;
}

int CCDDARipper::RipChunk(int& nPercent)
{
  BYTE* pbtStream = NULL;
  long lBytesRead = 0;

  // get data
  int iResult = m_cdReader.GetData(&pbtStream, lBytesRead);

  // return if rip is done or on some kind of error
  if (iResult != CDDARIP_OK) return iResult;

  // encode data
  m_pEncoder->Encode(lBytesRead, pbtStream);

  // Get progress indication
  nPercent = m_cdReader.GetPercent();

  return CDDARIP_OK;
}

// rip a single track from cd to hd
// strFileName has to be a valid filename and the directory must exist
bool CCDDARipper::Rip(const CStdString& strTrackFile, const CStdString& strFile, MUSIC_INFO::CMusicInfoTag& infoTag)
{
  int iPercent, iOldPercent = 0;
  bool bCancelled = false;
  const char* strFilename = strFile.c_str();

  CLog::Log(LOGINFO, "Start ripping track %s to %s", strTrackFile.c_str(), strFile.c_str());

  // if we are ripping to a samba share, rip it to hd first and then copy it it the share
  CFileItem file(strFile, false);
#ifdef _LINUX
  char tmp[128];
  strcpy(tmp,"/tmp/xbmc/abcdXXXXXX");
  if (mkstemp(tmp) == -1)
    return false;
  strFilename = tmp;
#else
  if (file.IsRemote()) strFilename = tempnam("Z:\\", "");
#endif
  if (!strFilename)
  {
    CLog::Log(LOGERROR, "CCDDARipper: Error opening file");
    return false;
  }

  // init ripper
  if (!Init(strTrackFile, strFilename, &infoTag))
  {
    CLog::Log(LOGERROR, "Error: CCDDARipper::Init failed");
    return false;
  }

  // setup the progress dialog
  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CStdString strLine0, strLine1;
  int iTrack = atoi(strTrackFile.substr(13, strTrackFile.size() - 13 - 5).c_str());
  strLine0.Format("%s %i", g_localizeStrings.Get(606).c_str(), iTrack); // Track Number: %i
  strLine1.Format("%s %s", g_localizeStrings.Get(607).c_str(), strFile); // To: %s
  pDlgProgress->SetHeading(605); // Ripping
  pDlgProgress->SetLine(0, strLine0);
  pDlgProgress->SetLine(1, strLine1);
  pDlgProgress->SetLine(2, "");
  pDlgProgress->StartModal();
  pDlgProgress->ShowProgressBar(true);

  // show progress dialog
  g_graphicsContext.Lock();
  pDlgProgress->Progress();
  g_graphicsContext.Unlock();

  // start ripping
  while (!bCancelled && CDDARIP_DONE != RipChunk(iPercent))
  {
    pDlgProgress->ProgressKeys();
    bCancelled = pDlgProgress->IsCanceled();
    if (!bCancelled && iPercent > (iOldPercent + 2)) // update each 2%, it's a bit faster then every 1%
    {
      // update dialog
      iOldPercent = iPercent;
      pDlgProgress->SetPercentage(iPercent);
      pDlgProgress->Progress();
    }
  }

  // close dialog and deinit ripper
  pDlgProgress->Close();
  DeInit();

  if (file.IsRemote() && !bCancelled)
  {
    // copy the ripped track to the share
    if (!CFile::Cache(strFilename, strFile.c_str()))
    {
      CLog::Log(LOGINFO, "Error copying file from %s to %s", strFilename, strFile.c_str());
      // show error
      g_graphicsContext.Lock();
      CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      pDlgOK->SetHeading("Error copying");
      pDlgOK->SetLine(0, CStdString(strFilename) + " to");
      pDlgOK->SetLine(1, strFile);
      pDlgOK->SetLine(2, "");
      pDlgOK->DoModal();
      g_graphicsContext.Unlock();
      CFile::Delete(strFilename);
      return false;
    }
    // delete cached file
    CFile::Delete(strFilename);
  }

  if (bCancelled) CLog::Log(LOGWARNING, "User Cancelled CDDA Rip");
  else CLog::Log(LOGINFO, "Finished ripping %s", strTrackFile.c_str());
  return !bCancelled;
}

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
  int iTrack = 0;
  CStdString strDirectory = g_guiSettings.GetString("cddaripper.path");
  if (!CUtil::HasSlashAtEnd(strDirectory)) CUtil::AddDirectorySeperator(strDirectory);
  CFileItem ripPath(strDirectory, true);
  bool bIsFATX = !ripPath.IsSmb();

  if (pItem->m_strPath.Find(".cdda") < 0) return false;
  if (strDirectory.size() < 3)
  {
    // no rip path has been set, show error
    CLog::Log(LOGERROR, "Error: CDDARipPath has not been set");
    g_graphicsContext.Lock();
    CGUIDialogOK::ShowAndGetInput(257, 608, 609, 0);
    g_graphicsContext.Unlock();
    return false;
  }

  // if album name is set, then we use this as the directory to place the new file in.
  if (pItem->GetMusicInfoTag()->GetAlbum().size() > 0)
  {
    strDirectory += CUtil::MakeLegalFileName(pItem->GetMusicInfoTag()->GetAlbum().c_str(), bIsFATX);
    CUtil::AddDirectorySeperator(strDirectory);
  }

  // Create directory if it doesn't exist
  CUtil::CreateDirectoryEx(strDirectory);

  CStdString strFile;
  CUtil::AddFileToFolder(strDirectory, GetTrackName(pItem, bIsFATX), strFile);

  return Rip(pItem->m_strPath, strFile.c_str(), *pItem->GetMusicInfoTag());
}

bool CCDDARipper::RipCD()
{
  int iTrack = 0;
  bool bResult = true;
  CStdString strFile;
  CStdString strDirectory = g_guiSettings.GetString("cddaripper.path");
  if (!CUtil::HasSlashAtEnd(strDirectory)) CUtil::AddDirectorySeperator(strDirectory);
  CFileItem ripPath(strDirectory, true);
  bool bIsFATX = !ripPath.IsSmb();

  // return here if cd is not a CDDA disc
  MEDIA_DETECT::CCdInfo* pInfo = MEDIA_DETECT::CDetectDVDMedia::GetCdInfo();
  if (pInfo == NULL && !pInfo->IsAudio(1))
  {
    CLog::Log(LOGDEBUG, "cddaripper: CD is not an audio cd");
    return false;
  }

  if (strDirectory.size() < 3)
  {
    // no rip path has been set, show error
    CLog::Log(LOGERROR, "Error: CDDARipPath has not been set");
    g_graphicsContext.Lock();
    CGUIDialogOK::ShowAndGetInput(257, 608, 609, 0);
    g_graphicsContext.Unlock();
    return false;
  }

  // get cd cdda contents
  CFileItemList vecItems;
  DIRECTORY::CCDDADirectory directory;
  directory.GetDirectory("cdda://local/", vecItems);

  //  Get cddb info
  for (int i = 0; i < vecItems.Size(); ++i)
  {
    CFileItem* pItem = vecItems[i];
    CMusicInfoTagLoaderFactory factory;
    auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
    if (NULL != pLoader.get())
    {
      pLoader->Load(pItem->m_strPath, *pItem->GetMusicInfoTag()); // get tag from file
      if (!pItem->GetMusicInfoTag()->Loaded())
        break;  //  No CDDB info available
    }
  }

  // if album name from first item is set,
  // then we use this as the directory to place the new file in.
  CStdString strAlbumDir;
  if (vecItems[0]->GetMusicInfoTag()->GetAlbum().size() > 0)
  {
    strAlbumDir=CUtil::MakeLegalFileName(vecItems[0]->GetMusicInfoTag()->GetAlbum().c_str(), bIsFATX);
  }

    // No legal fatx directory name or no album in tag
  if (strAlbumDir.IsEmpty())
  {
    // create a directory based on current date
    SYSTEMTIME datetime;
    CStdString strDate;
    GetLocalTime(&datetime);
    int iNumber = 1;
    while (1)
    {
      strDate.Format("%04i-%02i-%02i-%i", datetime.wYear, datetime.wMonth, datetime.wDay, iNumber);
      if (!CGUIDialogKeyboard::ShowAndGetInput(strDate, g_localizeStrings.Get(748), false))
        return false;
      if (!CFile::Exists(strDirectory + strDate))
      {
        strAlbumDir=strDate;
        break;
      }
      iNumber++;
    }
  }

  // construct directory where the tracks are stored
  strDirectory += strAlbumDir;
  CUtil::AddDirectorySeperator(strDirectory);

  // Create directory if it doesn't exist
  if (!CUtil::CreateDirectoryEx(strDirectory))
  {
    CLog::Log(LOGERROR, "Unable to create directory '%s'", strDirectory.c_str());
    return false;
  }

  // rip all tracks one by one, if one fails we quit and return false
  for (int i = 0; i < vecItems.Size() && bResult == true; i++)
  {
    CStdString track(GetTrackName(vecItems[i], bIsFATX));

    // construct filename
    CUtil::AddFileToFolder(strDirectory, track, strFile);

    DWORD dwTick = timeGetTime();

    // don't rip non cdda items
    if (vecItems[i]->m_strPath.Find(".cdda") < 0)
      continue;

    // return false if Rip returned false (this means an error or the user cancelled
    if (!Rip(vecItems[i]->m_strPath, strFile.c_str(), *vecItems[i]->GetMusicInfoTag())) return false;

    dwTick = timeGetTime() - dwTick;
    CStdString strTmp;
    StringUtils::SecondsToTimeString(dwTick / 1000, strTmp);
    CLog::Log(LOGINFO, "Ripping Track %d took %s", iTrack, strTmp.c_str());
  }
  CLog::Log(LOGINFO, "Ripped CD succesfull");
  return true;
}

char* CCDDARipper::GetExtension(int iEncoder)
{
  if (iEncoder == CDDARIP_ENCODER_WAV) return ".wav";
  if (iEncoder == CDDARIP_ENCODER_VORBIS) return ".ogg";
  return ".mp3";
}

CStdString CCDDARipper::GetTrackName(CFileItem *item, bool isFatX)
{
  // get track number from "cdda://local/01.cdda"
  int trackNumber = atoi(item->m_strPath.substr(13, item->m_strPath.size() - 13 - 5).c_str());

  // Format up our ripped file label
  CFileItem destItem(*item);
  destItem.SetLabel("");
  CLabelFormatter formatter(g_guiSettings.GetString("cddaripper.trackformat"), "");
  formatter.FormatLabel(&destItem);

  // grab the label to use it as our ripped filename
  CStdString track = destItem.GetLabel();
  if (track.IsEmpty())
    track.Format("%s%02i", "Track-", trackNumber);
  track += GetExtension(g_guiSettings.GetInt("cddaripper.encoder"));

  // make sure the filename is legal
  track = CUtil::MakeLegalFileName(track.c_str(), isFatX);
  return track;
}
