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

    ///
    /// \ingroup python_xbmc
    /// Write a string to Kodi's log file and the debug window.
    ///
    /// @param[in] msg                 string - text to output.
    /// @param[in] level               [opt] integer - log level to ouput at.
    ///                                <em>(default=LOGDEBUG)</em>
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
    void log(const char* msg, int level = lLOGDEBUG);

    ///
    /// \ingroup python_xbmc
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
    void shutdown();

    ///
    /// \ingroup python_xbmc
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
    void restart();

    ///
    /// \ingroup python_xbmc
    /// Execute a python script.
    ///
    /// @param[in] script                  string - script filename to execute.
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
    void executescript(const char* script);

    ///
    /// \ingroup python_xbmc
    /// Execute a built in Kodi function.
    ///
    /// @param[in] function                string - builtin function to execute.
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
    void executebuiltin(const char* function, bool wait = false);

    ///
    /// \ingroup python_xbmc
    /// Execute an JSONRPC command.
    ///
    /// @param[in] jsonrpccommand       string - jsonrpc command to execute.
    /// @return                         jsonrpc return string
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
    String executeJSONRPC(const char* jsonrpccommand);

    ///
    /// \ingroup python_xbmc
    /// Sleeps for 'time' msec.
    ///
    /// @param[in] time                 integer - number of msec to sleep.
    ///
    /// @throws PyExc_TypeError         If time is not an integer.
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
    void sleep(long timemillis);

    ///
    /// \ingroup python_xbmc
    /// Get a localized 'unicode string'.
    ///
    /// @param[in] id                   integer - id# for string you want to
    ///                                 localize.
    /// @return                         Localized 'unicode string'
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
    String getLocalizedString(int id);

    ///
    /// \ingroup python_xbmc
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
    String getSkinDir();

    ///
    /// \ingroup python_xbmc
    /// Get the active language.
    ///
    /// @param[in] format               [opt] format of the returned language
    ///                                 string
    /// | Value             | Description
    /// |------------------:|:-------------------------------------------------|
    /// | xbmc.ISO_639_1    | Two letter code as defined in ISO 639-1
    /// | xbmc.ISO_639_2    | Three letter code as defined in ISO 639-2/T or ISO 639-2/B
    /// | xbmc.ENGLISH_NAME | Full language name in English (default)
    /// @param[in] region               [opt] append the region delimited by "-"
    ///                                 of the language (setting) to the
    ///                                 returned language string
    /// @return                         The active language as a string
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
    String getLanguage(int format = CLangCodeExpander::ENGLISH_NAME, bool region = false);

    ///
    /// \ingroup python_xbmc
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
    String getIPAddress();

    ///
    /// \ingroup python_xbmc
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
    long getDVDState();

    ///
    /// \ingroup python_xbmc
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
    long getFreeMem();

    ///
    /// \ingroup python_xbmc
    /// Get a info label
    ///
    /// @param[in] infotag               string - infoTag for value you want
    ///                                  returned.
    /// @return                          InfoLabel as a string
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
    String getInfoLabel(const char* infotag);

    ///
    /// \ingroup python_xbmc
    /// Get filename including path to the InfoImage's thumbnail.
    ///
    /// @param[in] infotag               string - infotag for value you want
    ///                                  returned
    /// @return                          Filename including path to the
    ///                                  InfoImage's thumbnail as a string
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
    String getInfoImage(const char* infotag);

    ///
    /// \ingroup python_xbmc
    /// Plays a wav file by filename
    ///
    /// @param[in] filename              string - filename of the wav file to
    ///                                  play
    /// @param[in] useCached             [opt] bool - False = Dump any
    ///                                  previously cached wav associated with
    ///                                  filename
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
    void playSFX(const char* filename, bool useCached = true);

    ///
    /// \ingroup python_xbmc
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
    void stopSFX();

    ///
    /// \ingroup python_xbmc
    /// Enables/Disables nav sounds
    ///
    /// @param[in] yesNo                 integer - enable (True) or disable
    ///                                  (False) nav sounds
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
    void enableNavSounds(bool yesNo);

    ///
    /// \ingroup python_xbmc
    /// Get visibility conditions
    ///
    /// @param[in] condition             string - condition to check
    /// @return                          True (1) or False (0) as a bool
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
    bool getCondVisibility(const char* condition);

    ///
    /// \ingroup python_xbmc
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
    int getGlobalIdleTime();

    ///
    /// \ingroup python_xbmc
    /// Get thumb cache filename.
    ///
    /// @param[in] path                  string or unicode - path to file
    /// @return                          Thumb cache filename
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
    String getCacheThumbName(const String& path);

    ///
    /// \ingroup python_xbmc
    /// Returns a legal filename or path as a string.
    ///
    /// @param[in] filename              string or unicode - filename/path to
    ///                                  make legal
    /// @paran fatX                      [opt] bool - True=Xbox file system(Default)
    /// @return                          Legal filename or path as a string
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
    String makeLegalFilename(const String& filename,bool fatX = true);

    ///
    /// \ingroup python_xbmc
    /// Returns the translated path.
    ///
    /// @param[in] path                  string or unicode - Path to format
    /// @return                          Translated path
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
    String translatePath(const String& path);

    ///
    /// \ingroup python_xbmc
    /// Get clean movie title and year string if available.
    ///
    /// @param[in] path                  string or unicode - String to clean
    /// @param[in] usefoldername         [opt] bool - use folder names (defaults
    ///                                  to false)
    /// @return                          Clean movie title and year string if
    ///                                  available.
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
    Tuple<String,String> getCleanMovieTitle(const String& path, bool usefoldername = false);

    ///
    /// \ingroup python_xbmc
    /// Returns the validated path.
    ///
    /// @param[in] path                  string or unicode - Path to format
    /// @return                          Validated path
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
    String validatePath(const String& path);

    ///
    /// \ingroup python_xbmc
    /// Returns your regions setting as a string for the specified id.
    ///
    /// @param[in] id                    string - id of setting to return
    /// @return                          Region setting
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
    String getRegion(const char* id);

    ///
    /// \ingroup python_xbmc
    /// Get the supported file types for the specific media.
    ///
    /// @param[in] media                 string - media type
    /// @return                          Supported file types for the specific
    ///                                  media as a string
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
    String getSupportedMedia(const char* mediaType);

    ///
    /// \ingroup python_xbmc
    /// Check skin for presence of Image.
    ///
    /// @param[in] image                 string - image filename
    /// @return                          True if the image file exists in the skin
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
    bool skinHasImage(const char* image);

    ///
    /// \ingroup python_xbmc
    /// Start or stop a server.
    ///
    /// @param[in] typ                  integer - use SERVER_* constants
    /// @param[in] bStart               bool - start (True) or stop (False) a server
    /// @param[in] bWait                [opt] bool - wait on stop before returning (not supported by all servers)
    /// @return                          bool - True or False
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
    bool startServer(int iTyp, bool bStart, bool bWait = false);

    ///
    /// \ingroup python_xbmc
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
    void audioSuspend();

    ///
    /// \ingroup python_xbmc
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
    void audioResume();

    ///
    /// \ingroup python_xbmc
    /// @brief Returns Kodi's HTTP UserAgent string
    ///
    /// @return                           HTTP user agent
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
    String getUserAgent();

    ///
    /// \ingroup python_xbmc
    /// @bruef Returns the given language converted to the given format as a
    /// string.
    ///
    /// @param[in] language              string either as name in English, two
    ///                                  letter code (ISO 639-1), or three
    ///                                  letter code (ISO 639-2/T(B)
    /// @param[in] format                format of the returned language string
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
    String convertLanguage(const char* language, int format);
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
