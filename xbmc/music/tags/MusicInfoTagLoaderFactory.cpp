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
#include "MusicInfoTagLoaderDatabase.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "FileItem.h"

#include "addons/AddonManager.h"
#include "addons/AudioDecoder.h"

using namespace ADDON;

using namespace MUSIC_INFO;

CMusicInfoTagLoaderFactory::CMusicInfoTagLoaderFactory()
{}

CMusicInfoTagLoaderFactory::~CMusicInfoTagLoaderFactory()
{}

IMusicInfoTagLoader* CMusicInfoTagLoaderFactory::CreateLoader(const CFileItem& item)
{
  // dont try to read the tags for streams & shoutcast
  if (item.IsInternetStream())
    return NULL;

  if (item.IsMusicDb())
    return new CMusicInfoTagLoaderDatabase();

  std::string strExtension = URIUtils::GetExtension(item.GetPath());
  StringUtils::ToLower(strExtension);
  StringUtils::TrimLeft(strExtension, ".");

  if (strExtension.empty())
    return NULL;

  VECADDONS codecs;
  CAddonMgr::Get().GetAddons(ADDON_AUDIODECODER, codecs);
  for (size_t i=0;i<codecs.size();++i)
  {
    std::shared_ptr<CAudioDecoder> dec(std::static_pointer_cast<CAudioDecoder>(codecs[i]));
    if (dec->HasTags() && dec->GetExtensions().find("."+strExtension) != std::string::npos)
    {
      CAudioDecoder* result = new CAudioDecoder(*dec);
      static_cast<AudioDecoderDll&>(*result).Create();
      return result;
    }
  }


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
      strExtension == "mod" ||
      strExtension == "s3m" || strExtension == "it" || strExtension == "xm" ||
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

  return NULL;
}
