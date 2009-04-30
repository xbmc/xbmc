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
   * XBMC AddOn Master callbacks (a must do for every AddOn)
   * AddOn's can access XBMC internal helper functions as defined here
   */ 
  typedef void (*AddOnStatusCallback)(void *userData, const ADDON_STATUS, const char*);
  typedef void (*AddOnLogCallback)(void *userData, const ADDON_LOG loglevel, const char *format, ... );
  typedef const char* (*AddOnLocStrings)(long dwCode);
  typedef const char* (*AddOnCharConv)(const char *sourceDest);

}

#endif /* __DLL_ADDON_TYPES_H__ */
