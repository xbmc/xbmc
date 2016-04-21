/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonString.h"
#include "Tuple.h"
//#include "Monitor.h"

#include "utils/LangCodeExpander.h"
#include "swighelper.h"
#include <vector>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace XBMCAddon
{
  namespace xbmc
  {
#ifndef SWIG
    // This is a bit of a hack to get around a SWIG problem
    extern const int lLOGDEBUG;
#endif
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

    //
    /// \defgroup python_xbmc Library - xbmc
    /// @{
    /// @brief **General functions on Kodi.**
    ///
    /// Offers classes and functions that provide information about the media
    /// currently playing and that allow manipulation of the media player (such
    /// as starting a new song). You can also find system information using the
    /// functions available in this library.
    //

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.log(msg[, level]) }
    ///-----------------------------------------------------------------------
    /// Write a string to Kodi's log file and the debug window.
    ///
    /// @param msg                 string - text to output.
    /// @param level               [opt] integer - log level to ouput at.
    ///                            <em>(default=LOGDEBUG)</em>
    ///  |  Value:         | Description:                                      |
    ///  |----------------:|---------------------------------------------------|
    ///  | xbmc.LOGDEBUG   | In depth information about the status of Kodi. This information can pretty much only be deciphered by a developer or long time Kodi power user.
    ///  | xbmc.LOGINFO    | Something has happened. It's not a problem, we just thought you might want to know. Fairly excessive output that most people won't care about.
    ///  | xbmc.LOGNOTICE  | Similar to INFO but the average Joe might want to know about these events. This level and above are logged by default.
    ///  | xbmc.LOGWARNING | Something potentially bad has happened. If Kodi did something you didn't expect, this is probably why. Watch for errors to follow.
    ///  | xbmc.LOGERROR   | This event is bad. Something has failed. You likely noticed problems with the application be it skin artifacts, failure of playback a crash, etc.
    ///  | xbmc.LOGFATAL   | We're screwed. Kodi is about to crash.
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments. Once you use a keyword, all following
    ///       arguments require the keyword.
    ///
    /// Text is written to the log for the following conditions.
    ///           - loglevel == -1 (NONE, nothing at all is logged)
    ///           - loglevel == 0 (NORMAL, shows LOGNOTICE, LOGERROR, LOGSEVERE
    ///             and LOGFATAL)
    ///           - loglevel == 1 (DEBUG, shows all)
    ///           See pydocs for valid values for level.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.log(msg='This is a test string.', level=xbmc.LOGDEBUG);
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    log(...);
#else
    void log(const char* msg, int level = lLOGDEBUG);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.Shutdown() }
    ///-----------------------------------------------------------------------
    /// Shutdown the htpc.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.shutdown()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    shutdown();
#else
    void shutdown();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.restart() }
    ///-----------------------------------------------------------------------
    /// Restart the htpc.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.restart()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    restart();
#else
    void restart();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.executescript(script) }
    ///-----------------------------------------------------------------------
    /// Execute a python script.
    ///
    /// @param script                  string - script filename to execute.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.executescript('special://home/scripts/update.py')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    executescript(...);
#else
    void executescript(const char* script);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.executebuiltin(function) }
    ///-----------------------------------------------------------------------
    /// Execute a built in Kodi function.
    ///
    /// @param function                string - builtin function to execute.
    ///
    ///
    /// List of functions - http://kodi.wiki/view/List_of_Built_In_Functions
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.executebuiltin('RunXBE(c:\\avalaunch.xbe)')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    executebuiltin(...);
#else
    void executebuiltin(const char* function, bool wait = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.executeJSONRPC(jsonrpccommand) }
    ///-----------------------------------------------------------------------
    /// Execute an JSONRPC command.
    ///
    /// @param jsonrpccommand       string - jsonrpc command to execute.
    /// @return                     jsonrpc return string
    ///
    ///
    /// List of commands -
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// response = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "method": "JSONRPC.Introspect", "id": 1 }')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    executeJSONRPC(...);
#else
    String executeJSONRPC(const char* jsonrpccommand);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.sleep(time) }
    ///-----------------------------------------------------------------------
    /// Sleeps for 'time' msec.
    ///
    /// @param time                 integer - number of msec to sleep.
    ///
    /// @throws PyExc_TypeError     If time is not an integer.
    ///
    /// @note This is useful if you have for example a Player class that is
    ///       waiting for onPlayBackEnded() calls.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.sleep(2000) # sleeps for 2 seconds
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    sleep(...);
#else
    void sleep(long timemillis);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getLocalizedString(id) }
    ///-----------------------------------------------------------------------
    /// Get a localized 'unicode string'.
    ///
    /// @param id                   integer - id# for string you want to
    ///                             localize.
    /// @return                     Localized 'unicode string'
    ///
    /// @note See strings.xml in `\language\{yourlanguage}\` for which id
    ///        you need for a string.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// locstr = xbmc.getLocalizedString(6)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getLocalizedString(...);
