#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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

/*
 * Common data structures shared between XBMC and Add-On's
 *
 * The following data structures and definitions are used
 * by all the types of AddOn's and must be handled by the
 * XBMC part of the AddOn type.
 *
 * Following AddOn Types are currently defined but not all of
 * them are supported at this time.
 *
 * Identifier               ID  Description
 * --------------------------------------------------------
 * ADDON_MULTITYPE          0   General usage, like a DVD Burning Tool
 * ADDON_VIZ                1   Visualisation AddOn
 * ADDON_SKIN               2   Skin Theme AddOn
 * ADDON_PVRDLL             3   PVR Client<->Backend Interface
 * ADDON_SCRIPT             4   Phyton script
 * ADDON_SCRAPER            5   Music/Show/Program/Video Scraper
 * ADDON_SCREENSAVER        6   Screensaver
 * ADDON_PLUGIN_PVR         7   PVR/TV plugin extension
 * ADDON_PLUGIN_MUSIC       8   Music plugin extension
 * ADDON_PLUGIN_VIDEO       9   Video plugin extension
 * ADDON_PLUGIN_PROGRAM     10  Program plugin extension
 * ADDON_PLUGIN_PICTURES    11  Pictures plugin extension
 * ADDON_DSP_AUDIO          12  Audio DSP like Surround decoders
 *
 */

#ifndef __DLL_ADDON_TYPES_H__
#define __DLL_ADDON_TYPES_H__

