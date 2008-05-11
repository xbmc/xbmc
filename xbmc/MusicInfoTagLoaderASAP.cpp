#include "stdafx.h"
#include "MusicInfoTagLoaderASAP.h"
#include "Util.h"
#include "MusicInfoTag.h"
#include "FileSystem/File.h"

using namespace MUSIC_INFO;

CMusicInfoTagLoaderASAP::CMusicInfoTagLoaderASAP()
{
}

CMusicInfoTagLoaderASAP::~CMusicInfoTagLoaderASAP()
{
}

bool CMusicInfoTagLoaderASAP::Load(const CStdString &strFile, CMusicInfoTag &tag)
{
  tag.SetLoaded(false);

  if (!m_dll.Load())
    return false;

  CStdString strFileToLoad = strFile;
  int song = -1;
  CStdString strExtension;
  CUtil::GetExtension(strFile, strExtension);
  strExtension.MakeLower();
  if (strExtension == ".asapstream")
  {
    CStdString strFileName = CUtil::GetFileName(strFile);
    int iStart = strFileName.ReverseFind('-') + 1;
    song = atoi(strFileName.substr(iStart, strFileName.size() - iStart - 11).c_str()) - 1;
    CStdString strPath = strFile;
    CUtil::GetDirectory(strPath, strFileToLoad);
    CUtil::RemoveSlashAtEnd(strFileToLoad);
  }

  ASAP_SongInfo songInfo;
  if (!m_dll.asapGetInfo(strFileToLoad.c_str(), song, &songInfo))
    return false;

  tag.SetURL(strFileToLoad);
  if (songInfo.name[0] != '\0')
    tag.SetTitle(songInfo.name);
  if (songInfo.author[0] != '\0')
    tag.SetArtist(songInfo.author);
  if (song >= 0)
    tag.SetTrackNumber(song + 1);
  if (songInfo.duration >= 0)
    tag.SetDuration(songInfo.duration / 1000);
  if (songInfo.year > 0)
  {
    SYSTEMTIME dateTime = { songInfo.year, songInfo.month, 0, songInfo.day, 0, 0, 0, 0 };
    tag.SetReleaseDate(dateTime);
  }
  tag.SetLoaded(true);
  return true;
}
