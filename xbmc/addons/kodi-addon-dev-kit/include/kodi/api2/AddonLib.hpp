#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "definitions.hpp"
#include <string>

namespace V2
{
namespace KodiAPI
{
extern "C"
{

/*\_____________________________________________________________________________
| |
| | AddOn development guide for AddOnLIB used for doxygen
| |_____________________________________________________________________________
\*/
/*!

\defgroup AddOnLIB_CPP Library - AddOnLIB
\ingroup cpp
\brief <b><em>Primary add-on helper library with general functions</em></b>

------------------------------------------------------------------------

**Code example:**

Below is a code example do show what must be done on add-on base function to
become support for library:

~~~~~~~~~~~~~{.cpp}
#include <kodi/api2/AddonLib.hpp>

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  // Enable access to all add-on to Kodi functions
  if (KodiAPI::InitLibAddon(hdl) != API_SUCCESS)
    return ADDON_STATUS_PERMANENT_FAILURE;
  ...

  // Example lib call
  KodiAPI::Log(LOG_INFO, "My add-on creation done");

  return ADDON_STATUS_OK;
}

void ADDON_Destroy()
{
  ...
  KodiAPI::Finalize();
  ...
}
~~~~~~~~~~~~~

 */
/*\_____________________________________________________________________________
\*/

  ///
  /// \defgroup KodiAddon_CPP_Main
  /// \ingroup AddOnLIB_CPP
  /// @{
  /// @brief <b>Addon basic library functions</b>
  ///
  /// Permits the use of the required functions of the add-on to Kodi.
  ///
  /// These are functions on them no other initialization is need.
  ///
  /// It has the header \ref AddonLib.hpp "#include <kodi/api2/AddonLib.hpp>" be included
  /// to enjoy it.
  ///

  //============================================================================
  /// \ingroup KodiAddon_CPP_Main
  /// @brief Last error code generated from lib
  ///
  /// Becomes only be set if on  function on  error the  return of them  is  not
  /// possible, e.g. `KodiAPI::Init`.
  ///
  /// Human readable string can be retrived with `KodiAPI_ErrorCodeToString`.
  ///
  extern uint32_t KODI_API_lasterror;
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup KodiAddon_CPP_Main
  /// @brief Get string for error code
  ///
  /// To  translate a from several functions used error code in a human readable
  /// format
  ///
  /// @param[in] code         Error value to translate to string
  /// @return                 String about error
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// ...
  /// int retValue = KodiAPI::Init(argc, argv, &props)
  /// if (retValue != API_SUCCESS)
  /// {
  ///   fprintf(stderr, "Binary AddOn: %s\n", KodiAPI_ErrorCodeToString(retValue));
  ///   return 1;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  const char* KodiAPI_ErrorCodeToString(uint32_t code);
  //----------------------------------------------------------------------------

  //============================================================================
  /// \ingroup KodiAddon_CPP_Main
  /// @brief Add-on initialization function
  ///
  /// @warning Function only be needed on binary executable add-ons for Kodi!
  ///
  /// To be used for initialization of main interface  from Add-on to Kodi. This
  /// is  the  master  to be used  on  the  executables  <b><c>main(...)</c></b>
  /// function.
  ///
  /// @warning The  used   handle  is  only  be  used on his thread,  if another
  /// thread    is   created,   should    be   another  handle   returned   from
  /// <b><c>KodiAPI::InitThread(...)</c></b> to be used!\n\n
  /// Further  is  this  call   only   be   allowed   to   call   one   time  on
  /// <b><c>main(...)</c></b>.\n\n
  /// Also  should  never  forget  to call  <b><c>KodiAPI::Finalize(...)</c></b>
  /// before exit  of  addon  to   prevent  problems   about  still  organizated
  /// communication interface between Kodi and Add-on
  ///
  /// @param[in] argc         Amount of command line values present
  /// @param[in] argv[]       String list with given command line values
  /// @param[in] props        From add-on developer requested  properties on his
  ///                         add-on to Kodi.
  /// @param[in] hostname     The  host  name   address   to  use,  default   is
  ///                         127.0.0.1
  /// @return                 Add-on error codes from `KODI_API_Errortype` or if
  ///                         OK with `API_SUCCESS`
  ///
  /// @note If pointer is  returned  NULL is the  related last  error code  with
  /// `uint32_t KODI_API_lasterror` available.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// int main(int argc, char *argv[])
  /// {
  ///   addon_properties props;
  ///   props.addon_name      = "My Demo binary add-on";
  ///   props.is_independent  = false;
  ///
  ///   int retValue = KodiAPI::Init(argc, argv, &props)
  ///   if (retValue != API_SUCCESS)
  ///   {
  ///     fprintf(stderr, "Binary AddOn: %s\n", KodiAPI::ErrorCodeToString(retValue));
  ///     return 1;
  ///   }
  ///   ...
  ///   return 0;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  int Init(
    int               argc,
    char*             argv[],
    addon_properties* props,
    const std::string &hostname = "127.0.0.1");
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup KodiAddon_CPP_Main
  /// @brief Add-on's thread to Kodi's API creation
  ///
  /// @warning Function only be needed on binary executable add-ons for Kodi!
  ///
  /// This function creates the interface to Kodi for a new thread created  from
  /// add-on.If on new thread no call to Kodi's API is used can be this ignored.
  ///
  /// @warning A call of  <b><c>KodiAPI::Finalize(...)</c></b> on thread end  is
  /// needed if this function is used!\n\n
  /// Also  can  this  function only  be  used on  Threads new  process function
  /// `void* CThread::Process(void)` to have work on right thread id!
  ///
  ///
  /// @return                 Add-on error codes from `KODI_API_Errortype` or if
  ///                         OK with `API_SUCCESS`
  ///
  /// @note If pointer is returned  NULL is  the related  last error  code  with
  /// `uint32_t KODI_API_lasterror` available.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// void* CThreadTest::Process(void)
  /// {
  ///   KodiAPI::InitThread();
  ///
  ///   int cnt = 8;
  ///   while (!IsStopped())
  ///   {
  ///     sleep(1);
  ///     cnt--;
  ///     KodiAPI::Log(LOG_ERROR, "Countdown ---> %i", cnt);
  ///
  ///     if (cnt == 0)
  ///       break;
  ///   }
  ///
  ///   KodiAPI::Finalize();
  ///   return nullptr;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  ///
  int InitThread();
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup KodiAddon_CPP_Main
  /// @brief Resolve all callback methods for shared library type add-on
  ///
  /// @warning This function is only be used on shared library add-on for Kodi!
  ///
  /// This function link the all functions from library to kodi and without
  /// this call are them note usable.
  ///
  /// @note The required handle becomes only be send from ADDON_Create call.
  ///
  /// @param[in] handle       Pointer to the shared lib add-on
  /// @return                 Add-on error codes from `KODI_API_Errortype` or if
  ///                         OK with `API_SUCCESS`
  ///
  /// @note If pointer is returned  NULL is  the related  last error  code  with
  /// `uint32_t KODI_API_lasterror` available.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// ...
  /// ADDON_STATUS ADDON_Create(void* hdl, void* props)
  /// {
  ///   ...
  ///   // Enable access to all add-on to Kodi functions
  ///   int retValue = KodiAPI::InitLibAddon(hdl);
  ///   if (retValue != API_SUCCESS)
  ///   {
  ///     fprintf(stderr, "Binary AddOn: %s\n", KodiAPI::ErrorCodeToString(retValue));
  ///     return ADDON_STATUS_PERMANENT_FAILURE;
  ///   }
  ///
  ///   ...
  /// }
  /// ~~~~~~~~~~~~~
  ///
  ///
  int InitLibAddon(void* hdl);
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup KodiAddon_CPP_Main
  /// @brief To close Add-on to Kodi control interface
  ///
  /// This function is required after end of processes and on exit of them, e.g.
  /// Add-on itself exits or a thread with handling becomes closed.
  ///
  /// @return                 Add-on error codes from `KODI_API_Errortype` or if
  ///                         OK with `API_SUCCESS`
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example 1 (Executable add-on):**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// int main(int argc, char *argv[])
  /// {
  ///   addon_properties props;
  ///   ...
  ///
  ///   KodiAPI::Init(argc, argv, &props);
  ///   ...
  ///
  ///   KodiAPI::Finalize();
  ///   return 0;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example 2 (Library add-on):**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// ...
  ///
  /// void ADDON_Destroy()
  /// {
  ///   KodiAPI::Finalize();
  ///   m_CurStatus = ADDON_STATUS_UNKNOWN;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example 3: (Add-on Thread)**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// void* CThreadTest::Process(void)
  /// {
  ///   KodiAPI_InitThread();
  ///
  ///   while (!IsStopped())
  ///   {
  ///     ...
  ///   }
  ///
  ///   KodiAPI::Finalize();
  ///   return nullptr;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  int Finalize();
  //----------------------------------------------------------------------------


