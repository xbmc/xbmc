/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "musicInfoTagLoaderFactory.h"
#include "MusicInfoTagLoaderMP3.h"
#include "MusicInfoTagLoaderOgg.h"
#include "MusicInfoTagLoaderWMA.h"
#include "MusicInfoTagLoaderFlac.h"
#include "MusicInfoTagLoaderMP4.h"
#include "MusicInfoTagLoaderCDDA.h"
#include "MusicInfoTagLoaderApe.h"
#include "MusicInfoTagLoaderMPC.h"
#include "MusicInfoTagLoaderShn.h"
#include "MusicInfoTagLoaderSid.h"
#include "MusicInfoTagLoaderMod.h"
#include "MusicInfoTagLoaderWav.h"
#include "MusicInfoTagLoaderAAC.h"
#include "MusicInfoTagLoaderWavPack.h"
#ifdef HAS_MOD_PLAYER
#include "cores/ModPlayer.h"
#endif
#include "MusicInfoTagLoaderNSF.h"
#include "MusicInfoTagLoaderSPC.h"
#include "MusicInfoTagLoaderGYM.h"
#include "MusicInfoTagLoaderAdplug.h"
#include "MusicInfoTagLoaderYM.h"
#include "MusicInfoTagLoaderDatabase.h"

#include "Util.h"


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

  CStdString strExtension;
  CUtil::GetExtension( strFileName, strExtension);
  strExtension.ToLower();
  strExtension.TrimLeft('.');

  if (strExtension.IsEmpty())
    return NULL;

  if (strExtension == "mp3")
  {
    CMusicInfoTagLoaderMP3 *pTagLoader = new CMusicInfoTagLoaderMP3();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ogg" || strExtension == "oggstream")
  {
    CMusicInfoTagLoaderOgg *pTagLoader = new CMusicInfoTagLoaderOgg();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "wma")
  {
    CMusicInfoTagLoaderWMA *pTagLoader = new CMusicInfoTagLoaderWMA();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "flac")
  {
    CMusicInfoTagLoaderFlac *pTagLoader = new CMusicInfoTagLoaderFlac();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "m4a" || strExtension == "mp4")
  {
    CMusicInfoTagLoaderMP4 *pTagLoader = new CMusicInfoTagLoaderMP4();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "cdda")
  {
    CMusicInfoTagLoaderCDDA *pTagLoader = new CMusicInfoTagLoaderCDDA();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ape" || strExtension == "mac")
  {
    CMusicInfoTagLoaderApe *pTagLoader = new CMusicInfoTagLoaderApe();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "mpc" || strExtension == "mpp" || strExtension == "mp+")
  {
    CMusicInfoTagLoaderMPC *pTagLoader = new CMusicInfoTagLoaderMPC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "shn")
  {
    CMusicInfoTagLoaderSHN *pTagLoader = new CMusicInfoTagLoaderSHN();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "sid" || strExtension == "sidstream")
  {
    CMusicInfoTagLoaderSid *pTagLoader = new CMusicInfoTagLoaderSid();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#ifdef HAS_MOD_PLAYER
  else if (ModPlayer::IsSupportedFormat(strExtension) || strExtension == "mod" || strExtension == "it" || strExtension == "s3m")
  {
    CMusicInfoTagLoaderMod *pTagLoader = new CMusicInfoTagLoaderMod();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
#endif
  else if (strExtension == "wav")
  {
    CMusicInfoTagLoaderWAV *pTagLoader = new CMusicInfoTagLoaderWAV();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "aac")
  {
    CMusicInfoTagLoaderAAC *pTagLoader = new CMusicInfoTagLoaderAAC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "wv")
  {
    CMusicInfoTagLoaderWAVPack *pTagLoader = new CMusicInfoTagLoaderWAVPack();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "nsf" || strExtension == "nsfstream")
  {
    CMusicInfoTagLoaderNSF *pTagLoader = new CMusicInfoTagLoaderNSF();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "spc")
  {
    CMusicInfoTagLoaderSPC *pTagLoader = new CMusicInfoTagLoaderSPC();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "gym")
  {
    CMusicInfoTagLoaderGYM *pTagLoader = new CMusicInfoTagLoaderGYM();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (strExtension == "ym")
  {
    CMusicInfoTagLoaderYM *pTagLoader = new CMusicInfoTagLoaderYM();
    return (IMusicInfoTagLoader*)pTagLoader;
  }
  else if (AdplugCodec::IsSupportedFormat(strExtension))
  {
    CMusicInfoTagLoaderAdplug *pTagLoader = new CMusicInfoTagLoaderAdplug();
    return (IMusicInfoTagLoader*)pTagLoader;
  }

  return NULL;
}
