/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AddonString.h"
#include "Tuple.h"
//#include "Monitor.h"

#include "utils/log.h"
#include "utils/StdString.h"

#include "swighelper.h"
#include <vector>

namespace XBMCAddon
{
  namespace xbmc
  {
#ifndef SWIG
    // This is a bit of a hack to get around a SWIG problem
    extern const int lLOGNOTICE;
#endif

    /**
     * log(msg[, level]) -- Write a string to XBMC's log file and the debug window.
     *     msg            : string - text to output.
     *     level          : [opt] integer - log level to ouput at. (default=LOGNOTICE)
     *     
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     * 
     * Text is written to the log for the following conditions.
     *           XBMC loglevel == -1 (NONE, nothing at all is logged)
     *           XBMC loglevel == 0 (NORMAL, shows LOGNOTICE, LOGERROR, LOGSEVERE and LOGFATAL)\
     *           XBMC loglevel == 1 (DEBUG, shows all)
     *           See pydocs for valid values for level.
     *           
     *           example:
     *             - xbmc.output(msg='This is a test string.', level=xbmc.LOGDEBUG));
     */
    void log(const char* msg, int level = lLOGNOTICE);

    /**
     * Shutdown() -- Shutdown the xbox.
     *
     * example:
     *  - xbmc.shutdown()
     */
    void shutdown();

    /**
     * restart() -- Restart the xbox.
     * example:
     *  - xbmc.restart()
     */
    void restart();

    /**
     * executescript(script) -- Execute a python script.
     * 
     * script         : string - script filename to execute.
     * 
     * example:
     *   - xbmc.executescript('special://home/scripts/update.py')
     */
    void executescript(const char* script);

    /**
     * executebuiltin(function) -- Execute a built in XBMC function.
     * 
     * function       : string - builtin function to execute.
     * 
     * List of functions - http://wiki.xbmc.org/?title=List_of_Built_In_Functions 
     * 
     * example:
     *   - xbmc.executebuiltin('XBMC.RunXBE(c:\\\\avalaunch.xbe)')
     */
    void executebuiltin(const char* function, bool wait = false);

    /**
     * executehttpapi(httpcommand) -- Not implemented anymore.
     */
    String executehttpapi(const char* httpcommand);

    /**
     * executeJSONRPC(jsonrpccommand) -- Execute an JSONRPC command.
     * 
     * jsonrpccommand    : string - jsonrpc command to execute.
     * 
     * List of commands - 
     * 
     * example:
     *   - response = xbmc.executeJSONRPC('{ \"jsonrpc\": \"2.0\", \"method\": \"JSONRPC.Introspect\", \"id\": 1 }')
     */
    String executeJSONRPC(const char* jsonrpccommand);

    /**
     * sleep(time) -- Sleeps for 'time' msec.
     * 
     * time           : integer - number of msec to sleep.
     * 
     * *Note, This is useful if you have for example a Player class that is waiting
     *        for onPlayBackEnded() calls.
     * 
     * Throws: PyExc_TypeError, if time is not an integer.
     * 
     * example:
     *   - xbmc.sleep(2000) # sleeps for 2 seconds
     */
    void sleep(long timemillis);

    /**
     * getLocalizedString(id) -- Returns a localized 'unicode string'.
     * 
     * id             : integer - id# for string you want to localize.
     * 
     * *Note, See strings.xml in \\language\\{yourlanguage}\\ for which id
     *        you need for a string.
     * 
     * example:
     *   - locstr = xbmc.getLocalizedString(6)
     */
    String getLocalizedString(int id);

    /**
     * getSkinDir() -- Returns the active skin directory as a string.
     * 
     * *Note, This is not the full path like 'special://home/addons/MediaCenter', but only 'MediaCenter'.
     * 
     * example:
     *   - skindir = xbmc.getSkinDir()
     */
    String getSkinDir();

    /**
     * getLanguage() -- Returns the active language as a string.
     * 
     * example:
     *   - language = xbmc.getLanguage()
     */
    String getLanguage();

    /**
     * getIPAddress() -- Returns the current ip address as a string.
     * 
     * example:
     *   - ip = xbmc.getIPAddress()
     */
    String getIPAddress();

