/*
 *      Copyright (C) 2005-2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InfoTagRadioRDS.h"
#include "utils/StringUtils.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    InfoTagRadioRDS::InfoTagRadioRDS()
    {
      PVR::CPVRRadioRDSInfoTagPtr empty;
      infoTag = empty;
    }

    InfoTagRadioRDS::InfoTagRadioRDS(const PVR::CPVRRadioRDSInfoTagPtr tag)
    {
      infoTag = tag;
    }

    InfoTagRadioRDS::~InfoTagRadioRDS() = default;

    String InfoTagRadioRDS::getTitle()
    {
      if (infoTag)
        return infoTag->GetTitle();
      return "";
    }

    String InfoTagRadioRDS::getBand()
    {
      if (infoTag)
        return infoTag->GetBand();
      return "";
    }

    String InfoTagRadioRDS::getArtist()
    {
      if (infoTag)
        return infoTag->GetArtist();
      return "";
    }

    String InfoTagRadioRDS::getComposer()
    {
      if (infoTag)
        return infoTag->GetComposer();
      return "";
    }

    String InfoTagRadioRDS::getConductor()
    {
      if (infoTag)
        return infoTag->GetConductor();
      return "";
    }

    String InfoTagRadioRDS::getAlbum()
    {
      if (infoTag)
        return infoTag->GetAlbum();
      return "";
    }

    String InfoTagRadioRDS::getComment()
    {
      if (infoTag)
        return infoTag->GetComment();
      return "";
    }

    int InfoTagRadioRDS::getAlbumTrackNumber()
    {
      if (infoTag)
        return infoTag->GetAlbumTrackNumber();
      return 0;
    }

    String InfoTagRadioRDS::getInfoNews()
    {
      if (infoTag)
        return infoTag->GetInfoNews();
      return "";
    }

    String InfoTagRadioRDS::getInfoNewsLocal()
    {
      if (infoTag)
        return infoTag->GetInfoNewsLocal();
      return "";
    }

    String InfoTagRadioRDS::getInfoSport()
    {
      if (infoTag)
        return infoTag->GetInfoSport();
      return "";
    }

    String InfoTagRadioRDS::getInfoStock()
    {
      if (infoTag)
        return infoTag->GetInfoStock();
      return "";
    }

    String InfoTagRadioRDS::getInfoWeather()
    {
      if (infoTag)
        return infoTag->GetInfoWeather();
      return "";
    }

    String InfoTagRadioRDS::getInfoHoroscope()
    {
      if (infoTag)
        return infoTag->GetInfoHoroscope();
      return "";
    }

    String InfoTagRadioRDS::getInfoCinema()
    {
      if (infoTag)
        return infoTag->GetInfoCinema();
      return "";
    }

    String InfoTagRadioRDS::getInfoLottery()
    {
      if (infoTag)
        return infoTag->GetInfoLottery();
      return "";
    }

    String InfoTagRadioRDS::getInfoOther()
    {
      if (infoTag)
        return infoTag->GetInfoOther();
      return "";
    }

    String InfoTagRadioRDS::getEditorialStaff()
    {
      if (infoTag)
        return infoTag->GetEditorialStaff();
      return "";
    }

    String InfoTagRadioRDS::getProgStation()
    {
      if (infoTag)
        return infoTag->GetProgStation();
      return "";
    }

    String InfoTagRadioRDS::getProgStyle()
    {
      if (infoTag)
        return infoTag->GetProgStyle();
      return "";
    }

    String InfoTagRadioRDS::getProgHost()
    {
      if (infoTag)
        return infoTag->GetProgHost();
      return "";
    }

    String InfoTagRadioRDS::getProgWebsite()
    {
      if (infoTag)
        return infoTag->GetProgWebsite();
      return "";
    }

    String InfoTagRadioRDS::getProgNow()
    {
      if (infoTag)
        return infoTag->GetProgNow();
      return "";
    }

    String InfoTagRadioRDS::getProgNext()
    {
      if (infoTag)
        return infoTag->GetProgNext();
      return "";
    }

    String InfoTagRadioRDS::getPhoneHotline()
    {
      if (infoTag)
        return infoTag->GetPhoneHotline();
      return "";
    }

    String InfoTagRadioRDS::getEMailHotline()
    {
      if (infoTag)
        return infoTag->GetEMailHotline();
      return "";
    }

    String InfoTagRadioRDS::getPhoneStudio()
    {
      if (infoTag)
        return infoTag->GetPhoneStudio();
      return "";
    }

    String InfoTagRadioRDS::getEMailStudio()
    {
      if (infoTag)
        return infoTag->GetEMailStudio();
      return "";
    }

    String InfoTagRadioRDS::getSMSStudio()
    {
      if (infoTag)
        return infoTag->GetSMSStudio();
      return "";
    }

  }
}