#else
    String getLocalizedString(int id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getSkinDir() }
    ///-----------------------------------------------------------------------
    /// Get the active skin directory.
    ///
    /// @return                         The active skin directory as a string
    ///
    ///
    /// @note This is not the full path like 'special://home/addons/MediaCenter',
    /// but only 'MediaCenter'.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// skindir = xbmc.getSkinDir()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getSkinDir();
#else
    String getSkinDir();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getLanguage([format], [region]) }
    ///-----------------------------------------------------------------------
    /// Get the active language.
    ///
    /// @param format               [opt] format of the returned language
    ///                             string
    /// | Value             | Description
    /// |------------------:|:-------------------------------------------------|
    /// | xbmc.ISO_639_1    | Two letter code as defined in ISO 639-1
    /// | xbmc.ISO_639_2    | Three letter code as defined in ISO 639-2/T or ISO 639-2/B
    /// | xbmc.ENGLISH_NAME | Full language name in English (default)
    /// @param region               [opt] append the region delimited by "-"
    ///                             of the language (setting) to the
    ///                             returned language string
    /// @return                     The active language as a string
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// language = xbmc.getLanguage(xbmc.ENGLISH_NAME)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getLanguage(...);
#else
    String getLanguage(int format = CLangCodeExpander::ENGLISH_NAME, bool region = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getIPAddress() }
    ///-----------------------------------------------------------------------
    /// Get the current ip address.
    ///
    /// @return The current ip address as a string
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// ip = xbmc.getIPAddress()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getIPAddress();
#else
    String getIPAddress();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getDVDState() }
    ///-----------------------------------------------------------------------
    /// Returns the dvd state as an integer.
    ///
    /// @return Values for state are:
    /// | Value | Name                           |
    /// |------:|:-------------------------------|
    /// |   1   | xbmc.DRIVE_NOT_READY
    /// |   16  | xbmc.TRAY_OPEN
    /// |   64  | xbmc.TRAY_CLOSED_NO_MEDIA
    /// |   96  | xbmc.TRAY_CLOSED_MEDIA_PRESENT
    ///
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// dvdstate = xbmc.getDVDState()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getDVDState();
#else
    long getDVDState();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getFreeMem() }
    ///-----------------------------------------------------------------------
    /// Get amount of free memory in MB.
    ///
    /// @return The amount of free memory in MB as an integer
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// freemem = xbmc.getFreeMem()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getFreeMem();
#else
    long getFreeMem();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getInfoLabel(infotag) }
    ///-----------------------------------------------------------------------
    /// Get a info label
    ///
    /// @param infotag               string - infoTag for value you want
    ///                              returned.
    /// @return                      InfoLabel as a string
    ///
    /// List of InfoTags - http://kodi.wiki/view/InfoLabels
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// label = xbmc.getInfoLabel('Weather.Conditions')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getInfoLabel(...);
#else
    String getInfoLabel(const char* cLine);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getInfoImage(infotag) }
    ///-----------------------------------------------------------------------
    /// Get filename including path to the InfoImage's thumbnail.
    ///
    /// @param infotag               string - infotag for value you want
    ///                              returned
    /// @return                      Filename including path to the
    ///                              InfoImage's thumbnail as a string
    ///
    ///
    /// List of InfoTags - http://kodi.wiki/view/InfoLabels
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// filename = xbmc.getInfoImage('Weather.Conditions')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getInfoImage(...);
#else
    String getInfoImage(const char * infotag);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.playSFX(filename,[useCached]) }
    ///-----------------------------------------------------------------------
    /// Plays a wav file by filename
    ///
    /// @param filename              string - filename of the wav file to
    ///                              play
    /// @param useCached             [opt] bool - False = Dump any
    ///                              previously cached wav associated with
    ///                              filename
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.playSFX('special://xbmc/scripts/dingdong.wav')
    /// xbmc.playSFX('special://xbmc/scripts/dingdong.wav',False)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    playSFX(...);