  //============================================================================
  /// \ingroup KodiAddon_CPP_Main
  /// @brief Add a message to [KODI's log](http://kodi.wiki/view/Log_file/Advanced#Log_levels).
  ///
  /// @param[in] loglevel     The log level of the message
  ///  |  enum code:  | Description:          |
  ///  |-------------:|-----------------------|
  ///  |  LOG_DEBUG   | In depth information about the status of Kodi. This information can pretty much only be deciphered by a developer or long time Kodi power user.
  ///  |  LOG_INFO    | Something has happened. It's not a problem, we just thought you might want to know. Fairly excessive output that most people won't care about.
  ///  |  LOG_NOTICE  | Similar to INFO but the average Joe might want to know about these events. This level and above are logged by default.
  ///  |  LOG_WARNING | Something potentially bad has happened. If Kodi did something you didn't expect, this is probably why. Watch for errors to follow.
  ///  |  LOG_ERROR   | This event is bad. Something has failed. You likely noticed problems with the application be it skin artifacts, failure of playback a crash, etc.
  ///  |  LOG_FATAL   | We're screwed. Kodi is about to crash.
  /// @param[in] format       The format of the message to pass to KODI.
  /// @param[in] ...          Added string values
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/AddonLib.hpp>
  ///
  /// int main(int argc, char *argv[])
  /// {
  ///   addon_properties props;
  ///   props.addon_name      = "My Demo binary add-on";
  ///   props.is_independent  = false;
  ///   ...
  ///
  ///   if (KodiAPI::Init(argc, argv, &props) != API_SUCCESS)
  ///     exit(1);
  ///   ...
  ///   KodiAPI::Log(LOG_FATAL, "Oh my goddess, I'm so fatal ;)");
  ///   ...
  /// }
  /// ~~~~~~~~~~~~~
  ///
  void Log(
    const addon_log   loglevel,
    const char*       format,
    ... );
  //----------------------------------------------------------------------------
  ///@}

}; /* extern "C" */
}; /* namespace KodiAPI */
}; /* namespace V2 */
