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

namespace XBMCAddon
{
  namespace xbmc
  {
#ifndef SWIG
    // This is a bit of a hack to get around a SWIG problem
    extern const int lLOGNOTICE;
#endif

    /**
     * log(msg[, level]) -- Write a string to XBMC's log file and the debug window.\n
     *     msg            : string - text to output.\n
     *     level          : [opt] integer - log level to ouput at. (default=LOGNOTICE)\n
     *     \n
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.\n
     * \n
     * Text is written to the log for the following conditions.\n
     *           XBMC loglevel == -1 (NONE, nothing at all is logged)\n
     *           XBMC loglevel == 0 (NORMAL, shows LOGNOTICE, LOGERROR, LOGSEVERE and LOGFATAL)\n
     *           XBMC loglevel == 1 (DEBUG, shows all)\n
     *           See pydocs for valid values for level.\n
     *           
     *           example:
     *             - xbmc.log(msg='This is a test string.', level=xbmc.LOGDEBUG));
     */
    void log(const char* msg, int level = lLOGNOTICE);

    /**
     * Shutdown() -- Shutdown the htpc.
     *
     * example:
     *  - xbmc.shutdown()
     */
    void shutdown();

    /**
     * restart() -- Restart the htpc.
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
     * List of functions - http://kodi.wiki/view/List_of_Built_In_Functions
     * 
     * example:
     *   - xbmc.executebuiltin('RunXBE(c:\\avalaunch.xbe)')
     */
    void executebuiltin(const char* function, bool wait = false);

    /**
     * executeJSONRPC(jsonrpccommand) -- Execute an JSONRPC command.
     * 
     * jsonrpccommand    : string - jsonrpc command to execute.
     * 
     * List of commands - 
     * 
     * example:
     *   - response = xbmc.executeJSONRPC('{ "jsonrpc": "2.0", "method": "JSONRPC.Introspect", "id": 1 }')
     */
    String executeJSONRPC(const char* jsonrpccommand);

    /**
     * sleep(time) -- Sleeps for 'time' msec.
     * 
     * time           : integer - number of msec to sleep.
     * 
     * *Note, This is useful if you have for example a Player class that is waiting\n
     *        for onPlayBackEnded() calls.\n
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
     * *Note, See strings.xml in \language\{yourlanguage}\ for which id\n
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
    * getLanguage([format], [region]) -- Returns the active language as a string.
    *
    * format: [opt] format of the returned language string
    *               - xbmc.ISO_639_1: two letter code as defined in ISO 639-1
    *               - xbmc.ISO_639_2: three letter code as defined in ISO 639-2/T or ISO 639-2/B
    *               - xbmc.ENGLISH_NAME: full language name in English (default)
    *
    * region: [opt] append the region delimited by "-" of the language (setting)
    *               to the returned language string
    *
    * example:
    *   - language = xbmc.getLanguage(xbmc.ENGLISH_NAME)
    */
    String getLanguage(int format = CLangCodeExpander::ENGLISH_NAME, bool region = false);

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
     *   -  1 : xbmc.DRIVE_NOT_READY
     *   - 16 : xbmc.TRAY_OPEN
     *   - 64 : xbmc.TRAY_CLOSED_NO_MEDIA
     *   - 96 : xbmc.TRAY_CLOSED_MEDIA_PRESENT
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
     * List of InfoTags - http://kodi.wiki/view/InfoLabels
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
     * List of InfoTags - http://kodi.wiki/view/InfoLabels
     * 
     * example:
     *   - filename = xbmc.getInfoImage('Weather.Conditions')
     */
    String getInfoImage(const char * infotag);

    /**
     * playSFX(filename,[useCached]) -- Plays a wav file by filename
     * 
     * filename       : string - filename of the wav file to play.\n
     * useCached      : [opt] bool - False = Dump any previously cached wav associated with filename
     * 
     * example:
     *   - xbmc.playSFX('special://xbmc/scripts/dingdong.wav')\n
     *   - xbmc.playSFX('special://xbmc/scripts/dingdong.wav',False)
     */
    void playSFX(const char* filename, bool useCached = true);