    /**
     * getDVDState() -- Returns the dvd state as an integer.
     * 
     * return values are:
     *    1 : xbmc.DRIVE_NOT_READY
     *   16 : xbmc.TRAY_OPEN
     *   64 : xbmc.TRAY_CLOSED_NO_MEDIA
     *   96 : xbmc.TRAY_CLOSED_MEDIA_PRESENT
     * 
     * example:
     *   - dvdstate = xbmc.getDVDState()
     */
    long getDVDState();

    /**
     * getFreeMem() -- Returns the amount of free memory in MB as an integer.
     * 
     * example:
     *   - freemem = xbmc.getFreeMem()
     */
    long getFreeMem();

    /**
     * getInfoLabel(infotag) -- Returns an InfoLabel as a string.
     * 
     * infotag        : string - infoTag for value you want returned.
     * 
     * List of InfoTags - http://wiki.xbmc.org/?title=InfoLabels 
     * 
     * example:
     *   - label = xbmc.getInfoLabel('Weather.Conditions')
     */
    String getInfoLabel(const char* cLine);

    /**
     * getInfoImage(infotag) -- Returns a filename including path to the InfoImage's
     *                          thumbnail as a string.
     * 
     * infotag        : string - infotag for value you want returned.
     * 
     * List of InfoTags - http://wiki.xbmc.org/?title=InfoLabels 
     * 
     * example:
     *   - filename = xbmc.getInfoImage('Weather.Conditions')
     */
    String getInfoImage(const char * infotag);

    /**
     * playSFX(filename) -- Plays a wav file by filename
     * 
     * filename       : string - filename of the wav file to play.
     * 
     * example:
     *   - xbmc.playSFX('special://xbmc/scripts/dingdong.wav')
     */
    void playSFX(const char* filename);

    /**
     * enableNavSounds(yesNo) -- Enables/Disables nav sounds
     * 
     * yesNo          : integer - enable (True) or disable (False) nav sounds
     * 
     * example:
     *   - xbmc.enableNavSounds(True)
     */
    void enableNavSounds(bool yesNo);

    /**
     * getCondVisibility(condition) -- Returns True (1) or False (0) as a bool.
     * 
     * condition      : string - condition to check.
     * 
     * List of Conditions - http://wiki.xbmc.org/?title=List_of_Boolean_Conditions 
     * 
     * *Note, You can combine two (or more) of the above settings by using \"+\" as an AND operator,
     * \"|\" as an OR operator, \"!\" as a NOT operator, and \"[\" and \"]\" to bracket expressions.
     * 
     * example:
     *   - visible = xbmc.getCondVisibility('[Control.IsVisible(41) + !Control.IsVisible(12)]')
     */
    bool getCondVisibility(const char *condition);

    /**
     * getGlobalIdleTime() -- Returns the elapsed idle time in seconds as an integer.
     * 
     * example:
     *   - t = xbmc.getGlobalIdleTime()
     */
    int getGlobalIdleTime();

    /**
     * getCacheThumbName(path) -- Returns a thumb cache filename.
     * 
     * path           : string or unicode - path to file
     * 
     * example:
     *   - thumb = xbmc.getCacheThumbName('f:\\\\videos\\\\movie.avi')
     */
    String getCacheThumbName(const String& path);

    /**
     * makeLegalFilename(filename[, fatX]) -- Returns a legal filename or path as a string.
     * 
     * filename       : string or unicode - filename/path to make legal
     * fatX           : [opt] bool - True=Xbox file system(Default)
     * 
     * *Note, If fatX is true you should pass a full path. If fatX is false only pass
     *        the basename of the path.
     * 
     *        You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     * 
     * example:
     *   - filename = xbmc.makeLegalFilename('F:\\Trailers\\Ice Age: The Meltdown.avi')
     */
    String makeLegalFilename(const String& filename,bool fatX = true);

    /**
     * translatePath(path) -- Returns the translated path.
     * 
     * path           : string or unicode - Path to format
     * 
     * *Note, Only useful if you are coding for both Linux and Windows/Xbox.
     *        e.g. Converts 'special://masterprofile/script_data' -> '/home/user/XBMC/UserData/script_data'
     *        on Linux. Would return 'special://masterprofile/script_data' on the Xbox.
     * 
     * example:
     *   - fpath = xbmc.translatePath('special://masterprofile/script_data')
     */
    String translatePath(const String& path);

    /**
     * getCleanMovieTitle(path[, usefoldername]) -- Returns a clean movie title and year string if available.
     * 
     * path           : string or unicode - String to clean
     * bool           : [opt] bool - use folder names (defaults to false)
     * 
     * example:
     *   - title, year = xbmc.getCleanMovieTitle('/path/to/moviefolder/test.avi', True)
     */
    Tuple<String,String> getCleanMovieTitle(const String& path, bool usefoldername = false);

