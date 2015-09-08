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

#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "AddonClass.h"

#pragma once

namespace XBMCAddon
{
  namespace xbmc
  {
    /**
     * InfoTagRadioRDS class.\n
     */
    class InfoTagRadioRDS : public AddonClass
    {
    private:
      PVR::CPVRRadioRDSInfoTagPtr infoTag;

    public:
#ifndef SWIG
      InfoTagRadioRDS(const PVR::CPVRRadioRDSInfoTagPtr tag);
#endif
      InfoTagRadioRDS();
      virtual ~InfoTagRadioRDS();

      /**
       * getTitle() -- returns a string.\n
       */
      String getTitle();
      /**
       * getBand() -- returns a string.\n
       */
      String getBand();
      /**
       * getArtist() -- returns a string.\n
       */
      String getArtist();
      /**
       * getComposer() -- returns a string.\n
       */
      String getComposer();
      /**
       * getConductor() -- returns a string.\n
       */
      String getConductor();
      /**
       * getAlbum() -- returns a string.\n
       */
      String getAlbum();
      /**
       * getComment() -- returns a string.\n
       */
      String getComment();
      /**
       * getAlbumTrackNumber() -- returns a integer.\n
       */
      int getAlbumTrackNumber();
      /**
       * getInfoNews() -- returns an string.\n
       */
      String getInfoNews();
      /**
       * getInfoNewsLocal() -- returns a string.\n
       */
      String getInfoNewsLocal();
      /**
       * getInfoSport() -- returns a string.\n
       */
      String getInfoSport();
      /**
       * getInfoStock() -- returns a string.\n
       */
      String getInfoStock();
      /**
       * getInfoWeather() -- returns a string.\n
       */
      String getInfoWeather();
      /**
       * getInfoHoroscope() -- returns a string.\n
       */
      String getInfoHoroscope();
      /**
       * getInfoCinema() -- returns a string.\n
       */
      String getInfoCinema();
      /**
       * getInfoLottery() -- returns a string.\n
       */
      String getInfoLottery();
      /**
       * getInfoOther() -- returns a string.\n
       */
      String getInfoOther();
      /**
       * getEditorialStaff() -- returns a string.\n
       */
      String getEditorialStaff();
      /**
       * getProgStation() -- returns a string.\n
       */
      String getProgStation();
      /**
       * getProgStyle() -- returns a string.\n
       */
      String getProgStyle();
      /**
       * getProgHost() -- returns a string.\n
       */
      String getProgHost();
      /**
       * getProgWebsite() -- returns a string.\n
       */
      String getProgWebsite();
      /**
       * getProgNow() -- returns a string.\n
       */
      String getProgNow();
      /**
       * getProgNext() -- returns a string.\n
       */
      String getProgNext();
      /**
       * getPhoneHotline() -- returns a string.\n
       */
      String getPhoneHotline();
      /**
       * getEMailHotline() -- returns a string.\n
       */
      String getEMailHotline();
      /**
       * getPhoneStudio() -- returns a string.\n
       */
      String getPhoneStudio();
      /**
       * getEMailStudio() -- returns a string.\n
       */
      String getEMailStudio();
      /**
       * getSMSStudio() -- returns a string.\n
       */
      String getSMSStudio();

    };
  }
}