#else
    void playSFX(const char* filename, bool useCached = true);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.stopSFX() }
    ///-----------------------------------------------------------------------
    /// Stops wav file
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.stopSFX()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    stopSFX();
#else
    void stopSFX();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.enableNavSounds(yesNo) }
    ///-----------------------------------------------------------------------
    /// Enables/Disables nav sounds
    ///
    /// @param yesNo                 integer - enable (True) or disable
    ///                              (False) nav sounds
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.enableNavSounds(True)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    enableNavSounds(...);
#else
    void enableNavSounds(bool yesNo);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getCondVisibility(condition) }
    ///-----------------------------------------------------------------------
    /// Get visibility conditions
    ///
    /// @param condition             string - condition to check
    /// @return                      True (1) or False (0) as a bool
    ///
    /// List of Conditions - http://kodi.wiki/view/List_of_Boolean_Conditions
    ///
    /// @note You can combine two (or more) of the above settings by using <b>"+"</b> as an AND operator,
    /// <b>"|"</b> as an OR operator, <b>"!"</b> as a NOT operator, and <b>"["</b> and <b>"]"</b> to bracket expressions.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// visible = xbmc.getCondVisibility('[Control.IsVisible(41) + !Control.IsVisible(12)]')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getCondVisibility(...);
#else
    bool getCondVisibility(const char *condition);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getGlobalIdleTime() }
    ///-----------------------------------------------------------------------
    /// Get the elapsed idle time in seconds.
    ///
    /// @return Elapsed idle time in seconds as an integer
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// t = xbmc.getGlobalIdleTime()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getGlobalIdleTime();
#else
    int getGlobalIdleTime();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getCacheThumbName(path) }
    ///-----------------------------------------------------------------------
    /// Get thumb cache filename.
    ///
    /// @param path                  string or unicode - path to file
    /// @return                      Thumb cache filename
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// thumb = xbmc.getCacheThumbName('f:\\videos\\movie.avi')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getCacheThumbName(...);
#else
    String getCacheThumbName(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.makeLegalFilename(filename[, fatX]) }
    ///-----------------------------------------------------------------------
    /// Returns a legal filename or path as a string.
    ///
    /// @param filename              string or unicode - filename/path to
    ///                              make legal
    /// @paran fatX                  [opt] bool - True=Xbox file system(Default)
    /// @return                      Legal filename or path as a string
    ///
    ///
    /// @note If fatX is true you should pass a full path. If fatX is false only pass
    ///       the basename of the path.\n\n
    ///       You can use the above as keywords for arguments and skip certain optional arguments.
    ///       Once you use a keyword, all following arguments require the keyword.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// filename = xbmc.makeLegalFilename('F:\\Trailers\\Ice Age: The Meltdown.avi')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    makeLegalFilename(...);
#else
    String makeLegalFilename(const String& filename,bool fatX = true);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.translatePath(path)  }
    ///-----------------------------------------------------------------------
    /// Returns the translated path.
    ///
    /// @param path                  string or unicode - Path to format
    /// @return                      Translated path
    ///
    /// @note Only useful if you are coding for both Linux and Windows.
    ///        e.g. Converts 'special://masterprofile/script_data' -> '/home/user/XBMC/UserData/script_data'
    ///        on Linux.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// fpath = xbmc.translatePath('special://masterprofile/script_data')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    translatePath(...);
