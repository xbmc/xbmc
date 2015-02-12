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

#include "system.h"
#include "MusicInfoTagLoaderFactory.h"
#include "TagLoaderTagLib.h"
#include "MusicInfoTagLoaderCDDA.h"
#include "MusicInfoTagLoaderShn.h"
#ifdef HAS_MOD_PLAYER
#include "cores/ModPlayer.h"
#endif
#include "MusicInfoTagLoaderNSF.h"
#include "MusicInfoTagLoaderSPC.h"
#include "MusicInfoTagLoaderYM.h"
#include "MusicInfoTagLoaderDatabase.h"
#include "MusicInfoTagLoaderASAP.h"
#include "MusicInfoTagLoaderMidi.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"

#ifdef HAS_ASAP_CODEC
#include "cores/paplayer/ASAPCodec.h"
#endif

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory()
{}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory()
{}

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const std::string& strFileName)
{
  // dont try to read the tags for streams & shoutcast
  CFileItem item(strFileName, false);
  if (item.IsInternetStream())
    return NULL;

  if (item.IsMusicDb())
    return new CMusicInfoTagLoaderDatabase();

  std::string strExtension = URIUtils::GetExtension(strFileName);
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
