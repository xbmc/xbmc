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
 * The following data structures and definations are used
 * by all the types of AddOn's and must be handled by the
 * XBMC part of the AddOn type.
 *
 * Following AddOn Types are currently defined but not all of
 * them are supported at this time.
 *
 * Identifier               ID  Description
 * --------------------------------------------------------
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
    STATUS_DATA_UPDATE,        /* Data structures handled by the AddOn are changed */
    STATUS_NEED_RESTART,       /* Request to restart the AddOn and data structures need updated */
    STATUS_NEED_EMER_RESTART,  /* Request to restart XBMC (hope no AddOn need or do this) */
    STATUS_MISSING_SETTINGS,   /* Some required settings are missing */
    STATUS_BAD_SETTINGS,       /* A setting value is invalid */
    STATUS_WRONG_HOST,         /* AddOn want to connect to unknown host (for ones that use Network) */
    STATUS_INVALID_USER,       /* Invalid or unknown user */
    STATUS_WRONG_PASS,         /* Invalid or wrong password */
    STATUS_MISSING_DATA,       /* Some AddOn data is missing (check log's for missing data) */
    STATUS_MISSING_FILE,       /* A AddOn file is missing (check log's for missing data) */
    STATUS_OUTDATED,           /* Some data is outdated */
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
     * AddOnCallbacks -- Log -- Write a string to XBMC's log file and the debug window.
     *
     * userData       : pointer - Points to the AddOn specific data
     * loglevel       : enum - log level to ouput at. (default=LOG_DEBUG)
     * format         : string - text to output.
     * 
     * Text is written to the log for the following conditions:
     *   LOG_DEBUG  : Show as debug meassage
     *   LOG_INFO   : Show as info meassage
     *   LOG_ERROR  : Show as error meassage
     *   3 : ShowAndGetWriteableDirectory
     *
     * example:
     *  - g_xbmc->AddOn.Log(g_xbmc->userData, LOG_INFO, "This is a test string.");
     */
    typedef void (*AddOnLogCallback)(void *userData, const ADDON_LOG loglevel, const char *format, ... );

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
      DialogOpenSelect          OpenSelect;
      DialogProgressCreate      ProgressCreate;
      DialogProgressUpdate      ProgressUpdate;
      DialogProgressIsCanceled  ProgressIsCanceled;
      DialogProgressClose       ProgressClose;
    } DialogCallbacks;


  /************************************************************************************************************
   * XBMC AddOn Utilities callbacks
   * Helper to access different types of useful functions
   */

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
     *   - const char* string = g_xbmc->Dialog.LocalizedString(g_xbmc->userData, 124);
     */
    typedef const char* (*UtilsLocStrings)(void *userData, long dwCode);

    /**
     * UtilsCallbacks -- UnknownToUTF8 -- Converts a string to UTF8 coding.
     *
     * sourceDest     : string - the string to convert
     *
     * *Note, Returns the entered data as a string in UTF8.
     *
     * example:
     *   - g_xbmc->Dialog.UnknownToUTF8("German have umlauts like 'ÄÖÜäöü' that must be converted");
     */
    typedef const char* (*UtilsUnknownToUTF8)(const char *sourceDest);

    typedef struct UtilsCallbacks
    {
      UtilsUnknownToUTF8     UnknownToUTF8;
      UtilsLocStrings        LocalizedString;
    } UtilsCallbacks;

}

#endif /* __DLL_ADDON_TYPES_H__ */
