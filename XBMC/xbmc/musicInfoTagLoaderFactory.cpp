#pragma once

#include "stdafx.h"
#include "musicInfoTagLoaderFactory.h"
#include "MusicInfoTagLoaderMP3.h"
#include "MusicInfoTagLoaderOgg.h"
#include "MusicInfoTagLoaderWMA.h"
#include "MusicInfoTagLoaderFlac.h"
#include "MusicInfoTagLoaderMP4.h"
#include "util.h"
#include "url.h"

using namespace MUSIC_INFO;
CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory()
{
}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory()
{
}

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CStdString& strFileName)
{
	CStdString strExtension;
	CUtil::GetExtension( strFileName, strExtension);
	strExtension.ToLower();
  CURL url(strFileName);

  // dont try to locate a folder.jpg for streams &  shoutcast
  if (url.GetProtocol() =="http" || url.GetProtocol()=="HTTP") return NULL;
  if (url.GetProtocol() =="shout" || url.GetProtocol()=="SHOUT") return NULL;
  if (url.GetProtocol() =="mms" || url.GetProtocol()=="MMS") return NULL;
  if (url.GetProtocol() =="udp" || url.GetProtocol()=="UDP") return NULL;
  if (url.GetProtocol() =="ftp" || url.GetProtocol()=="FTP") return NULL;
  if (url.GetProtocol() =="rtp" || url.GetProtocol()=="RTP") return NULL;
  if (url.GetProtocol() =="rtsp" || url.GetProtocol()=="RTSP") return NULL;
	if (strExtension==".mp3")
	{
		CMusicInfoTagLoaderMP3 *pTagLoader= new CMusicInfoTagLoaderMP3();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	else if (strExtension==".ogg")
	{
		CMusicInfoTagLoaderOgg *pTagLoader= new CMusicInfoTagLoaderOgg();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	else if (strExtension==".wma")
	{
		CMusicInfoTagLoaderWMA *pTagLoader= new CMusicInfoTagLoaderWMA();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	else if (strExtension==".flac")
	{
		CMusicInfoTagLoaderFlac *pTagLoader= new CMusicInfoTagLoaderFlac();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	else if (strExtension==".m4a")
	{
		CMusicInfoTagLoaderMP4 *pTagLoader= new CMusicInfoTagLoaderMP4();
		return (IMusicInfoTagLoader*)pTagLoader;
	}
	return NULL;
}