#else
    String translatePath(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getCleanMovieTitle(path[, usefoldername]) }
    ///-----------------------------------------------------------------------
    /// Get clean movie title and year string if available.
    ///
    /// @param path                  string or unicode - String to clean
    /// @param usefoldername         [opt] bool - use folder names (defaults
    ///                              to false)
    /// @return                      Clean movie title and year string if
    ///                              available.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// title, year = xbmc.getCleanMovieTitle('/path/to/moviefolder/test.avi', True)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getCleanMovieTitle(...);
#else
    Tuple<String,String> getCleanMovieTitle(const String& path, bool usefoldername = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.validatePath(path) }
    ///-----------------------------------------------------------------------
    /// Returns the validated path.
    ///
    /// @param path                  string or unicode - Path to format
    /// @return                      Validated path
    ///
    /// @note Only useful if you are coding for both Linux and Windows for fixing slash problems.
    ///       e.g. Corrects 'Z://something' -> 'Z:\something'
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// fpath = xbmc.validatePath(somepath)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    validatePath(...);
#else
    String validatePath(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getRegion(id) }
    ///-----------------------------------------------------------------------
    /// Returns your regions setting as a string for the specified id.
    ///
    /// @param id                    string - id of setting to return
    /// @return                      Region setting
    ///
    /// @note choices are (dateshort, datelong, time, meridiem, tempunit, speedunit)
    ///        You can use the above as keywords for arguments.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// date_long_format = xbmc.getRegion('datelong')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getRegion(...);
#else
    String getRegion(const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getSupportedMedia(media) }
    ///-----------------------------------------------------------------------
    /// Get the supported file types for the specific media.
    ///
    /// @param media                 string - media type
    /// @return                      Supported file types for the specific
    ///                              media as a string
    ///
    ///
    /// @note Media type can be (video, music, picture).
    ///       The return value is a pipe separated string of filetypes
    ///       (eg. '.mov|.avi').\n
    ///       You can use the above as keywords for arguments.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// mTypes = xbmc.getSupportedMedia('video')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    getSupportedMedia(...);
#else
    String getSupportedMedia(const char* mediaType);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.skinHasImage(image) }
    ///-----------------------------------------------------------------------
    /// Check skin for presence of Image.
    ///
    /// @param image                 string - image filename
    /// @return                      True if the image file exists in the skin
    ///
    ///
    /// @note If the media resides in a subfolder include it. (eg. home-myfiles\\home-myfiles2.png).
    ///       You can use the above as keywords for arguments.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// exists = xbmc.skinHasImage('ButtonFocusedTexture.png')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    skinHasImage(...);
#else
    bool skinHasImage(const char* image);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.startServer(typ, bStart, bWait) }
    ///-------------------------------------------------------------------------
    /// Start or stop a server.
    ///
    /// @param typ                  integer - use SERVER_* constants
    /// - Used format of the returned language string
    /// | Value                     | Description                                                |
    /// |--------------------------:|------------------------------------------------------------|
    /// | xbmc.SERVER_WEBSERVER     | [To control Kodi's builtin webserver](http://kodi.wiki/view/Webserver)
    /// | xbmc.SERVER_AIRPLAYSERVER | [AirPlay is a proprietary protocol stack/suite developed by Apple Inc.](http://kodi.wiki/view/AirPlay)
    /// | xbmc.SERVER_JSONRPCSERVER | [Control JSON-RPC HTTP/TCP socket-based interface](http://kodi.wiki/view/JSON-RPC_API)
    /// | xbmc.SERVER_UPNPRENDERER  | [UPnP client (aka UPnP renderer)](http://kodi.wiki/view/UPnP/Client)
    /// | xbmc.SERVER_UPNPSERVER    | [Control built-in UPnP A/V media server (UPnP-server)](http://kodi.wiki/view/UPnP/Server)
    /// | xbmc.SERVER_EVENTSERVER   | [Set eventServer part that accepts remote device input on all platforms](http://kodi.wiki/view/EventServer)
    /// | xbmc.SERVER_ZEROCONF      | [Control Kodi's Avahi Zeroconf](http://kodi.wiki/view/Zeroconf)
    /// @param bStart               bool - start (True) or stop (False) a server
    /// @param bWait                [opt] bool - wait on stop before returning (not supported by all servers)
    /// @return                     bool - True or False
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.startServer(xbmc.SERVER_AIRPLAYSERVER, False)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    startServer(...);
#else
    bool startServer(int iTyp, bool bStart, bool bWait = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.audioSuspend() }
    ///-----------------------------------------------------------------------
    /// Suspend Audio engine.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.audioSuspend()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    audioSuspend();
