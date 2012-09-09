/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "system.h"

#ifdef HAS_CDDA_RIPPER

#include "CDDARipper.h"
#include "CDDARipJob.h"
#include "utils/StringUtils.h"
#include "Util.h"
#include "filesystem/CDDADirectory.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "utils/LabelFormatter.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "filesystem/SpecialProtocol.h"
#include "storage/MediaManager.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;

CCDDARipper& CCDDARipper::GetInstance()
{
  static CCDDARipper sRipper;
  return sRipper;
}

CCDDARipper::CCDDARipper()
{
}

CCDDARipper::~CCDDARipper()
{
}

// rip a single track from cd
bool CCDDARipper::RipTrack(CFileItem* pItem)
{
  // don't rip non cdda items
  CStdString strExt;
  URIUtils::GetExtension(pItem->GetPath(), strExt);
  if (strExt.CompareNoCase(".cdda") != 0) 
  {
    CLog::Log(LOGDEBUG, "cddaripper: file is not a cdda track");
    return false;
  }

  // construct directory where the track is stored
  CStdString strDirectory;
  int legalType;
  if (!CreateAlbumDir(*pItem->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  CStdString strFile = URIUtils::AddFileToFolder(strDirectory, 
                      CUtil::MakeLegalFileName(GetTrackName(pItem), legalType));

  AddJob(new CCDDARipJob(pItem->GetPath(),strFile,
                         *pItem->GetMusicInfoTag(),
                         g_guiSettings.GetInt("audiocds.encoder")));

  return true;
}

bool CCDDARipper::RipCD()
{
  // return here if cd is not a CDDA disc
  MEDIA_DETECT::CCdInfo* pInfo = g_mediaManager.GetCdInfo();
  if (pInfo == NULL || !pInfo->IsAudio(1))
  {
    CLog::Log(LOGDEBUG, "cddaripper: CD is not an audio cd");
    return false;
  }

  // get cd cdda contents
  CFileItemList vecItems;
  XFILE::CCDDADirectory directory;
  directory.GetDirectory("cdda://local/", vecItems);

  // get cddb info
  for (int i = 0; i < vecItems.Size(); ++i)
  {
    CFileItemPtr pItem = vecItems[i];
    CMusicInfoTagLoaderFactory factory;
    auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->GetPath()));
    if (NULL != pLoader.get())
    {
      pLoader->Load(pItem->GetPath(), *pItem->GetMusicInfoTag()); // get tag from file
      if (!pItem->GetMusicInfoTag()->Loaded())
        break;  //  No CDDB info available
    }
  }

  // construct directory where the tracks are stored
  CStdString strDirectory;
  int legalType;
  if (!CreateAlbumDir(*vecItems[0]->GetMusicInfoTag(), strDirectory, legalType))
    return false;

  // rip all tracks one by one
  for (int i = 0; i < vecItems.Size(); i++)
  {
    CFileItemPtr item = vecItems[i];

    // construct filename
    CStdString strFile = URIUtils::AddFileToFolder(strDirectory, CUtil::MakeLegalFileName(GetTrackName(item.get()), legalType));

    // don't rip non cdda items
    if (item->GetPath().Find(".cdda") < 0)
      continue;

    bool eject = g_guiSettings.GetBool("audiocds.ejectonrip") && 
                 i == vecItems.Size()-1;
    AddJob(new CCDDARipJob(item->GetPath(),strFile,
                           *item->GetMusicInfoTag(),
                           g_guiSettings.GetInt("audiocds.encoder"), eject));
  }

  return true;
}

const char* CCDDARipper::GetExtension(int iEncoder)
{
  if (iEncoder == CDDARIP_ENCODER_WAV) return ".wav";
  if (iEncoder == CDDARIP_ENCODER_VORBIS) return ".ogg";
  if (iEncoder == CDDARIP_ENCODER_FLAC) return ".flac";
  return ".mp3";
}

