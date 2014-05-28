/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#ifndef TAGS_SYSTEM_H_INCLUDED
#define TAGS_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERFACTORY_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERFACTORY_H_INCLUDED
#include "MusicInfoTagLoaderFactory.h"
#endif

#ifndef TAGS_TAGLOADERTAGLIB_H_INCLUDED
#define TAGS_TAGLOADERTAGLIB_H_INCLUDED
#include "TagLoaderTagLib.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERCDDA_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERCDDA_H_INCLUDED
#include "MusicInfoTagLoaderCDDA.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERSHN_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERSHN_H_INCLUDED
#include "MusicInfoTagLoaderShn.h"
#endif

#ifdef HAS_MOD_PLAYER
#include "cores/ModPlayer.h"
#endif
#ifndef TAGS_MUSICINFOTAGLOADERNSF_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERNSF_H_INCLUDED
#include "MusicInfoTagLoaderNSF.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERSPC_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERSPC_H_INCLUDED
#include "MusicInfoTagLoaderSPC.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERYM_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERYM_H_INCLUDED
#include "MusicInfoTagLoaderYM.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERDATABASE_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERDATABASE_H_INCLUDED
#include "MusicInfoTagLoaderDatabase.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERASAP_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERASAP_H_INCLUDED
#include "MusicInfoTagLoaderASAP.h"
#endif

#ifndef TAGS_MUSICINFOTAGLOADERMIDI_H_INCLUDED
#define TAGS_MUSICINFOTAGLOADERMIDI_H_INCLUDED
#include "MusicInfoTagLoaderMidi.h"
#endif

#ifndef TAGS_UTILS_STRINGUTILS_H_INCLUDED
#define TAGS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef TAGS_UTILS_URIUTILS_H_INCLUDED
#define TAGS_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef TAGS_FILEITEM_H_INCLUDED
#define TAGS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif


#ifdef HAS_ASAP_CODEC
#include "cores/paplayer/ASAPCodec.h"
#endif

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory()
{}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory()
{}

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CStdString& strFileName)
{
  // dont try to read the tags for streams & shoutcast
  CFileItem item(strFileName, false);
  if (item.IsInternetStream())
    return NULL;

  if (item.IsMusicDb())
    return new CMusicInfoTagLoaderDatabase();

  CStdString strExtension = URIUtils::GetExtension(strFileName);
  StringUtils::ToLower(strExtension);
  StringUtils::TrimLeft(strExtension, ".");

  if (strExtension.empty())
    return NULL;

  if (strExtension == "aac" ||
      strExtension == "ape" || strExtension == "mac" ||
      strExtension == "mp3" || 
      strExtension == "wma" || 
      strExtension == "flac" || 
      strExtension == "m4a" || strExtension == "mp4" ||
      strExtension == "mpc" || strExtension == "mpp" || strExtension == "mp+" ||
      strExtension == "ogg" || strExtension == "oga" || strExtension == "oggstream" ||
      strExtension == "aif" || strExtension == "aiff" ||
      strExtension == "wav" ||
#ifdef HAS_MOD_PLAYER
      ModPlayer::IsSupportedFormat(strExtension) ||
      strExtension == "mod" || strExtension == "nsf" || strExtension == "nsfstream" ||
      strExtension == "s3m" || strExtension == "it" || strExtension == "xm" ||
#endif
      strExtension == "wv")
  {
    CTagLoaderTagLib *pTagLoader = new CTagLoaderTagLib();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_DVD_DRIVE
  else if (strExtension == "cdda")
  {
    CMusicInfoTagLoaderCDDA *pTagLoader = new CMusicInfoTagLoaderCDDA();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
  else if (strExtension == "shn")
  {
    CMusicInfoTagLoaderSHN *pTagLoader = new CMusicInfoTagLoaderSHN();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "spc")
  {
    CMusicInfoTagLoaderSPC *pTagLoader = new CMusicInfoTagLoaderSPC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ym")
  {
    CMusicInfoTagLoaderYM *pTagLoader = new CMusicInfoTagLoaderYM();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_ASAP_CODEC
  else if (ASAPCodec::IsSupportedFormat(strExtension) || strExtension == "asapstream")
  {
    CMusicInfoTagLoaderASAP *pTagLoader = new CMusicInfoTagLoaderASAP();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
  else if ( TimidityCodec::IsSupportedFormat( strExtension ) )
  {
    CMusicInfoTagLoaderMidi * pTagLoader = new CMusicInfoTagLoaderMidi();
    return (IMusicInfoTagLoader*)pTagLoader;
  }

  return NULL;
}