    /**
     * validatePath(path) -- Returns the validated path.
     * 
     * path           : string or unicode - Path to format
     * 
     * *Note, Only useful if you are coding for both Linux and Windows/Xbox for fixing slash problems.
     *        e.g. Corrects 'Z://something' -> 'Z:\\something'
     * 
     * example:
     *   - fpath = xbmc.validatePath(somepath)
     */
    String validatePath(const String& path);

    /**
     * getRegion(id) -- Returns your regions setting as a string for the specified id.
     * 
     * id             : string - id of setting to return
     * 
     * *Note, choices are (dateshort, datelong, time, meridiem, tempunit, speedunit)
     * 
     *        You can use the above as keywords for arguments.
     * 
     * example:
     *   - date_long_format = xbmc.getRegion('datelong')
     */
    String getRegion(const char* id);

    /**
     * getSupportedMedia(media) -- Returns the supported file types for the specific media as a string.
     * 
     * media          : string - media type
     * 
     * *Note, media type can be (video, music, picture).
     * 
     *        The return value is a pipe separated string of filetypes (eg. '.mov|.avi').
     * 
     *        You can use the above as keywords for arguments.
     * 
     * example:
     *   - mTypes = xbmc.getSupportedMedia('video')
     */
    String getSupportedMedia(const char* mediaType);

    /**
     * skinHasImage(image) -- Returns True if the image file exists in the skin.
     * 
     * image          : string - image filename
     * 
     * *Note, If the media resides in a subfolder include it. (eg. home-myfiles\\\\home-myfiles2.png)
     * 
     *        You can use the above as keywords for arguments.
     * 
     * example:
     *   - exists = xbmc.skinHasImage('ButtonFocusedTexture.png')
     */
    bool skinHasImage(const char* image);

    /**
     * startServer(typ, bStart, bWait) -- start or stop a server.
     * 
     * typ          : integer - use SERVER_* constants
     * 
     * bStart       : bool - start (True) or stop (False) a server
     * 
     * bWait        : [opt] bool - wait on stop before returning (not supported by all servers)
     * 
     * returnValue  : bool - True or False
     * example:
     *   - xbmc.startServer(xbmc.SERVER_AIRPLAYSERVER, False)
     */
    bool startServer(int iTyp, bool bStart, bool bWait = false);

    /**
     * AudioSuspend() -- Suspend Audio engine.
     * 
     * example:
     *   xbmc.AudioSuspend()
     */
    void audioSuspend();

    /**
     * AudioResume() -- Resume Audio engine.
     * 
     * example:
     *   xbmc.AudioResume()
     */  
    void audioResume();

    SWIG_CONSTANT_FROM_GETTER(int,SERVER_WEBSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_AIRPLAYSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_UPNPSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_UPNPRENDERER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_EVENTSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_JSONRPCSERVER);
    SWIG_CONSTANT_FROM_GETTER(int,SERVER_ZEROCONF);

    SWIG_CONSTANT_FROM_GETTER(int,PLAYLIST_MUSIC);
    SWIG_CONSTANT_FROM_GETTER(int,PLAYLIST_VIDEO);
    SWIG_CONSTANT_FROM_GETTER(int,PLAYER_CORE_AUTO);
    SWIG_CONSTANT_FROM_GETTER(int,PLAYER_CORE_DVDPLAYER);
    SWIG_CONSTANT_FROM_GETTER(int,PLAYER_CORE_MPLAYER);
    SWIG_CONSTANT_FROM_GETTER(int,PLAYER_CORE_PAPLAYER);
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


    SWIG_CONSTANT_FROM_GETTER(int,CAPTURE_STATE_WORKING);
    SWIG_CONSTANT_FROM_GETTER(int,CAPTURE_STATE_DONE);
    SWIG_CONSTANT_FROM_GETTER(int,CAPTURE_STATE_FAILED);

    SWIG_CONSTANT_FROM_GETTER(int,CAPTURE_FLAG_CONTINUOUS);
    SWIG_CONSTANT_FROM_GETTER(int,CAPTURE_FLAG_IMMEDIATELY);
#if 0
    void registerMonitor(Monitor* monitor);
    void unregisterMonitor(Monitor* monitor);
#endif
  }
}
