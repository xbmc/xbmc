/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoTagRadioRDS.h"

#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "utils/StringUtils.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    InfoTagRadioRDS::InfoTagRadioRDS() = default;

    InfoTagRadioRDS::InfoTagRadioRDS(const std::shared_ptr<PVR::CPVRChannel>& channel)
    {
      if (channel)
        infoTag = channel->GetRadioRDSInfoTag();
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

