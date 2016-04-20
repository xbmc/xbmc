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
    /// @brief **Kodi's radio RDS info tag class.**
    ///
    /// \python_class{ InfoTagRadioRDS() }
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

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getTitle() }
      ///-----------------------------------------------------------------------
      /// Title of the item on the air; i.e. song title.
      ///
      /// @return Title
      ///
      getTitle();
#else
      String getTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getBand() }
      ///-----------------------------------------------------------------------
      /// Band of the item on air.
      ///
      /// @return Band
      ///
      getBand();
#else
      String getBand();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getArtist() }
      ///-----------------------------------------------------------------------
      /// Artist of the item on air.
      ///
      /// @return Artist
      ///
      getArtist();
#else
      String getArtist();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getComposer() }
      ///-----------------------------------------------------------------------
      /// Get the Composer of the music.
      ///
      /// @return Composer
      ///
      getComposer();
#else
      String getComposer();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getConductor() }
      ///-----------------------------------------------------------------------
      /// Get the Conductor of the Band.
      ///
      /// @return Conductor
      ///
      getConductor();
#else
      String getConductor();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getAlbum() }
      ///-----------------------------------------------------------------------
      /// Album of item on air.
      ///
      /// @return Album name
      ///
      getAlbum();
#else
      String getAlbum();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getComment() }
      ///-----------------------------------------------------------------------
      /// Get Comment text from channel.
      ///
      /// @return Comment
      ///
      getComment();
#else
      String getComment();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getAlbumTrackNumber() }
      ///-----------------------------------------------------------------------
      /// Get the album track number of currently sended music.
      ///
      /// @return Track Number
      ///
      getAlbumTrackNumber();
#else
      int getAlbumTrackNumber();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoNews() }
      ///-----------------------------------------------------------------------
      /// Get News informations.
      ///
      /// @return News Information
      ///
      getInfoNews();
#else
      String getInfoNews();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoNewsLocal() }
      ///-----------------------------------------------------------------------
      /// Get Local news informations.
      ///
      /// @return Local News Information
      ///
      getInfoNewsLocal();
#else
      String getInfoNewsLocal();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoSport() }
      ///-----------------------------------------------------------------------
      /// Get Sport informations.
      ///
      /// @return Sport Information
      ///
      getInfoSport();
#else
      String getInfoSport();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoStock() }
      ///-----------------------------------------------------------------------
      /// Get Stock informations.
      ///
      /// @return Stock Information
      ///
      getInfoStock();
#else
      String getInfoStock();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoWeather() }
      ///-----------------------------------------------------------------------
      /// Get Weather informations.
      ///
      /// @return Weather Information
      ///
      getInfoWeather();
#else
      String getInfoWeather();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoHoroscope() }
      ///-----------------------------------------------------------------------
      /// Get Horoscope informations.
      ///
      /// @return Horoscope Information
      ///
      getInfoHoroscope();
#else
      String getInfoHoroscope();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoCinema() }
      ///-----------------------------------------------------------------------
      /// Get Cinema informations.
      ///
      /// @return Cinema Information
      ///
      getInfoCinema();
#else
      String getInfoCinema();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoLottery() }
      ///-----------------------------------------------------------------------
      /// Get Lottery informations.
      ///
      /// @return Lottery Information
      ///
      getInfoLottery();
#else
      String getInfoLottery();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoOther() }
      ///-----------------------------------------------------------------------
      /// Get other informations.
      ///
      /// @return Other Information
      ///
      getInfoOther();
#else
      String getInfoOther();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getEditorialStaff() }
      ///-----------------------------------------------------------------------
      /// Get Editorial Staff names.
      ///
      /// @return Editorial Staff
      ///
      getEditorialStaff();
#else
      String getEditorialStaff();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgStation() }
      ///-----------------------------------------------------------------------
      /// Name describing station.
      ///
      /// @return Program Station
      ///
      getProgStation();
#else
      String getProgStation();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgStyle() }
      ///-----------------------------------------------------------------------
      /// The the radio channel style currently used.
      ///
      /// @return Program Style
      ///
      getProgStyle();
#else
      String getProgStyle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgHost() }
      ///-----------------------------------------------------------------------
      /// Host of current radio show.
      ///
      /// @return Program Host
      ///
      getProgHost();
#else
      String getProgHost();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgWebsite() }
      ///-----------------------------------------------------------------------
      /// Link to URL (web page) for radio station homepage.
      ///
      /// @return Program Website
      ///
      getProgWebsite();
#else
      String getProgWebsite();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgNow() }
      ///-----------------------------------------------------------------------
      /// Current radio program show.
      ///
      /// @return Program Now
      ///
      getProgNow();
#else
      String getProgNow();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgNext() }
      ///-----------------------------------------------------------------------
      /// Next program show.
      ///
      /// @return Program Next
      ///
      getProgNext();
#else
      String getProgNext();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getPhoneHotline() }
      ///-----------------------------------------------------------------------
      /// Telephone number of the radio station's hotline.
      ///
      /// @return Phone Hotline
      ///
      getPhoneHotline();
#else
      String getPhoneHotline();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getEMailHotline() }
      ///-----------------------------------------------------------------------
      /// Email address of the radio station's studio.
      ///
      /// @return EMail Hotline
      ///
      getEMailHotline();
#else
      String getEMailHotline();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getPhoneStudio() }
      ///-----------------------------------------------------------------------
      /// Telephone number of the radio station's studio.
      ///
      /// @return Phone Studio
      ///
      getPhoneStudio();
#else
      String getPhoneStudio();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getEMailStudio() }
      ///-----------------------------------------------------------------------
      /// Email address of radio station studio.
      ///
      /// @return EMail Studio
      ///
      getEMailStudio();
#else
      String getEMailStudio();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getSMSStudio() }
      ///-----------------------------------------------------------------------
      /// SMS (Text Messaging) number for studio.
      ///
      /// @return SMS Studio
      ///
      getSMSStudio();
#else
      String getSMSStudio();
#endif
    };
    //@}
  }
}
