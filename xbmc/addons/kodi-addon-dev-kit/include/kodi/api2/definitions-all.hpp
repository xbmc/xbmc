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

#ifdef TARGET_WINDOWS
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#include <string>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#define API_LEVELS_ACTIVE 1

extern "C"
{

  ///
  /// \defgroup KodiAddon_CPP_Defs Library definitions
  /// \ingroup c cpp
  /// @{
  /// @brief <b>Library definition values</b>
  ///
  ///

  typedef int ADDON_STATUS;

  //============================================================================
  ///
  /// \ingroup KodiAddon_C_CPP_Defs
  /// @brief Handle identifier pointer
  /// @{
  typedef void* KODI_HANDLE;
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  /// @{
  /// \ingroup KodiAddon_CPP_Defs
  /// @brief KODI API Error Classes
  ///
  /// The error codes are to describe the caused errors on interface.
  ///
  typedef enum KODI_API_Errortype
  {
    /// @brief Successful return code
    API_SUCCESS                 = 0,

    /// @brief Invalid buffer pointer
    API_ERR_BUFFER              = 1,

    /// @brief Invalid count argument
    API_ERR_COUNT               = 2,

    /// @brief Invalid datatype argument
    API_ERR_TYPE                = 3,

    /// @brief Invalid tag argument
    API_ERR_TAG                 = 4,

    /// @brief Invalid communicator
    API_ERR_COMM                = 5,

    /// @brief Invalid rank
    API_ERR_RANK                = 6,

    /// @brief Invalid root
    API_ERR_ROOT                = 7,

    /// @brief Null group passed to function
    API_ERR_GROUP               = 8,

    /// @brief Invalid operation
    API_ERR_OP                  = 9,

    /// @brief Invalid topology
    API_ERR_TOPOLOGY            = 10,

    /// @brief Illegal dimension argument
    API_ERR_DIMS                = 11,

    /// @brief Invalid argument
    API_ERR_ARG                 = 12,

    /// @brief Unknown error
    API_ERR_UNKNOWN             = 13,

    /// @brief message truncated on receive
    API_ERR_TRUNCATE            = 14,

    /// @brief Other error; use Error_string
    API_ERR_OTHER               = 15,

    /// @brief internal error code
    API_ERR_INTERN              = 16,

    /// @brief Look in status for error value
    API_ERR_IN_STATUS           = 17,

    /// @brief Pending request
    API_ERR_PENDING             = 18,

    /// @brief illegal API_request handle
    API_ERR_REQUEST             = 19,

    /// @brief failed to connect
    API_ERR_CONNECTION          = 20,

    /// @brief failed to connect
    API_ERR_VALUE_NOT_AVAILABLE = 21,

    /// @brief Last error code -- always at end
    API_ERR_LASTCODE            = 22
  } KODI_API_Errortype;
  /// @}
  //----------------------------------------------------------------------------


namespace V2
{
namespace KodiAPI
{

  //============================================================================
  ///
  /// \ingroup KodiAddon_CPP_Defs
  /// @brief API level and Add-On to Kodi interface Version.
  ///
  /// Here the values define for the Add-On and also for Kodi the version of the
  /// interface between the both.
  ///
  /// @{

  ///
  /// \ingroup KodiAddon_CPP_Defs
  /// @brief The current add-on interface API level
  ///
  static constexpr const int   KODI_API_Level = 2;       //!< API level version

  ///
  /// \ingroup KodiAddon_CPP_Defs
  /// @brief The complete Version of this interface architecture.
  ///
  static constexpr const char* KODI_API_Version  = "0.0.1"; //!< Code version of add-on interface system

  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup KodiAddon_CPP_Defs
  /// @brief Properties about add-on informations
  ///
  /// This special extension point must be provided by all add-ons, and is the
  /// way that your add-on is described to users of the Kodi add-on manager.
  ///
  /// @note This properties becomes only be used on binary executable add-ons,
  /// which are possible to start independent from Kodi (but Kodi is running).
  /// This is required to inform Kodi about him.
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// int main(int argc, char *argv[])
  /// {
  ///   addon_properties props;
  ///   props.id              = "demo_binary_addon";
  ///   props.type            = "xbmc.addon.executable";
  ///   props.version         = "0.0.1";
  ///   props.name            = "Demo binary add-on";
  ///   props.license         = "GNU GENERAL PUBLIC LICENSE. Version 2, June 1991";
  ///   props.summary         = "";
  ///   props.description     = "Hello World add-on provides some basic examples on how to create your first add-on";
  ///                           " and hopefully will increase the number of Kodi users to start creating their own addons.";
  ///   props.path            = "";
  ///   props.libname         = "";
  ///   props.author          = "";
  ///   props.source          = "http://github.com/someone/myaddon"";
  ///   props.icon            = "";
  ///   props.disclaimer      = "Feel free to use this add-on. For information visit the wiki.";
  ///   props.changelog       = "";
  ///   props.fanart          = "";
  ///   props.is_independent  = false;
  ///   props.use_net_only    = true;
  ///
  ///   if (KodiAPI::Init(argc, argv, &props, "127.0.0.1") != API_SUCCESS)
  ///   {
  ///     fprintf(stderr, "Binary AddOn: %i %s\n", KODI_API_lasterror, KodiAPI_ErrorCodeToString(KODI_API_lasterror));
  ///     return 1;
  ///   }
  ///   ...
  /// ~~~~~~~~~~~~~
  typedef struct addon_properties
  {
    /// @brief Identification name for add-on
    const char* id;

    /// @brief The used type of binary add-on
    ///
    /// Possible types are:
    ///
    /// | Identification name   | Description                                               |
    /// |-----------------------|-----------------------------------------------------------|
    /// | xbmc.addon.executable | A normal executable which match from style not the others |
    /// | xbmc.addon.video      | Video add-on for streams and everything to do there       |
    /// | xbmc.addon.audio      | To support audio for the user after start of him          |
    /// | xbmc.addon.image      | Add-on used to handle images in Kodi                      |
    ///
    const char* type;

    /// @brief Version of the add-on
    const char* version;

    /// @brief The own name.
    ///
    /// The human readable name of add-on used from Kodi e.g. in Log entries.
    ///
    const char* name;

    /// @brief Add-on licence
    ///
    /// The license element indicates what license is used for this add-on.
    ///
    /// ------------------------------------------------------------------------
    ///
    /// ~~~~~~~~~~~~~
    /// license="GNU GENERAL PUBLIC LICENSE. Version 2, June 1991";
    /// ~~~~~~~~~~~~~
    const char* license;


    /// @brief Summary add-on description
    ///
    /// The element provide a short summary of what the add-on does. This should
    /// be a single sentence.
    ///
    /// ------------------------------------------------------------------------
    ///
    /// ~~~~~~~~~~~~~
    /// summary="Hello World add-on provides some basic examples on how to create your first add-on.";
    /// ~~~~~~~~~~~~~
    const char* summary;

    /// @brief Detailed add-on description
    ///
    /// The element provide a more detailed summary of what the add-on does.
    ///
    /// ------------------------------------------------------------------------
    ///
    /// ~~~~~~~~~~~~~
    /// description="Hello World add-on provides some basic examples on how to create your first add-on"
    ///             " and hopefully will increase the number of Kodi users to start creating their own addons.";
    /// ~~~~~~~~~~~~~
    const char* description;

    /// @brief Add-on path (optional)
    const char* path;

    /// @brief Add-on executable name (optional)
    const char* libname;

    /// @brief Author of add-on
    ///
    /// Name of the man or company who created the add-on.
    const char* author;

    /// @brief Source URL (optional)
    ///
    /// The source element provides the URL for the source code for this specific
    /// add-on.
    ///
    /// -------------------------------------------------------------------------
    ///
    /// ~~~~~~~~~~~~~
    /// source="http://github.com/someone/myaddon";
    /// ~~~~~~~~~~~~~
    const char* source;

    /// @brief Add-on icon
    ///
    /// A PNG icon for the add-on. It can be 256x256 or  512x512 pixels big.  Try
    /// to make it look nice!
    const char* icon;

    /// @brief Disclaimer of add-on
    ///
    /// The element that indicate what (if any) things the user should know about
    /// the add-on. There is no need to have a disclaimer if you don't want  one,
    /// though if something requires  settings, or only  works  in  a  particular
    /// country then you may want to state this here.
    ///
    /// ------------------------------------------------------------------------
    ///
    /// ~~~~~~~~~~~~~
    /// disclaimer="Feel free to use this add-on. For information visit the wiki.";
    /// ~~~~~~~~~~~~~
    const char* disclaimer;

    /// @brief Change log of add-on (optional)
    ///
    /// Description of changes on add-on versions to allow show on Kodi's GUI for
    /// the user.
    const char* changelog;

    /// @brief Fan art Image
    ///
    /// Fan  art,  also  known  as  Backdrops, are high  quality artwork  that is
    /// displayed in the background as wallpapers on add-on GUI menus.
    const char* fanart;

    /// @brief To inform add-on is independent from Kodi.
    ///
    /// To  inform Kodi  that add-on  is independent and is not  stopped by  Kodi
    /// after calling "KodiAPI_Finalize".
    ///
    bool        is_independent;

    /// @brief Option to create interface only over network.
    ///
    /// @note Should be  ignored and set  to false.  Is passicly  used  for  test
    /// purpose only.
    ///
    bool        use_net_only;
  } addon_properties;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup KodiAddon_CPP_Defs
  /// @brief Log file
  ///
  /// At some point during your foray into Kodi, you will likely come up against
  /// a problem that isn't made 100% clear from errors in the GUI. This is where
  /// the log file comes into play. Kodi writes all sorts of useful stuff to its
  /// log, which is why it should be included with every bug/problem report.
  /// Don't be afraid of its contents! Often a quick glance through the log will
  /// turn up a simple typo or missing file which you can easily fix on your own.
  ///
  /// Each event is logged to one line of the Kodi log file which is formatted as
  /// follows...
  ///
  /// ~~~~~~~~~~~~~
  /// [TIMESTAMP] T:[THREADID] M:[FREEMEM] [LEVEL]: [MESSAGE]
  /// ~~~~~~~~~~~~~
  ///
  /// | Identification name   | Description                                               |
  /// |-----------------------|-----------------------------------------------------------|
  /// | TIMESTAMP             | The wall time at which the event occurred.
  /// | THREADID              | The thread identification number of the thread in which the event occurred.
  /// | FREEMEM               | The amount of memory (in bytes) free at the time of the event.
  /// | LEVEL                 | The severity of the event.
  /// | MESSAGE               | A brief description and/or important information about the event.
  ///
  typedef enum addon_log
  {
    /// @brief Log level "Debug"
    ///
    /// In  depth  informatio n about  the  status  of  Kodi.  This  information
    /// can  pretty  much only be  deciphered  by a developer or  long time Kodi
    /// power user.
    ///
    ADDON_LOG_DEBUG,

    /// @brief Log level "Info"
    ///
    /// Something  has happened. It's not  a problem, we just  thought you might
    /// want to know. Fairly excessive output that most people won't care about.
    ///
    ADDON_LOG_INFO,

    /// @brief Log level "Notice"
    ///
    /// Similar  to  INFO but  the average  Joe might  want to  know about these
    /// events. This level and above are logged by default.
    ///
    ADDON_LOG_NOTICE,

    /// @brief Log level "Warning"
    ///
    /// Something potentially bad has happened. If Kodi did something you didn't
    /// expect, this is probably why. Watch for errors to follow.
    ///
    ADDON_LOG_WARNING,

    /// @brief Log level "Error"
    ///
    /// This event is bad.  Something has failed.  You  likely noticed  problems
    /// with the application be it skin artifacts, failure of playback a crash,
    /// etc.
    ///
    ADDON_LOG_ERROR,

    /// @brief Log level "Severe"
    ///
    ADDON_LOG_SEVERE,

    /// @brief Log level "Fatal"
    ///
    /// We're screwed. Kodi's add-on is about to crash.
    ///
    ADDON_LOG_FATAL
  } addon_log;
  //----------------------------------------------------------------------------

  ///
  /// @brief Platform dependent path separator
  ///
  #ifndef PATH_SEPARATOR_CHAR
    #if (defined(_WIN32) || defined(_WIN64))
      #define PATH_SEPARATOR_CHAR '\\'
    #else
      #define PATH_SEPARATOR_CHAR '/'
    #endif
  #endif

  #if !defined(__stat64)
    #if defined(__APPLE__)
      #define __stat64 stat
    #else
      #define __stat64 stat64
    #endif
  #endif

}; /* namespace KodiAPI */
}; /* namespace V2 */
}; /* extern "C" */
