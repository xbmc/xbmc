
#include "stdafx.h"
#include "playlistfactory.h"
#include "Playlistm3u.h"
#include "PlaylistPLS.h"
#include "Playlistb4S.h"
#include "PlaylistWPL.h"
#include "util.h"


using namespace PLAYLIST;


CPlayList* CPlayListFactory::Create(const CStdString& filename)
{
  CFileItem item(filename,false);
  return Create(item);
}

CPlayList* CPlayListFactory::Create(const CFileItem& item)
{  
  if(item.IsLastFM()) //lastfm is always a stream, and just silly to check content
    return NULL;

  if( item.IsInternetStream() )
  {
    CStdString strContentType = item.GetContentType();
    strContentType.MakeLower();

    if (strContentType == "video/x-ms-asf" 
    || strContentType == "video/x-ms-asx")
      return new CPlayListASX();

    if (strContentType == "audio/x-pn-realaudio")
      return new CPlayListRAM();

    if (strContentType == "audio/x-scpls" 
    || strContentType == "playlist"
    || strContentType == "text/html")
      return new CPlayListPLS();
  }

  CStdString extension = CUtil::GetExtension(item.m_strPath);
  extension.MakeLower();

  if (extension == ".m3u")
    return new CPlayListM3U();

  if (extension == ".pls" || extension == ".strm")
    return new CPlayListPLS();

  if (extension == ".b4s")
    return new CPlayListB4S();

  if (extension == ".wpl")
    return new CPlayListWPL();

  if (extension == ".asx")
    return new CPlayListASX();

  if (extension == ".ram")
    return new CPlayListRAM();

  return NULL;

}

bool CPlayListFactory::IsPlaylist(const CStdString& filename)
{
  CStdString extension = CUtil::GetExtension(filename);
  extension.ToLower();

  if (extension == ".m3u") return true;
  if (extension == ".b4s") return true;
  if (extension == ".pls") return true;
  if (extension == ".strm") return true;
  if (extension == ".wpl") return true;
  if (extension == ".asx") return true;
  if (extension == ".ram") return true;
  return false;
}