extern "C"
{
  /**
   * XBMC logging levels
   */ 
  enum ADDON_LOG {
    LOG_DEBUG,
    LOG_INFO,
    LOG_ERROR
  };

  /**
   * AddOn Master status types
   * Returned by AddOnStatusCallback if something is wrong.
   * The callback must return a String in second variable for
   * every status that was reported.
   *
   * As example: AddOn report "STATUS_BAD_SETTINGS" in this case
   * the string must be included the name of the setting that is wrong.
   * This is then reported by XBMC to inform the user.
   */ 
  enum ADDON_STATUS {
    STATUS_OK,                 /* Normally not returned (everything is ok) */
    STATUS_INVALID_HOST,       /* AddOn want to connect to unknown host (for ones that use Network) */
    STATUS_INVALID_USER,       /* Invalid or unknown user */
    STATUS_WRONG_PASS,         /* Invalid or wrong password */
    STATUS_LOST_CONNECTION,    /* AddOn lost connection to his backend (for ones that use Network) */
    STATUS_NEED_RESTART,       /* Request to restart the AddOn and data structures need updated */
    STATUS_NEED_EMER_RESTART,  /* Request to restart XBMC (hope no AddOn need or do this) */
    STATUS_MISSING_SETTINGS,   /* Some required settings are missing */
    STATUS_BAD_SETTINGS,       /* A setting value is invalid */
    STATUS_MISSING_FILE,       /* A AddOn file is missing (check log's for missing data) */
    STATUS_UNKNOWN             /* A unknown event is occurred */
  };

  /**
   * XBMC String list array
   * This structure is to hold pointers to strings.
   *
   * Strings field can be allocated by:
   * AddOnStringList.Strings = (const char**) calloc (AddOnStringList.Items+1,sizeof(const char*));
   * where AddOnStringList.Items hold how many strings are must stored.
   *
   * It must be released by the creating function.
   */ 
  typedef struct {
    const char**    Strings;
    int             Items;
  } AddOnStringList;

  /************************************************************************************************************
   * XBMC AddOn Master callbacks
   * Helper's to access AddOn related functions
   */

    /**
     *
     */
    typedef void (*AddOnStatusCallback)(void *userData, const ADDON_STATUS, const char*);

    /**
     * AddOnCallbacks -- Log -- Write a string to XBMC's log file
     *
     * userData       : pointer - Points to the AddOn specific data
     * loglevel       : enum - log level to ouput at. (default=LOG_DEBUG)
     * format         : string - text to output.
     * 
     * Text is written to the log for the following conditions:
     *   LOG_DEBUG  : Show as debug meassage
     *   LOG_INFO   : Show as info meassage
     *   LOG_ERROR  : Show as error meassage
     *
     * example:
     *  - g_xbmc->AddOn.Log(g_xbmc->userData, LOG_INFO, "This is a test string.");
     */
    typedef void (*AddOnLogCallback)(void *userData, const ADDON_LOG loglevel, const char *format, ... );

    /**
     * AddOnCallbacks -- GetSetting -- Get a current addon setting value
     *
     * userData       : pointer - Points to the AddOn specific data
     * settingName    : string - name of the settings used inside the XML Files.
     * settingValue   : pointer - Points to the memory where the value must saved
     * 
     * *Note, Returns True if 'Ok', else False.
     *        Address of Boolean or Integer values can be passed directly by "settingValue".
     *        For character values a buffer with a minimum size of 1024 Bytes must
     *        be used with "settingValue".
     *        If the Setting is not found, "settingValue" is mot modified.
     *
     * example 1:
     *  - char * buffer;
     *  - buffer = (char*) malloc (1024);
     *  - buffer[0] = 0;
     *  - if (g_xbmc->AddOn.GetSetting(g_xbmc->userData, "hostname", buffer))
     *  - {
     *  -   ***do something***
     *  - }
     *  - free(buffer);
     *
     * example 2:
     *  - int port;
     *  - g_xbmc->AddOn.GetSetting(g_xbmc->userData, "port", &port)
     */
    typedef bool (*AddOnGetSetting)(void *userData, const char *settingName, void *settingValue);

    /**
     * AddOnCallbacks -- OpenSettings -- Opens some AddOn settings.
     *
     * url            : string - url of plugin. (addon://addons/pvrVDR/)
     * reload         : bool - reload language strings and settings
     *
     * *Note, Reload is only necessary if calling OpenSettings() from the AddOn.
     *
     * example:
     *  - g_xbmc->AddOn.OpenSettings("addon://addons/pvrVDR/", true);
     */
    typedef void (*AddOnOpenSettings)(const char *url, bool bReload);

    /**
     * AddOnCallbacks -- OpenOwnSettings -- Opens this AddOn settings.
     *
     * userData       : pointer - Points to the AddOn specific data
     * reload         : bool - reload language strings and settings
     *
     * *Note, Reload is only necessary if calling OpenOwnSettings() from the AddOn.
     *
     * example:
     *  - g_xbmc->AddOn.OpenOwnSettings(g_xbmc->userData, true);
     */
    typedef void (*AddOnOpenOwnSettings)(void *userData, bool bReload);

    typedef struct AddOnCallbacks
    {
      AddOnStatusCallback    ReportStatus;
      AddOnLogCallback       Log;
      AddOnGetSetting        GetSetting;
      AddOnOpenSettings      OpenSettings;
      AddOnOpenOwnSettings   OpenOwnSettings;
    } AddOnCallbacks;


  /************************************************************************************************************
   * XBMC AddOn Dialog callbacks
   * Helper's to access GUI Dialog related functions
   */ 

    /**
     * DialogCallbacks -- OpenOK -- Show a dialog 'OK'.
     *
     * heading        : string - dialog heading.
     * line1          : string - line #1 text.
     * line2          : string - line #2 text or NULL (Optional)
     * line3          : string - line #3 text or NULL (Optional)
     *
     * *Note, Returns True if 'Ok' was pressed, else False.
     *
     * example:
     *   - bool ret = g_xbmc->Dialog.OpenOK("XBMC", "There was an error.");
     */
    typedef bool (*DialogOpenOK)(const char*, const char*, const char*, const char*);

    /**
     * DialogCallbacks -- OpenYesNo -- Show a dialog 'YES/NO'.
     *
     * heading        : string - dialog heading.
     * line1          : string - line #1 text.
     * line2          : string - line #2 text or NULL (Optional)
     * line3          : string - line #3 text or NULL (Optional)
     * nolabel        : label to put on the no button or NULL (Optional)
     * yeslabel       : label to put on the yes button or NULL (Optional)
     *
     * *Note, Returns True if 'Yes' was pressed, else False.
     *
     * example:
     *   - bool ret = g_xbmc->Dialog.OpenYesNo("XBMC", "Do you want to exit this AddOn?");
     */
    typedef bool (*DialogOpenYesNo)(const char*, const char*, const char*, const char*, const char*, const char*);

    /**
     * DialogCallbacks -- OpenNumeric -- Show a 'Browse' dialog.
     *
     * type           : integer - the type of browse dialog.
     * heading        : string - dialog heading.
     * shares         : string - from sources.xml. (i.e. 'myprograms')
     * mask           : string - '|' separated file mask. (i.e. '.jpg|.png') or NULL (Optional)
     * useThumbs      : boolean - if True autoswitch to Thumb view if files exist.
     * treatAsFolder  : boolean - if True playlists and archives act as folders.
     * default        : string - default path or file or NULL (Optional)
     *
     * Types:
     *   0 : ShowAndGetDirectory
     *   1 : ShowAndGetFile
     *   2 : ShowAndGetImage
     *   3 : ShowAndGetWriteableDirectory
     *
     * *Note, Returns filename and/or path as a string to the location of the highlighted item,
     *        if user pressed 'Ok' or a masked item was selected.
     *        Returns the default value if dialog was canceled.
     *
     * example:
     *   - g_xbmc->Dialog.OpenBrowse(3, "XBMC", "files", "", false, false, "special://masterprofile/script_data/XBMC Lyrics"));
     */
    typedef const char* (*DialogOpenBrowse)(int, const char*, const char*, const char*, bool, bool, const char*);

    /**
     * DialogCallbacks -- OpenNumeric -- Show a 'Numeric' dialog.
     *
     * type           : integer - the type of numeric dialog
     * heading        : string - dialog heading
     * default        : string - default value or NULL (Optional)
     *
     * Types:
     *   0 : ShowAndGetNumber    (default format: #)
     *   1 : ShowAndGetDate      (default format: DD/MM/YYYY)
     *   2 : ShowAndGetTime      (default format: HH:MM)
     *   3 : ShowAndGetIPAddress (default format: #.#.#.#)
     *  
     * *Note, Returns the entered data as a string.
     *  Returns the default value if dialog was canceled.
     *
     * example:
     *   - g_xbmc->Dialog.OpenNumeric(1, "Enter date of birth", NULL);
     */
    typedef const char* (*DialogOpenNumeric)(int, const char*, const char*);

    /**
     * DialogCallbacks -- OpenKeyboard -- Creates a new Keyboard object with default text
     *                                    heading and hidden input flag if supplied.
     *
     * heading        : string - keyboard heading (Optional).
     * default        : string - default text entry (Optional).
     * hidden         : boolean - True for hidden text entry.
     *
     *  
     * *Note, Returns the entered data as a string.
     *  Returns NULL if keyboard was canceled.
     *
     * example:
     *   - const char *yourName = g_xbmc->Dialog.OpenKeyboard("Enter your Name", "John Doe", false)
     *   - if (yourName != NULL)
     *   - {
     *   -   *** do something ***
     *   - }
     */
    typedef const char* (*DialogOpenKeyboard)(const char*, const char*, bool);

    /**
     * DialogCallbacks -- OpenSelect -- Show a select dialog.
     *
     * heading        : string or unicode - dialog heading.
     * list           : string list - list of items.
     *
     * *Note, Returns the position of the highlighted item as an integer.
     *
     * example:
     *  - AddOnStringList list;
     *  - list.Items = 4;
     *  - list.Strings = (const char**) calloc (list.Items+1,sizeof(const char*));
     *  - list.Strings[0] = "Playlist #1";
     *  - list.Strings[1] = "Playlist #2";
     *  - list.Strings[2] = "Playlist #3";
     *  - list.Strings[3] = "Playlist #4";
     *  - int ret = g_xbmc->Dialog.OpenSelect("Choose a playlist", &list));
     *  - free(list.Strings);
     */
    typedef int (*DialogOpenSelect)(const char*, AddOnStringList*);

    /**
     * DialogCallbacks -- ProgressCreate -- Create and show a progress dialog.
     *
     * heading        : string - dialog heading.
     * line1          : string - line #1 text.
     * line2          : string - line #2 text or NULL (Optional)
     * line3          : string - line #3 text or NULL (Optional)
     *
     * *Note, Use update() to update lines and progressbar.
     *
     * example:
     *   - bool ret = g_xbmc->Dialog.ProgressCreate("XBMC", "Initializing script...", NULL, NULL);
     */
    typedef bool (*DialogProgressCreate)(const char*, const char*, const char*, const char*);

    /**
     * DialogCallbacks -- ProgressUpdate -- Update's the progress dialog.
     *
     * percent        : integer - percent complete. (0:100)
     * line1          : string - line #1 text or NULL (Optional)
     * line2          : string - line #2 text or NULL (Optional)
     * line3          : string - line #3 text or NULL (Optional)
     *
     * *Note, If percent == 0, the progressbar will be hidden.
     *
     * example:
     *   - g_xbmc->Dialog.ProgressUpdate(25, "Importing modules...", NULL, NULL);
     */
    typedef void (*DialogProgressUpdate)(int, const char*, const char*, const char*);

    /**
     * DialogCallbacks -- ProgressClose -- Returns True if the user pressed cancel.
     *
     * example:
     *   - if (g_xbmc->Dialog.ProgressIsCanceled) return;
     */
    typedef bool (*DialogProgressIsCanceled)();

    /**
     * DialogCallbacks -- ProgressClose -- Close the progress dialog.
     *
     * example:
     *   - g_xbmc->Dialog.ProgressClose();
     */
    typedef void (*DialogProgressClose)();

    typedef struct DialogCallbacks
    {
      DialogOpenOK              OpenOK;
      DialogOpenYesNo           OpenYesNo;
      DialogOpenBrowse          OpenBrowse;
      DialogOpenNumeric         OpenNumeric;
      DialogOpenKeyboard        OpenKeyboard;
      DialogOpenSelect          OpenSelect;
      DialogProgressCreate      ProgressCreate;
      DialogProgressUpdate      ProgressUpdate;
      DialogProgressIsCanceled  ProgressIsCanceled;
      DialogProgressClose       ProgressClose;
    } DialogCallbacks;


  /************************************************************************************************************
   * XBMC AddOn GUI callbacks
   * Helper to access different types of GUI functions
   */

    /**
     * GUICallbacks -- Lock -- Lock the gui until xbmcgui.unlock() is called.
     * 
     * *Note, This will improve performance when doing a lot of gui manipulation at once.
     *        The main program (xbmc itself) will freeze until xbmcgui.unlock() is called.
     *
     * example:
     *   - g_xbmc->GUI.Lock());
     */
    typedef void (*GUILock)();

    /**
     * GUICallbacks -- Unlock -- Unlock the gui from a lock() call.
     *
     * example:
     *   - g_xbmc->GUI.Unlock();
     */
    typedef void (*GUIUnlock)();

    /**
     * GUICallbacks -- GetCurrentWindowId -- Returns the id for the current 'active' window as an integer.
     *
     * example:
     *   - int wid = g_xbmc->GUI.GetCurrentWindowId());
     */
    typedef int (*GUIGetCurrentWindowId)();

    /**
     * GUICallbacks -- GetCurrentWindowDialogId -- Returns the id for the current 'active' dialog as an integer.
     *
     * example:
     *   - int wid = g_xbmc->GUI.GetCurrentWindowDialogId());
     */
    typedef int (*GUIGetCurrentWindowDialogId)();

    typedef struct GUICallbacks
    {
      GUILock                       Lock;
      GUIUnlock                     Unlock;
      GUIGetCurrentWindowId         GetCurrentWindowId;
      GUIGetCurrentWindowDialogId   GetCurrentWindowDialogId;
    } GUICallbacks;


  /************************************************************************************************************
   * XBMC AddOn Utilities callbacks
   * Helper to access different types of useful functions
   */
   
    /**
     * UtilsCallbacks -- Shutdown -- Shutdown the xbox.
     *
     * example:
     *   - g_xbmc->Utils.Shutdown();
     */
     typedef void (*UtilsShutdown)();

    /**
     * UtilsCallbacks -- Restart -- Restart the xbox.
     *
     * example:
     *   - g_xbmc->Utils.Restart();
     */
     typedef void (*UtilsRestart)();

    /**
     * UtilsCallbacks -- Dashboard -- Boot to dashboard as set in My Pograms/General.
     *
     * example:
     *   - g_xbmc->Utils.Dashboard();
     */
     typedef void (*UtilsDashboard)();

    /**
     * UtilsCallbacks -- ExecuteScript -- Execute a python script.
    "
    "script         : string - script filename to execute.
    "
    "example:
    "  - g_xbmc->Utils.ExecuteScript("special://home/scripts/update.py");
     */
    typedef void (*UtilsExecuteScript)(const char *script);

    /**
     * UtilsCallbacks -- ExecuteBuiltIn -- Execute a built in XBMC function.
     *
     * function       : string - builtin function to execute.
     *
     * List of functions - http://xbmc.org/wiki/?title=List_of_Built_In_Functions 
     * 
     * example:
     *   - g_xbmc->Utils.ExecuteBuiltIn("XBMC.RunXBE(c:\\\\avalaunch.xbe)");
     */
    typedef void (*UtilsExecuteBuiltIn)(const char *function);

    /**
     * UtilsCallbacks -- ExecuteHttpApi -- Execute an HTTP API command.
     *
     * httpcommand    : string - http command to execute.
     *
     * List of commands - http://xbmc.org/wiki/?title=WebServerHTTP-API#The_Commands
     *
     * example:
     *   - const char* string = g_xbmc->Utils.ExecuteHttpApi("TakeScreenShot(special://temp/test.jpg,0,false,200,-1,90)");
     */
    typedef const char* (*UtilsExecuteHttpApi)(char *httpcommand);

    /**
     * UtilsCallbacks -- LocalizedString -- Returns a localized 'unicode string'.
     *
     * userData       : pointer - Points to the AddOn specific data
     * dwCode         : integer - id# for string you want to localize.
     *
     * *Note, LocalizedString() will fallback to XBMC strings if no string found.
     *
     *       You can use the above as keywords for arguments and skip certain optional arguments.
     *       Once you use a keyword, all following arguments require the keyword.
     *
     * example:
     *   - const char* string = g_xbmc->Utils.LocalizedString(g_xbmc->userData, 124);
     */
    typedef const char* (*UtilsLocStrings)(void *userData, long dwCode);

    /**
     * UtilsCallbacks -- GetSkinDir -- Returns the active skin directory as a string.
     *
     * sourceDest     : string - the string to convert
     *
     * *Note, This is not the full path like 'special://home/skin/MediaCenter', but only 'MediaCenter'.
     *
     * example:
     *   - const char* string = g_xbmc->Utils.GetSkinDir();
     */
    typedef const char* (*UtilsGetSkinDir)();

    /**
     * UtilsCallbacks -- UnknownToUTF8 -- Converts a string to UTF8 coding.
     *
     * sourceDest     : string - the string to convert
     *
     * *Note, Returns the entered data as a string in UTF8.
     *
     * example:
     *   - g_xbmc->Utils.UnknownToUTF8("German have umlauts like 'ÄÖÜäöü' that must be converted");
     */
    typedef const char* (*UtilsUnknownToUTF8)(const char *sourceDest);

    /**
     * UtilsCallbacks -- GetLanguage -- Returns the active language as a string.
     *
     * example:
     *   - const char* language = g_xbmc->Utils.GetLanguage();
     */
    typedef const char* (*UtilsGetLanguage)();

    /**
     * UtilsCallbacks -- GetIPAddress -- Returns the current ip address as a string.
     *
     * example:
     *   - const char* ip = g_xbmc->Utils.GetIPAddress();
     */
    typedef const char* (*UtilsGetIPAddress)();

    /**
     * UtilsCallbacks -- GetDVDState -- Returns the dvd state as an integer.
     *
     * return values are:
     *   0 : DRIVE_OPEN                 : Open...
     *   1 : DRIVE_NOT_READY            : Opening.. Closing...
     *   2 : DRIVE_READY                :
     *   3 : DRIVE_CLOSED_NO_MEDIA      : CLOSED...but no media in drive
     *   4 : DRIVE_CLOSED_MEDIA_PRESENT : Will be send once when the drive just have closed
     *   5 : DRIVE_NONE                 : system doesn't have an optical drive
     *
     * example:
     *   - int dvdstate = g_xbmc->Utils.GetDVDState();
     */
    typedef int (*UtilsGetDVDState)();

    /**
     * UtilsCallbacks -- GetFreeMem -- Returns the amount of free memory in MB as an integer.
     *
     * example:
     *   - int freemem = g_xbmc->Utils.GetFreeMem();
     */
    typedef int (*UtilsGetFreeMem)();
    
    /**
     * UtilsCallbacks -- GetInfoLabel -- Returns an InfoLabel as a string.
     *
     * infotag        : string - infoTag for value you want returned.
     *
     * List of InfoTags - http://xbmc.org/wiki/?title=InfoLabels
     *
     * example:
     *   - const char* label = g_xbmc->Utils.GetInfoLabel("Weather.Conditions");
     */
    typedef const char* (*UtilsGetInfoLabel)(const char *infotag);

    /**
     * UtilsCallbacks -- GetInfoImage -- Returns a filename including path to 
     *                                   the InfoImage's thumbnail as a string.
     *
     * infotag        : string - infoTag for value you want returned.
     *
     * List of InfoTags - http://xbmc.org/wiki/?title=InfoLabels
     *
     * example:
     *   - const char* filename = g_xbmc->Utils.GetInfoImage("Weather.Conditions");
     */
    typedef const char* (*UtilsGetInfoImage)(const char *infotag);

    /**
     * UtilsCallbacks -- EnableNavSounds -- Returns True (1) or False (0) as a bool.
     * 
     * condition      : string - condition to check.
     * 
     * List of Conditions - http://xbmc.org/wiki/?title=List_of_Boolean_Conditions 
     * 
     * *Note, You can combine two (or more) of the above settings by using "+" as an AND operator,
     * "|" as an OR operator, "!" as a NOT operator, and "[" and "]" to bracket expressions.
     * 
     * example:
     *   - visible bool = g_xbmc->Utils.GetCondVisibility("[Control.IsVisible(41) + !Control.IsVisible(12)]");
     */
    typedef bool (*UtilsGetCondVisibility)(const char *condition);

    /**
     * UtilsCallbacks -- EnableNavSounds -- Enables/Disables nav sounds.
     *
     * yesNo         : boolean - enable (True) or disable (False) nav sounds
     *
     * example:
     *   - g_xbmc->Utils.EnableNavSounds(false);
     */
    typedef void (*UtilsEnableNavSounds)(bool yesNo);

    /**
     * UtilsCallbacks -- PlaySFX -- Plays a wav file by filename.
     *
     * filename     : string - filename of the wav file to play.
     *
     * example:
     *   - g_xbmc->Utils.PlaySFX("special://xbmc/scripts/dingdong.wav");
     */
    typedef void (*UtilsPlaySFX)(const char *filename);

    /**
     * UtilsCallbacks -- GetSupportedMedia -- Returns the supported file types for the specific media as a string.
     *
     * media          : integer - media type
     *
     * Media Types:
     *   0 : Video
     *   1 : Music
     *   2 : Pictures
     *
     * *Note, media type can be (video, music, picture).
     *        The return value is a pipe separated string of filetypes (eg. '.mov|.avi').
     *        You can use the above as keywords for arguments.
     *
     * example:
     *   - const char* mTypes = g_xbmc->Utils.GetSupportedMedia("video.png");
     */
    typedef const char* (*UtilsGetSupportedMedia)(int media);

    /**
     * UtilsCallbacks -- GetGlobalIdleTime -- Returns the elapsed idle time in seconds as an integer.
     *
     * example:
     *   - int t = xbmc.GetGlobalIdleTime();
     */
     typedef int (*UtilsGetGlobalIdleTime)();

    /**
     * UtilsCallbacks -- GetCacheThumbName -- Returns a thumb cache filename.
     *
     * path           : string or unicode - path to file
     *
     * example:
     *   - const char* thumb = xbmc.GetCacheThumbName("f:\\\\videos\\\\movie.avi");
     */
     typedef const char* (*UtilsGetCacheThumbName)(const char *path);

    /**
     * UtilsCallbacks -- MakeLegalFilename -- Returns a legal filename or path as a string.
     *
     * filename       : string or unicode - filename/path to make legal
     *
     * example:
     *   - const char* filename = xbmc.MakeLegalFilename("F:\\Trailers\\Ice Age: The Meltdown.avi");
     */
    typedef const char* (*UtilsMakeLegalFilename)(const char *filename);

    /**
     * UtilsCallbacks -- TranslatePath -- Returns the translated path.
     *
     * path           : string or unicode - Path to format
     *
     * *Note, Only useful if you are coding for both Linux and the Xbox.
     *        e.g. Converts 'special://masterprofile/script_data' -> '/home/user/XBMC/UserData/script_data'
     *        on Linux. Would return 'special://masterprofile/script_data' on the Xbox.
     *
     * example:
     *   - const char* fpath = g_xbmc->Utils.TranslatePath("special://masterprofile/script_data");
     */
    typedef const char* (*UtilsTranslatePath)(const char *path);

    /**
     * UtilsCallbacks -- GetRegion -- Returns your regions setting as a string for the specified id.
     *
     * id             : integer - id of setting to return
     *
     * Id Types:
     *   0 : datelong
     *   1 : dateshort
     *   2 : tempunit
     *   3 : speedunit
     *   4 : time
     *   5 : meridiem
     * *Note, choices are (dateshort, datelong, time, meridiem, tempunit, speedunit).
     *        You can use the above as keywords for arguments.
     *
     * example:
     *   - const char* date_long_format = g_xbmc->Utils.GetRegion("datelong");
     */
     typedef const char* (*UtilsGetRegion)(int id);

    /**
     * UtilsCallbacks -- SkinHasImage -- Returns True if the image file exists in the skin.
     *
     * image          : string - image filename
     *
     * *Note, If the media resides in a subfolder include it. (eg. home-myfiles\\\\home-myfiles2.png)
     *        You can use the above as keywords for arguments.
     *
     * example:
     *   - bool exists = g_xbmc->Utils.SkinHasImage("ButtonFocusedTexture.png");
     */
    typedef bool (*UtilsSkinHasImage)(const char *filename);

    typedef struct UtilsCallbacks
    {
      UtilsShutdown             Shutdown;
      UtilsRestart              Restart;
      UtilsDashboard            Dashboard;
      UtilsExecuteScript        ExecuteScript;
      UtilsExecuteBuiltIn       ExecuteBuiltIn;
      UtilsExecuteHttpApi       ExecuteHttpApi;
      UtilsUnknownToUTF8        UnknownToUTF8;
      UtilsLocStrings           LocalizedString;
      UtilsGetSkinDir           GetSkinDir;
      UtilsGetLanguage          GetLanguage;
      UtilsGetIPAddress         GetIPAddress;
      UtilsGetDVDState          GetDVDState;
      UtilsGetFreeMem           GetFreeMem;
      UtilsGetInfoLabel         GetInfoLabel;
      UtilsGetInfoImage         GetInfoImage;
      UtilsGetCondVisibility    GetCondVisibility;
      UtilsEnableNavSounds      EnableNavSounds;
      UtilsPlaySFX              PlaySFX;
      UtilsGetSupportedMedia    GetSupportedMedia;
      UtilsGetGlobalIdleTime    GetGlobalIdleTime;
      UtilsGetCacheThumbName    GetCacheThumbName;
      UtilsMakeLegalFilename    MakeLegalFilename;
      UtilsTranslatePath        TranslatePath;
      UtilsGetRegion            GetRegion;
      UtilsSkinHasImage         SkinHasImage;

    } UtilsCallbacks;

}

#endif /* __DLL_ADDON_TYPES_H__ */