#else
    void audioSuspend();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.audioResume() }
    ///-----------------------------------------------------------------------
    /// Resume Audio engine.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.audioResume()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    audioResume();
#else
    void audioResume();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.getUserAgent() }
    ///-----------------------------------------------------------------------
    /// @brief Returns Kodi's HTTP UserAgent string
    ///
    /// @return                           HTTP user agent
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmc.getUserAgent()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    /// example output:
    ///   Kodi/17.0-ALPHA1 (X11; Linux x86_64) Ubuntu/15.10 App_Bitness/64 Version/17.0-ALPHA1-Git:2015-12-23-5770d28
    ///
    getUserAgent();
#else
    String getUserAgent();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmc
    /// @brief \python_func{ xbmc.convertLanguage(language, format) }
    ///-----------------------------------------------------------------------
    /// @bruef Returns the given language converted to the given format as a
    /// string.
    ///
    /// @param language              string either as name in English, two
    ///                              letter code (ISO 639-1), or three
    ///                              letter code (ISO 639-2/T(B)
    /// @param format                format of the returned language string
    /// | Value             | Description
    /// |------------------:|:-------------------------------------------------|
    /// | xbmc.ISO_639_1    | Two letter code as defined in ISO 639-1
    /// | xbmc.ISO_639_2    | Three letter code as defined in ISO 639-2/T or ISO 639-2/B
    /// | xbmc.ENGLISH_NAME | Full language name in English (default)
    /// @return                          Converted Language string
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// language = xbmc.convertLanguage(English, xbmc.ISO_639_2)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    convertLanguage(...);
#else
    String convertLanguage(const char* language, int format);
#endif
    //@}
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_WEBSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_AIRPLAYSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_UPNPSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_UPNPRENDERER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_EVENTSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_JSONRPCSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_ZEROCONF);

    SWIG_CONSTANT_FROM_GETTER(int,PLAYLIST_MUSIC);
    SWIG_CONSTANT_FROM_GETTER(int,PLAYLIST_VIDEO);
    SWIG_CONSTANT_FROM_GETTER(int,TRAY_OPEN);
    SWIG_CONSTANT_FROM_GETTER(int,DRIVE_NOT_READY);
    SWIG_CONSTANT_FROM_GETTER(int,TRAY_CLOSED_NO_MEDIA);
    SWIG_CONSTANT_FROM_GETTER(int,TRAY_CLOSED_MEDIA_PRESENT);
    SWIG_CONSTANT_FROM_GETTER(int,LOGDEBUG);
    SWIG_CONSTANT_FROM_GETTER(int,LOGINFO);
    SWIG_CONSTANT_FROM_GETTER(int,LOGNOTICE);
    SWIG_CONSTANT_FROM_GETTER(int,LOGWARNING);
    SWIG_CONSTANT_FROM_GETTER(int,LOGERROR);
    SWIG_CONSTANT_FROM_GETTER(int,LOGSEVERE);
    SWIG_CONSTANT_FROM_GETTER(int,LOGFATAL);
    SWIG_CONSTANT_FROM_GETTER(int,LOGNONE);

    SWIG_CONSTANT_FROM_GETTER(int,ISO_639_1);
    SWIG_CONSTANT_FROM_GETTER(int,ISO_639_2);
    SWIG_CONSTANT_FROM_GETTER(int,ENGLISH_NAME);
#if 0
    void registerMonitor(Monitor* monitor);
    void unregisterMonitor(Monitor* monitor);
#endif
  }
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
