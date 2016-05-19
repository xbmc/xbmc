#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include "../definitions-all.hpp"

API_NAMESPACE

namespace KodiAPI
{
extern "C"
{

  /*!
  \defgroup CPP_KodiAPI_AddOn 2. AddOn
  \ingroup cpp
  \brief <b><em>Basic functions and classes to have access on Add-on to Kodi</em></b>
  */

  //============================================================================
  /// \ingroup
  /// @brief
  /// @{

  #define DVD_TIME_BASE 1000000

  //TODO original definition is in DVDClock.h
  #define DVD_NOPTS_VALUE 0xFFF0000000000000
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup CPP_KodiAPI_AddOn_General_Defs
  /// @brief For CAddonLib_General::QueueNotification used message types
  ///
  typedef enum queue_msg
  {
    /// Show info notification message
    QUEUE_INFO,
    /// Show warning notification message
    QUEUE_WARNING,
    /// Show error notification message
    QUEUE_ERROR
  } queue_msg;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup CPP_KodiAPI_AddOn_General_Defs
  /// @enum dvd_state State values about optical drive
  ///
  typedef enum dvd_state
  {
    ///
    ADDON_DRIVE_NOT_READY           = 0x01,
    ///
    ADDON_DRIVE_CLOSED_NO_MEDIA     = 0x03,
    ///
    ADDON_TRAY_OPEN                 = 0x10,
    ///
    ADDON_TRAY_CLOSED_NO_MEDIA      = 0x40,
    ///
    ADDON_TRAY_CLOSED_MEDIA_PRESENT = 0x60
  } dvd_state;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup CPP_KodiAPI_AddOn_General_Defs
  /// @brief Format codes to get string from them.
  ///
  typedef enum lang_formats
  {
    /// two letter code as defined in ISO 639-1
    LANG_FMT_ISO_639_1,
    /// three letter code as defined in ISO 639-2/T or ISO 639-2/B
    LANG_FMT_ISO_639_2,
    /// full language name in English
    LANG_FMT_ENGLISH_NAME
  } lang_formats;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup CPP_KodiAPI_AddOn_General_Defs
  /// @brief Kodi server identificators
  ///
  typedef enum eservers
  {
    /// [To control Kodi's builtin webserver](http://kodi.wiki/view/Webserver)
    ADDON_ES_WEBSERVER = 1,
    /// [AirPlay is a proprietary protocol stack/suite developed by Apple Inc.](http://kodi.wiki/view/AirPlay)
    ADDON_ES_AIRPLAYSERVER,
    /// [Control JSON-RPC HTTP/TCP socket-based interface](http://kodi.wiki/view/JSON-RPC_API)
    ADDON_ES_JSONRPCSERVER,
    /// [UPnP client (aka UPnP renderer)](http://kodi.wiki/view/UPnP/Client)
    ADDON_ES_UPNPRENDERER,
    /// [Control built-in UPnP A/V media server (UPnP-server)](http://kodi.wiki/view/UPnP/Server)
    ADDON_ES_UPNPSERVER,
    /// [Set eventServer part that accepts remote device input on all platforms](http://kodi.wiki/view/EventServer)
    ADDON_ES_EVENTSERVER,
    /// [Control Kodi's Avahi Zeroconf](http://kodi.wiki/view/Zeroconf)
    ADDON_ES_ZEROCONF
  } eservers;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup CPP_KodiAPI_AddOn_General_Defs
  /// @brief For CAddonLib_General::KodiVersion used structure
  ///
  typedef struct kodi_version
  {
    /// Application name, normally 'Kodi'
    std::string compile_name;
    /// Major code version of Kodi
    int         major;
    /// Minor code version of Kodi
    int         minor;
    /// The Revision contains a id and the build date, e.g. 2015-11-30-74edffb-dirty
    std::string revision;
    /// The version canditate e.g. alpha, beta or release
    std::string tag;
    /// The revision of tag before
    std::string tag_revision;
  } kodi_version;
  //----------------------------------------------------------------------------


  //============================================================================
  /// @ingroup CPP_KodiAPI_AddOn_SoundPlay_Defs
  /// @brief For class CAddonLib_SoundPlay used values
  ///
  /// The values are used to identify a channel and can also serve as a pointer
  /// to memory can be used. Currently supported Kodi not all listed position
  /// and are only available in order to avoid major changes after it is placed.
  ///
  typedef enum audio_channel
  {
    /// Invalid channel identifier, also used on CAddonLib_SoundPlay to reset selection
    AUDIO_CH_INVALID = -1,
    /// Front left
    AUDIO_CH_FL = 0,
    /// Front right
    AUDIO_CH_FR,
    /// Front center
    AUDIO_CH_FC,
    /// LFE (Bass)
    AUDIO_CH_LFE,
    /// Back left
    AUDIO_CH_BL,
    /// Back right
    AUDIO_CH_BR,
    /// Front left over center (currently not supported on Kodi)
    AUDIO_CH_FLOC,
    /// Front right over center (currently not supported on Kodi)
    AUDIO_CH_FROC,
    /// Back center
    AUDIO_CH_BC,
    /// Side left
    AUDIO_CH_SL,
    /// Side right
    AUDIO_CH_SR,
    /// Top front left (currently not supported on Kodi)
    AUDIO_CH_TFL,
    /// Top front right (currently not supported on Kodi)
    AUDIO_CH_TFR,
    /// Top front center (currently not supported on Kodi)
    AUDIO_CH_TFC,
    /// Top center (currently not supported on Kodi)
    AUDIO_CH_TC,
    /// Top back left (currently not supported on Kodi)
    AUDIO_CH_TBL,
    /// Top back right (currently not supported on Kodi)
    AUDIO_CH_TBR,
    /// Top back center (currently not supported on Kodi)
    AUDIO_CH_TBC,
    /// Back left over center (currently not supported on Kodi)
    AUDIO_CH_BLOC,
    /// Back right over center (currently not supported on Kodi)
    AUDIO_CH_BROC,
    /// Used as max value for a array size, not a position
    AUDIO_CH_MAX
  } audio_channel;
  //----------------------------------------------------------------------------

  //============================================================================
  /// \ingroup CPP_KodiAPI_AddOn_General_Defs
  /// @brief Used CURL message types
  ///
  typedef enum ADDON_CURLOPTIONTYPE
  {
    /// Set a general option
    ADDON_CURL_OPTION_OPTION,
    /// Set a protocol option
    ADDON_CURL_OPTION_PROTOCOL,
    /// Set User and password
    ADDON_CURL_OPTION_CREDENTIALS,
    /// Add a Header
    ADDON_CURL_OPTION_HEADER
  } ADDON_CURLOPTIONTYPE;
  //----------------------------------------------------------------------------

} /* extern "C" */
} /* namespace KodiAPI */

END_NAMESPACE()