bool CCDDARipper::CreateAlbumDir(const MUSIC_INFO::CMusicInfoTag& infoTag, CStdString& strDirectory, int& legalType)
{
  strDirectory = g_guiSettings.GetString("audiocds.recordingpath");
  URIUtils::AddSlashAtEnd(strDirectory);

  if (strDirectory.size() < 3)
  {
    // no rip path has been set, show error
    CLog::Log(LOGERROR, "Error: CDDARipPath has not been set");
    g_graphicsContext.Lock();
    CGUIDialogOK::ShowAndGetInput(257, 608, 609, 0);
    g_graphicsContext.Unlock();
    return false;
  }

  legalType = LEGAL_NONE;
  CFileItem ripPath(strDirectory, true);
  if (ripPath.IsSmb())
    legalType = LEGAL_WIN32_COMPAT;
#ifdef _WIN32
  if (ripPath.IsHD())
    legalType = LEGAL_WIN32_COMPAT;
#endif

  CStdString strAlbumDir = GetAlbumDirName(infoTag);

  if (!strAlbumDir.IsEmpty())
  {
    URIUtils::AddFileToFolder(strDirectory, strAlbumDir, strDirectory);
    URIUtils::AddSlashAtEnd(strDirectory);
  }

  strDirectory = CUtil::MakeLegalPath(strDirectory, legalType);

  // Create directory if it doesn't exist
  if (!CUtil::CreateDirectoryEx(strDirectory))
  {
    CLog::Log(LOGERROR, "Unable to create directory '%s'", strDirectory.c_str());
    return false;
  }

  return true;
}

CStdString CCDDARipper::GetAlbumDirName(const MUSIC_INFO::CMusicInfoTag& infoTag)
{
  CStdString strAlbumDir;

  // use audiocds.trackpathformat setting to format
  // directory name where CD tracks will be stored,
  // use only format part ending at the last '/'
  strAlbumDir = g_guiSettings.GetString("audiocds.trackpathformat");
  int pos = max(strAlbumDir.ReverseFind('/'), strAlbumDir.ReverseFind('\\'));
  if (pos < 0)
    return ""; // no directory
  
  strAlbumDir = strAlbumDir.Left(pos);

  // replace %A with album artist name
  if (strAlbumDir.Find("%A") != -1)
  {
    CStdString strAlbumArtist = StringUtils::Join(infoTag.GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator);
    if (strAlbumArtist.IsEmpty())
      strAlbumArtist = StringUtils::Join(infoTag.GetArtist(), g_advancedSettings.m_musicItemSeparator);
    if (strAlbumArtist.IsEmpty())
      strAlbumArtist = "Unknown Artist";
    else
      strAlbumArtist.Replace('/', '_');
    strAlbumDir.Replace("%A", strAlbumArtist);
  }

  // replace %B with album title
  if (strAlbumDir.Find("%B") != -1)
  {
    CStdString strAlbum = infoTag.GetAlbum();
    if (strAlbum.IsEmpty()) 
      strAlbum.Format("Unknown Album %s", CDateTime::GetCurrentDateTime().GetAsLocalizedDateTime().c_str());
    else
      strAlbum.Replace('/', '_');
    strAlbumDir.Replace("%B", strAlbum);
  }

  // replace %G with genre
  if (strAlbumDir.Find("%G") != -1)
  {
    CStdString strGenre = StringUtils::Join(infoTag.GetGenre(), g_advancedSettings.m_musicItemSeparator);
    if (strGenre.IsEmpty())
      strGenre = "Unknown Genre";
    else
      strGenre.Replace('/', '_');
    strAlbumDir.Replace("%G", strGenre);
  }

  // replace %Y with year
  if (strAlbumDir.Find("%Y") != -1)
  {
    CStdString strYear = infoTag.GetYearString();
    if (strYear.IsEmpty())
      strYear = "Unknown Year";
    else
      strYear.Replace('/', '_');
    strAlbumDir.Replace("%Y", strYear);
  }

  return strAlbumDir;
}

CStdString CCDDARipper::GetTrackName(CFileItem *item)
{
  // get track number from "cdda://local/01.cdda"
  int trackNumber = atoi(item->GetPath().substr(13, item->GetPath().size() - 13 - 5).c_str());

  // Format up our ripped file label
  CFileItem destItem(*item);
  destItem.SetLabel("");

  // get track file name format from audiocds.trackpathformat setting,
  // use only format part starting from the last '/'
  CStdString strFormat = g_guiSettings.GetString("audiocds.trackpathformat");
  int pos = max(strFormat.ReverseFind('/'), strFormat.ReverseFind('\\'));
  if (pos != -1)
  {
    strFormat = strFormat.Right(strFormat.GetLength() - pos - 1);
  }

  CLabelFormatter formatter(strFormat, "");
  formatter.FormatLabel(&destItem);

  // grab the label to use it as our ripped filename
  CStdString track = destItem.GetLabel();
  if (track.IsEmpty())
    track.Format("%s%02i", "Track-", trackNumber);
  track += GetExtension(g_guiSettings.GetInt("audiocds.encoder"));

  return track;
}

void CCDDARipper::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (success)
    return CJobQueue::OnJobComplete(jobID, success, job);

  CancelJobs();
}

#endif