    /**
     * stopSFX() -- Stops wav file
     *
     * example:
     *   - xbmc.stopSFX()
     */
    void stopSFX();
    
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
     * List of Conditions - http://kodi.wiki/view/List_of_Boolean_Conditions
     * 
     * *Note, You can combine two (or more) of the above settings by using "+" as an AND operator,\n
     * "|" as an OR operator, "!" as a NOT operator, and "[" and "]" to bracket expressions.\n
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
     *   - thumb = xbmc.getCacheThumbName('f:\\videos\\movie.avi')
     */
    String getCacheThumbName(const String& path);

    /**
     * makeLegalFilename(filename[, fatX]) -- Returns a legal filename or path as a string.
     * 
     * filename       : string or unicode - filename/path to make legal\n
     * fatX           : [opt] bool - True=Xbox file system(Default)\n
     * 
     * *Note, If fatX is true you should pass a full path. If fatX is false only pass
     *        the basename of the path.
     * 
     *        You can use the above as keywords for arguments and skip certain optional arguments.\n
     *        Once you use a keyword, all following arguments require the keyword.
     * 
     * example:
     *   - filename = xbmc.makeLegalFilename('F:\Trailers\Ice Age: The Meltdown.avi')
     */
    String makeLegalFilename(const String& filename,bool fatX = true);

    /**
     * translatePath(path) -- Returns the translated path.
     * 
     * path           : string or unicode - Path to format
     * 
     * *Note, Only useful if you are coding for both Linux and Windows.\n
     *        e.g. Converts 'special://masterprofile/script_data' -> '/home/user/XBMC/UserData/script_data'
     *        on Linux.
     * 
     * example:
     *   - fpath = xbmc.translatePath('special://masterprofile/script_data')
     */
    String translatePath(const String& path);

    /**
     * getCleanMovieTitle(path[, usefoldername]) -- Returns a clean movie title and year string if available.
     * 
     * path           : string or unicode - String to clean\n
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
     * *Note, Only useful if you are coding for both Linux and Windows for fixing slash problems.\n
     *        e.g. Corrects 'Z://something' -> 'Z:\something'\n
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
     * *Note, If the media resides in a subfolder include it. (eg. home-myfiles\\home-myfiles2.png)
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
     * typ          : integer - use SERVER_* constants\n
     * bStart       : bool - start (True) or stop (False) a server\n
     * bWait        : [opt] bool - wait on stop before returning (not supported by all servers)\n
     * returnValue  : bool - True or False\n
     *
     * example:
     *   - xbmc.startServer(xbmc.SERVER_AIRPLAYSERVER, False)
     */
    bool startServer(int iTyp, bool bStart, bool bWait = false);

    /**
     * audioSuspend() -- Suspend Audio engine.
     * 
     * example:
     *   - xbmc.audioSuspend()
     */
    void audioSuspend();

    /**
     * audioResume() -- Resume Audio engine.
     * 
     * example:
     *   xbmc.audioResume()
     */  
    void audioResume();

    /**
    * convertLanguage(language, format) -- Returns the given language converted to the given format as a string.
    *
    * language: string either as name in English, two letter code (ISO 639-1), or three letter code (ISO 639-2/T(B)
    *
    * format: format of the returned language string\n
    *         xbmc.ISO_639_1: two letter code as defined in ISO 639-1\n
    *         xbmc.ISO_639_2: three letter code as defined in ISO 639-2/T or ISO 639-2/B\n
    *         xbmc.ENGLISH_NAME: full language name in English (default)\n
    *
    * example:
    *   - language = xbmc.convertLanguage(English, xbmc.ISO_639_2)
    */
    String convertLanguage(const char* language, int format); 

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
    SWIG_CONSTANT_FROM_GETTER(int,ISO_639_1);
    SWIG_CONSTANT_FROM_GETTER(int,ISO_639_2);
    SWIG_CONSTANT_FROM_GETTER(int,ENGLISH_NAME);
#if 0
    void registerMonitor(Monitor* monitor);
    void unregisterMonitor(Monitor* monitor);
#endif
  }
}
