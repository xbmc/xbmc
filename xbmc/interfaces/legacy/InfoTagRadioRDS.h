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
    //
    /// \defgroup python_InfoTagRadioRDS InfoTagRadioRDS
    /// \ingroup python_xbmc
    /// @{
    /// @brief <b>Kodi's radio RDS info tag class.</b>
    ///
    /// To get radio RDS info tag data of currently played PVR radio channel source.
    ///
    /// @note Info tag load is only be possible from present player class.\n
    /// Also is all the data variable from radio channels and not known on begining
    /// of radio receiving.
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// tag = xbmc.Player().getRadioRDSInfoTag()
    ///
    /// title  = tag.getTitle()
    /// artist = tag.getArtist()
    /// ...
    /// ~~~~~~~~~~~~~
    //
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

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Title of the item on the air; i.e. song title.
      ///
      /// @return Title
      ///
      String getTitle();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Band of the item on air.
      ///
      /// @return Band
      ///
      String getBand();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Artist of the item on air.
      ///
      /// @return Artist
      ///
      String getArtist();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get the Composer of the music.
      ///
      /// @return Composer
      ///
      String getComposer();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get the Conductor of the Band.
      ///
      /// @return Conductor
      ///
      String getConductor();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Album of item on air.
      ///
      /// @return Album name
      ///
      String getAlbum();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Comment text from channel.
      ///
      /// @return Comment
      ///
      String getComment();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get the album track number of currently sended music.
      ///
      /// @return Track Number
      ///
      int getAlbumTrackNumber();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get News informations.
      ///
      /// @return News Information
      ///
      String getInfoNews();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Local news informations.
      ///
      /// @return Local News Information
      ///
      String getInfoNewsLocal();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Sport informations.
      ///
      /// @return Sport Information
      ///
      String getInfoSport();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Stock informations.
      ///
      /// @return Stock Information
      ///
      String getInfoStock();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Weather informations.
      ///
      /// @return Weather Information
      ///
      String getInfoWeather();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Horoscope informations.
      ///
      /// @return Horoscope Information
      ///
      String getInfoHoroscope();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Cinema informations.
      ///
      /// @return Cinema Information
      ///
      String getInfoCinema();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Lottery informations.
      ///
      /// @return Lottery Information
      ///
      String getInfoLottery();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get other informations.
      ///
      /// @return Other Information
      ///
      String getInfoOther();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Get Editorial Staff names.
      ///
      /// @return Editorial Staff
      ///
      String getEditorialStaff();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Name describing station.
      ///
      /// @return Program Station
      ///
      String getProgStation();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief The the radio channel style currently used.
      ///
      /// @return Program Style
      ///
      String getProgStyle();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Host of current radio show.
      ///
      /// @return Program Host
      ///
      String getProgHost();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Link to URL (web page) for radio station homepage.
      ///
      /// @return Program Website
      ///
      String getProgWebsite();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Current radio program show.
      ///
      /// @return Program Now
      ///
      String getProgNow();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Next program show.
      ///
      /// @return Program Next
      ///
      String getProgNext();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Telephone number of the radio station's hotline.
      ///
      /// @return Phone Hotline
      ///
      String getPhoneHotline();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Email address of the radio station's studio.
      ///
      /// @return EMail Hotline
      ///
      String getEMailHotline();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Telephone number of the radio station's studio.
      ///
      /// @return Phone Studio
      ///
      String getPhoneStudio();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief Email address of radio station studio.
      ///
      /// @return EMail Studio
      ///
      String getEMailStudio();

      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief SMS (Text Messaging) number for studio.
      ///
      /// @return SMS Studio
      ///
      String getSMSStudio();

    };
    //@}
  }
}
