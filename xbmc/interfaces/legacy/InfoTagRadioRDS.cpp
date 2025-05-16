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

    String InfoTagRadioRDS::getTitle() const {
      if (infoTag)
        return infoTag->GetTitle();
      return "";
    }

    String InfoTagRadioRDS::getBand() const {
      if (infoTag)
        return infoTag->GetBand();
      return "";
    }

    String InfoTagRadioRDS::getArtist() const {
      if (infoTag)
        return infoTag->GetArtist();
      return "";
    }

    String InfoTagRadioRDS::getComposer() const {
      if (infoTag)
        return infoTag->GetComposer();
      return "";
    }

    String InfoTagRadioRDS::getConductor() const {
      if (infoTag)
        return infoTag->GetConductor();
      return "";
    }

    String InfoTagRadioRDS::getAlbum() const {
      if (infoTag)
        return infoTag->GetAlbum();
      return "";
    }

    String InfoTagRadioRDS::getComment() const {
      if (infoTag)
        return infoTag->GetComment();
      return "";
    }

    int InfoTagRadioRDS::getAlbumTrackNumber() const {
      if (infoTag)
        return infoTag->GetAlbumTrackNumber();
      return 0;
    }

    String InfoTagRadioRDS::getInfoNews() const {
      if (infoTag)
        return infoTag->GetInfoNews();
      return "";
    }

    String InfoTagRadioRDS::getInfoNewsLocal() const {
      if (infoTag)
        return infoTag->GetInfoNewsLocal();
      return "";
    }

    String InfoTagRadioRDS::getInfoSport() const {
      if (infoTag)
        return infoTag->GetInfoSport();
      return "";
    }

    String InfoTagRadioRDS::getInfoStock() const {
      if (infoTag)
        return infoTag->GetInfoStock();
      return "";
    }

    String InfoTagRadioRDS::getInfoWeather() const {
      if (infoTag)
        return infoTag->GetInfoWeather();
      return "";
    }

    String InfoTagRadioRDS::getInfoHoroscope() const {
      if (infoTag)
        return infoTag->GetInfoHoroscope();
      return "";
    }

    String InfoTagRadioRDS::getInfoCinema() const {
      if (infoTag)
        return infoTag->GetInfoCinema();
      return "";
    }

    String InfoTagRadioRDS::getInfoLottery() const {
      if (infoTag)
        return infoTag->GetInfoLottery();
      return "";
    }

    String InfoTagRadioRDS::getInfoOther() const {
      if (infoTag)
        return infoTag->GetInfoOther();
      return "";
    }

    String InfoTagRadioRDS::getEditorialStaff() const {
      if (infoTag)
        return infoTag->GetEditorialStaff();
      return "";
    }

    String InfoTagRadioRDS::getProgStation() const {
      if (infoTag)
        return infoTag->GetProgStation();
      return "";
    }

    String InfoTagRadioRDS::getProgStyle() const {
      if (infoTag)
        return infoTag->GetProgStyle();
      return "";
    }

    String InfoTagRadioRDS::getProgHost() const {
      if (infoTag)
        return infoTag->GetProgHost();
      return "";
    }

    String InfoTagRadioRDS::getProgWebsite() const {
      if (infoTag)
        return infoTag->GetProgWebsite();
      return "";
    }

    String InfoTagRadioRDS::getProgNow() const {
      if (infoTag)
        return infoTag->GetProgNow();
      return "";
    }

    String InfoTagRadioRDS::getProgNext() const {
      if (infoTag)
        return infoTag->GetProgNext();
      return "";
    }

    String InfoTagRadioRDS::getPhoneHotline() const {
      if (infoTag)
        return infoTag->GetPhoneHotline();
      return "";
    }

    String InfoTagRadioRDS::getEMailHotline() const {
      if (infoTag)
        return infoTag->GetEMailHotline();
      return "";
    }

    String InfoTagRadioRDS::getPhoneStudio() const {
      if (infoTag)
        return infoTag->GetPhoneStudio();
      return "";
    }

    String InfoTagRadioRDS::getEMailStudio() const {
      if (infoTag)
        return infoTag->GetEMailStudio();
      return "";
    }

    String InfoTagRadioRDS::getSMSStudio() const {
      if (infoTag)
        return infoTag->GetSMSStudio();
      return "";
    }

  }
}

