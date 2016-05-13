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

#if !defined(__stat64)
  #if defined(__APPLE__)
    #define __stat64 stat
  #else
    #define __stat64 stat64
  #endif
#endif

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

API_NAMESPACE

namespace KodiAPI
{

  //============================================================================
  ///
  /// \ingroup KodiAddon_CPP_Defs KodiAddon_CPP_Main_Defs
  /// @brief API level and Add-On to Kodi interface Version.
  ///
  /// Here the values define for the Add-On and also for Kodi the version of the
  /// interface between the both.
  ///
  /// @{

  ///
  /// \ingroup KodiAddon_CPP_Defs KodiAddon_CPP_Main_Defs
  /// @brief The current add-on interface API level
  ///
  extern int KODI_API_Level;           //!< API level version

  ///
  /// \ingroup KodiAddon_CPP_Defs KodiAddon_CPP_Main_Defs
  /// @brief The complete Version of this interface architecture.
  ///
  extern const char* KODI_API_Version; //!< Code version of add-on interface system

  /// @}
  //----------------------------------------------------------------------------

  typedef int ADDON_STATUS;

  //============================================================================
  ///
  /// \ingroup KodiAddon_CPP_Defs KodiAddon_CPP_Main_Defs
  /// @brief Handle identifier pointer
  /// @{
  typedef void* KODI_HANDLE;
  /// @}
  //----------------------------------------------------------------------------


  //============================================================================
  /// @{
  /// \ingroup KodiAddon_CPP_Defs KodiAddon_CPP_Main_Defs
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


  //============================================================================
  /// \ingroup KodiAddon_CPP_Defs KodiAddon_CPP_Main_Defs
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

} /* namespace KodiAPI */

END_NAMESPACE()

} /* extern "C" */
