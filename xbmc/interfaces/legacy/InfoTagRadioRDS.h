/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"

#include <memory>

namespace PVR
{
class CPVRChannel;
class CPVRRadioRDSInfoTag;
}

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
    /// Also is all the data variable from radio channels and not known on beginning
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
      std::shared_ptr<PVR::CPVRRadioRDSInfoTag> infoTag;

    public:
#ifndef SWIG
      explicit InfoTagRadioRDS(const std::shared_ptr<PVR::CPVRChannel>& channel);
#endif
      InfoTagRadioRDS();
      ~InfoTagRadioRDS() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getTitle() }
      /// Title of the item on the air; i.e. song title.
      ///
      /// @return Title
      ///
      getTitle();
#else
      String getTitle() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getBand() }
      /// Band of the item on air.
      ///
      /// @return Band
      ///
      getBand();
#else
      String getBand() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getArtist() }
      /// Artist of the item on air.
      ///
      /// @return Artist
      ///
      getArtist();
#else
      String getArtist() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getComposer() }
      /// Get the Composer of the music.
      ///
      /// @return Composer
      ///
      getComposer();
#else
      String getComposer() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getConductor() }
      /// Get the Conductor of the Band.
      ///
      /// @return Conductor
      ///
      getConductor();
#else
      String getConductor() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getAlbum() }
      /// Album of item on air.
      ///
      /// @return Album name
      ///
      getAlbum();
#else
      String getAlbum() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getComment() }
      /// Get Comment text from channel.
      ///
      /// @return Comment
      ///
      getComment();
#else
      String getComment() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getAlbumTrackNumber() }
      /// Get the album track number of currently sended music.
      ///
      /// @return Track Number
      ///
      getAlbumTrackNumber();
#else
      int getAlbumTrackNumber() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoNews() }
      /// Get News informations.
      ///
      /// @return News Information
      ///
      getInfoNews();
#else
      String getInfoNews() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoNewsLocal() }
      /// Get Local news informations.
      ///
      /// @return Local News Information
      ///
      getInfoNewsLocal();
#else
      String getInfoNewsLocal() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoSport() }
      /// Get Sport informations.
      ///
      /// @return Sport Information
      ///
      getInfoSport();
#else
      String getInfoSport() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoStock() }
      /// Get Stock informations.
      ///
      /// @return Stock Information
      ///
      getInfoStock();
#else
      String getInfoStock() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoWeather() }
      /// Get Weather informations.
      ///
      /// @return Weather Information
      ///
      getInfoWeather();
#else
      String getInfoWeather() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoHoroscope() }
      /// Get Horoscope informations.
      ///
      /// @return Horoscope Information
      ///
      getInfoHoroscope();
#else
      String getInfoHoroscope() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoCinema() }
      /// Get Cinema informations.
      ///
      /// @return Cinema Information
      ///
      getInfoCinema();
#else
      String getInfoCinema() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoLottery() }
      /// Get Lottery informations.
      ///
      /// @return Lottery Information
      ///
      getInfoLottery();
#else
      String getInfoLottery() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getInfoOther() }
      /// Get other informations.
      ///
      /// @return Other Information
      ///
      getInfoOther();
#else
      String getInfoOther() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getEditorialStaff() }
      /// Get Editorial Staff names.
      ///
      /// @return Editorial Staff
      ///
      getEditorialStaff();
#else
      String getEditorialStaff() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgStation() }
      /// Name describing station.
      ///
      /// @return Program Station
      ///
      getProgStation();
#else
      String getProgStation() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgStyle() }
      /// The the radio channel style currently used.
      ///
      /// @return Program Style
      ///
      getProgStyle();
#else
      String getProgStyle() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgHost() }
      /// Host of current radio show.
      ///
      /// @return Program Host
      ///
      getProgHost();
#else
      String getProgHost() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgWebsite() }
      /// Link to URL (web page) for radio station homepage.
      ///
      /// @return Program Website
      ///
      getProgWebsite();
#else
      String getProgWebsite() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgNow() }
      /// Current radio program show.
      ///
      /// @return Program Now
      ///
      getProgNow();
#else
      String getProgNow() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getProgNext() }
      /// Next program show.
      ///
      /// @return Program Next
      ///
      getProgNext();
#else
      String getProgNext() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getPhoneHotline() }
      /// Telephone number of the radio station's hotline.
      ///
      /// @return Phone Hotline
      ///
      getPhoneHotline();
#else
      String getPhoneHotline() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getEMailHotline() }
      /// Email address of the radio station's studio.
      ///
      /// @return EMail Hotline
      ///
      getEMailHotline();
#else
      String getEMailHotline() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getPhoneStudio() }
      /// Telephone number of the radio station's studio.
      ///
      /// @return Phone Studio
      ///
      getPhoneStudio();
#else
      String getPhoneStudio() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getEMailStudio() }
      /// Email address of radio station studio.
      ///
      /// @return EMail Studio
      ///
      getEMailStudio();
#else
      String getEMailStudio() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// @ingroup python_InfoTagRadioRDS
      /// @brief \python_func{ getSMSStudio() }
      /// SMS (Text Messaging) number for studio.
      ///
      /// @return SMS Studio
      ///
      getSMSStudio();
#else
      String getSMSStudio() const;
#endif
    };
    //@}
  }
}
