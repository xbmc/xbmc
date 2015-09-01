/*
*      Copyright (C) 2005-2015 Team Kodi
*      http://kodi.tv
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

#include "RadioRDSInfoProvider.h"

#include "Application.h"
#include "FileItem.h"
#include "cores/IPlayer.h"
#include "epg/EpgInfoTag.h"
#include "guiinfo/GUIInfoLabels.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"

namespace KODI
{
namespace GUIINFO
{

static std::string GetAudioLang(const PVR::CPVRRadioRDSInfoTag& tag)
{
  if (!tag.GetLanguage().empty())
    return tag.GetLanguage();

  SPlayerAudioStreamInfo info;
  g_application.m_pPlayer->GetAudioStreamInfo(g_application.m_pPlayer->GetAudioStream(), info);
  return info.language;
}

static std::string GetProgNow(const CFileItem& currentFile, const PVR::CPVRRadioRDSInfoTag& tag)
{
  if (!tag.GetProgNow().empty())
    return tag.GetProgNow();

  EPG::CEpgInfoTagPtr epgNow(currentFile.GetPVRChannelInfoTag()->GetEPGNow());
  if (epgNow)
    return epgNow->Title();

  if (!CSettings::GetInstance().GetBool("epg.hidenoinfoavailable"))
    return g_localizeStrings.Get(19055); // no information available

  return std::string();
}

static std::string GetProgNext(const CFileItem& currentFile, const PVR::CPVRRadioRDSInfoTag& tag)
{
  if (!tag.GetProgNext().empty())
    return tag.GetProgNext();

  EPG::CEpgInfoTagPtr epgNext(currentFile.GetPVRChannelInfoTag()->GetEPGNext());
  if (epgNext)
    return epgNext->Title();

  if (!CSettings::GetInstance().GetBool("epg.hidenoinfoavailable"))
    return g_localizeStrings.Get(19055); // no information available

  return std::string();
}

static std::string GetProgStation(const CFileItem& currentFile, const PVR::CPVRRadioRDSInfoTag& tag)
{
  if (!tag.GetProgStation().empty())
    return tag.GetProgStation();

  const PVR::CPVRChannelPtr channeltag = currentFile.GetPVRChannelInfoTag();
  
  if (channeltag)
    return channeltag->ChannelName();

  return std::string();
}

std::string GetRadioRDSLabel(const CFileItem & currentFile, int info)
{
  if (!g_application.m_pPlayer->IsPlaying() ||
    !currentFile.HasPVRChannelInfoTag() ||
    !currentFile.HasPVRRadioRDSInfoTag())
    return std::string();

  const PVR::CPVRRadioRDSInfoTag &tag = *currentFile.GetPVRRadioRDSInfoTag();
  switch (info)
  {
  case RDS_CHANNEL_COUNTRY:
    return tag.GetCountry();

  case RDS_AUDIO_LANG:
    return GetAudioLang(tag);

  case RDS_TITLE:
    return tag.GetTitle();

  case RDS_ARTIST:
    return tag.GetArtist();

  case RDS_BAND:
    return tag.GetBand();

  case RDS_COMPOSER:
    return tag.GetComposer();

  case RDS_CONDUCTOR:
    return tag.GetConductor();

  case RDS_ALBUM:
    return tag.GetAlbum();

  case RDS_ALBUM_TRACKNUMBER:
    if (tag.GetAlbumTrackNumber() > 0)
      return StringUtils::Format("%i", tag.GetAlbumTrackNumber());
    break;

  case RDS_GET_RADIO_STYLE:
    return tag.GetRadioStyle();

  case RDS_COMMENT:
    return tag.GetComment();

  case RDS_INFO_NEWS:
    return tag.GetInfoNews();

  case RDS_INFO_NEWS_LOCAL:
    return tag.GetInfoNewsLocal();

  case RDS_INFO_STOCK:
    return tag.GetInfoStock();

  case RDS_INFO_STOCK_SIZE:
    return StringUtils::Format("%i", static_cast<int>(tag.GetInfoStock().length()));

  case RDS_INFO_SPORT:
    return tag.GetInfoSport();

  case RDS_INFO_SPORT_SIZE:
    return StringUtils::Format("%i", static_cast<int>(tag.GetInfoSport().length()));

  case RDS_INFO_LOTTERY:
    return tag.GetInfoLottery();

  case RDS_INFO_LOTTERY_SIZE:
    return StringUtils::Format("%i", static_cast<int>(tag.GetInfoLottery().length()));

  case RDS_INFO_WEATHER:
    return tag.GetInfoWeather();

  case RDS_INFO_WEATHER_SIZE:
    return StringUtils::Format("%i", static_cast<int>(tag.GetInfoWeather().length()));

  case RDS_INFO_HOROSCOPE:
    return tag.GetInfoHoroscope();

  case RDS_INFO_HOROSCOPE_SIZE:
    return StringUtils::Format("%i", static_cast<int>(tag.GetInfoHoroscope().length()));

  case RDS_INFO_CINEMA:
    return tag.GetInfoCinema();

  case RDS_INFO_CINEMA_SIZE:
    return StringUtils::Format("%i", static_cast<int>(tag.GetInfoCinema().length()));

  case RDS_INFO_OTHER:
    return tag.GetInfoOther();

  case RDS_INFO_OTHER_SIZE:
    return StringUtils::Format("%i", static_cast<int>(tag.GetInfoOther().length()));

  case RDS_PROG_STATION:
    return GetProgStation(currentFile, tag);

  case RDS_PROG_NOW:
    return GetProgNow(currentFile, tag);

  case RDS_PROG_NEXT:
    return GetProgNext(currentFile, tag);

  case RDS_PROG_HOST:
    return tag.GetProgHost();

  case RDS_PROG_EDIT_STAFF:
    return tag.GetEditorialStaff();

  case RDS_PROG_HOMEPAGE:
    return tag.GetProgWebsite();

  case RDS_PROG_STYLE:
    return tag.GetProgStyle();

  case RDS_PHONE_HOTLINE:
    return tag.GetPhoneHotline();

  case RDS_PHONE_STUDIO:
    return tag.GetPhoneStudio();

  case RDS_SMS_STUDIO:
    return tag.GetSMSStudio();

  case RDS_EMAIL_HOTLINE:
    return tag.GetEMailHotline();

  case RDS_EMAIL_STUDIO:
    return tag.GetEMailStudio();

  default:
    break;
  }

  return std::string();
}
}
}
